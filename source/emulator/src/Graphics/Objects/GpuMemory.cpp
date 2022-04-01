#include "Emulator/Graphics/Objects/GpuMemory.h"

#include "Kyty/Core/Database.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Profiler.h"
#include "Emulator/VirtualMemory.h"

#include <algorithm>
#include <atomic>
#include <vulkan/vulkan_core.h>

//#define XXH_INLINE_ALL
#include <xxhash/xxhash.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

constexpr int VADDR_BLOCKS_MAX = 3;

enum class OverlapType : uint64_t
{
	None,
	Equals,
	Crosses,
	Contains,
	IsContainedWithin,

	Max
};

constexpr uint64_t ObjectsRelation(GpuMemoryObjectType b, OverlapType relation, GpuMemoryObjectType a)
{
	return static_cast<uint64_t>(a) * static_cast<uint64_t>(GpuMemoryObjectType::Max) * static_cast<uint64_t>(OverlapType::Max) +
	       static_cast<uint64_t>(b) * static_cast<uint64_t>(OverlapType::Max) + static_cast<uint64_t>(relation);
}

static bool addr_in_block(uint64_t block_addr, uint64_t block_size, uint64_t addr)
{
	return addr >= block_addr && addr < block_addr + block_size;
};

static OverlapType GetOverlapType(uint64_t vaddr_a, uint64_t size_a, uint64_t vaddr_b, uint64_t size_b)
{
	// KYTY_PROFILER_FUNCTION();

	EXIT_IF(size_a == 0 || size_b == 0);

	if (vaddr_a == vaddr_b && size_a == size_b)
	{
		return OverlapType::Equals;
	}

	uint64_t vaddr_last_a = vaddr_a + size_a - 1;
	uint64_t vaddr_last_b = vaddr_b + size_b - 1;

	//	auto addr_in_block = [](uint64_t block_addr, uint64_t block_size, uint64_t addr)
	//	{ return addr >= block_addr && addr < block_addr + block_size; };

	bool a_b  = addr_in_block(vaddr_a, size_a, vaddr_b);
	bool a_lb = addr_in_block(vaddr_a, size_a, vaddr_last_b);
	bool b_a  = addr_in_block(vaddr_b, size_b, vaddr_a);
	bool b_la = addr_in_block(vaddr_b, size_b, vaddr_last_a);

	if (a_b && a_lb)
	{
		return OverlapType::Contains;
	}

	if (b_a && b_la)
	{
		return OverlapType::IsContainedWithin;
	}

	if ((a_b && b_la) || (b_a && a_lb))
	{
		return OverlapType::Crosses;
	}

	return OverlapType::None;
}

class MemoryWatcher
{
public:
	MemoryWatcher() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~MemoryWatcher() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(MemoryWatcher);

	using callback_func_t = void (*)(void*, void*);

	void Watch(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, callback_func_t func, void* arg0, void* arg1);
	void Stop(const uint64_t* vaddr, const uint64_t* size, int vaddr_num);
	bool AlreadyWatch(const uint64_t* vaddr, const uint64_t* size, int vaddr_num);
	bool Check(uint64_t vaddr, uint64_t size);

	static bool Enabled()
	{
		static const bool enabled = !Core::dbg_is_debugger_present();
		return enabled;
	}

private:
	static constexpr uint64_t PAGES_NUM = 0x400000;

	struct Callback
	{
		callback_func_t func = nullptr;
		void*           arg0 = nullptr;
		void*           arg1 = nullptr;
	};

	struct Block
	{
		bool     used                         = false;
		uint64_t vaddr[VADDR_BLOCKS_MAX]      = {};
		uint64_t size[VADDR_BLOCKS_MAX]       = {};
		uint64_t page_start[VADDR_BLOCKS_MAX] = {};
		uint64_t page_end[VADDR_BLOCKS_MAX]   = {};
		int      vaddr_num                    = 0;
		Callback cb;
	};

	void Delete(Block* b);

	Core::Mutex   m_mutex;
	uint8_t       m_pages[PAGES_NUM] = {};
	Vector<Block> m_blocks;
};

class GpuMemory
{
public:
	GpuMemory()
	{
		EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
		DbgInit();
	}
	virtual ~GpuMemory() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(GpuMemory);

	bool IsAllocated(uint64_t vaddr, uint64_t size);
	void SetAllocatedRange(uint64_t vaddr, uint64_t size);
	void Free(GraphicContext* ctx, uint64_t vaddr, uint64_t size, bool unmap);

	void* CreateObject(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
	                   const GpuObject& info);
	void  ResetHash(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type);
	void  FrameDone();

	Vector<GpuMemoryObject> FindObjects(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, bool exact, bool only_first);

	// Sync: GPU -> CPU
	void WriteBack(GraphicContext* ctx);

	// Sync: CPU -> GPU
	void Flush(GraphicContext* ctx);

	void DbgInit();
	void DbgDbDump();
	void DbgDbSave(const String& file_name);

private:
	static constexpr int OBJ_OVERLAPS_MAX = 2;

	struct AllocatedRange
	{
		uint64_t vaddr;
		uint64_t size;
	};

	struct ObjectInfo
	{
		GpuMemoryObject              object;
		uint64_t                     params[GpuObject::PARAMS_MAX] = {};
		uint64_t                     hash[VADDR_BLOCKS_MAX]        = {};
		uint64_t                     cpu_update_time               = 0;
		uint64_t                     gpu_update_time               = 0;
		GpuObject::write_back_func_t write_back_func               = nullptr;
		GpuObject::delete_func_t     delete_func                   = nullptr;
		GpuObject::update_func_t     update_func                   = nullptr;
		uint64_t                     use_last_frame                = 0;
		uint64_t                     use_num                       = 0;
		bool                         in_use                        = false;
		bool                         read_only                     = false;
		bool                         check_hash                    = false;
		VulkanMemory                 mem;
	};

	struct OverlappedBlock
	{
		OverlapType relation  = OverlapType::None;
		int         object_id = -1;
	};

	struct Block
	{
		uint64_t vaddr[VADDR_BLOCKS_MAX] = {};
		uint64_t size[VADDR_BLOCKS_MAX]  = {};
		int      vaddr_num               = 0;
	};

	struct Object
	{
		Block                   block;
		ObjectInfo              info;
		Vector<OverlappedBlock> others;
		GpuMemoryScenario       scenario = GpuMemoryScenario::Common;
		bool                    free     = true;
	};

	void Free(GraphicContext* ctx, int object_id);

	Vector<OverlappedBlock> FindBlocks(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, bool only_first = false);
	Block                   CreateBlock(const uint64_t* vaddr, const uint64_t* size, int vaddr_num);
	void                    DeleteBlock(Block* b);
	void                    Link(int id1, int id2, OverlapType rel, GpuMemoryScenario scenario);

	// Update (CPU -> GPU)
	void Update(GraphicContext* ctx, int obj_id);

	static void WatchCallback(void* a0, void* a1);

	bool create_existing(const Vector<OverlappedBlock>& others, const GpuObject& info, int* id);
	bool create_generate_mips(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type);
	bool create_texture_triplet(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type);
	bool create_all_the_same(const Vector<OverlappedBlock>& others);
	void create_dbg_exit(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const Vector<OverlappedBlock>& others,
	                     GpuMemoryObjectType type);

	Core::Mutex m_mutex;

	Vector<AllocatedRange> m_allocated;
	Vector<Object>         m_objects;
	uint64_t               m_objects_size  = 0;
	uint64_t               m_current_frame = 0;

	Core::Database::Connection m_db;
	Core::Database::Statement* m_db_add_range  = nullptr;
	Core::Database::Statement* m_db_add_object = nullptr;
};

class GpuResources
{
public:
	struct Info
	{
		uint32_t owner  = 0;
		bool     free   = true;
		uint64_t memory = 0;
		size_t   size   = 0;
		String   name;
		uint32_t type      = 0;
		uint64_t user_data = 0;
	};

	GpuResources() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~GpuResources() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(GpuResources);

	uint32_t AddOwner(const String& name);
	uint32_t AddResource(uint32_t owner_handle, uint64_t memory, size_t size, const String& name, uint32_t type, uint64_t user_data);
	void     DeleteOwner(uint32_t owner_handle);
	void     DeleteResources(uint32_t owner_handle);
	void     DeleteResource(uint32_t resource_handle);

	bool FindInfo(uint64_t memory, Info* dst);

private:
	struct Owner
	{
		String name;
		bool   free = true;
	};

	Core::Mutex m_mutex;

	Vector<Owner> m_owners;
	Vector<Info>  m_infos;
};

static GpuMemory*     g_gpu_memory         = nullptr;
static GpuResources*  g_gpu_resources      = nullptr;
static MemoryWatcher* g_gpu_memory_watcher = nullptr;

void MemoryWatcher::Watch(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, callback_func_t func, void* arg0, void* arg1)
{
	KYTY_PROFILER_BLOCK("MemoryWatcher::Watch");

	if (!Enabled())
	{
		return;
	}

	Core::LockGuard lock(m_mutex);

	if (AlreadyWatch(vaddr, size, vaddr_num))
	{
		return;
	}

	Stop(vaddr, size, vaddr_num);

	EXIT_IF(func == nullptr);

	Block nb;
	nb.used      = true;
	nb.cb.func   = func;
	nb.cb.arg0   = arg0;
	nb.cb.arg1   = arg1;
	nb.vaddr_num = vaddr_num;
	for (int i = 0; i < vaddr_num; i++)
	{
		EXIT_IF(size[i] == 0);
		nb.vaddr[i]      = vaddr[i];
		nb.size[i]       = size[i];
		nb.page_start[i] = vaddr[i] >> 12u;
		nb.page_end[i]   = (vaddr[i] + size[i] - 1) >> 12u;

		EXIT_IF(nb.page_end[i] < nb.page_start[i]);

		struct Region
		{
			uint64_t start = 0;
			uint64_t count = 0;
		};

		Vector<Region> rs;

		Region r;

		for (uint64_t page = nb.page_start[i]; page <= nb.page_end[i]; page++)
		{
			EXIT_NOT_IMPLEMENTED(page >= PAGES_NUM);
			EXIT_NOT_IMPLEMENTED(m_pages[page] == 0xff);

			if (m_pages[page] == 0)
			{
				if (r.count == 0)
				{
					r.start = page;
				}
				r.count++;
			} else
			{
				if (r.count > 0)
				{
					rs.Add(r);
					r.count = 0;
				}
			}

			m_pages[page]++;
		}

		if (r.count > 0)
		{
			rs.Add(r);
		}

		for (const auto& r: rs)
		{
			Kyty::Loader::VirtualMemory::Mode old_mode = Kyty::Loader::VirtualMemory::Mode::NoAccess;

			// printf("protect %016" PRIx64 "\n", page << 12u);

			Kyty::Loader::VirtualMemory::Protect(r.start << 12u, r.count * 0x1000, Kyty::Loader::VirtualMemory::Mode::Read, &old_mode);

			EXIT_NOT_IMPLEMENTED(old_mode != Kyty::Loader::VirtualMemory::Mode::ReadWrite);
		}
	}

	for (auto& b: m_blocks)
	{
		if (!b.used)
		{
			b = nb;
			return;
		}
	}

	m_blocks.Add(nb);
}

void MemoryWatcher::Delete(Block* b)
{
	KYTY_PROFILER_BLOCK("MemoryWatcher::Delete");

	EXIT_IF(!b->used);

	b->used = false;

	for (int i = 0; i < b->vaddr_num; i++)
	{
		struct Region
		{
			uint64_t start = 0;
			uint64_t count = 0;
		};

		Vector<Region> rs;

		Region r;

		for (uint64_t page = b->page_start[i]; page <= b->page_end[i]; page++)
		{
			EXIT_NOT_IMPLEMENTED(m_pages[page] == 0);

			m_pages[page]--;

			if (m_pages[page] == 0)
			{
				if (r.count == 0)
				{
					r.start = page;
				}
				r.count++;
			} else
			{
				if (r.count > 0)
				{
					rs.Add(r);
					r.count = 0;
				}
			}
		}

		if (r.count > 0)
		{
			rs.Add(r);
		}

		for (const auto& r: rs)
		{
			Kyty::Loader::VirtualMemory::Mode old_mode = Kyty::Loader::VirtualMemory::Mode::NoAccess;

			// printf("unprotect %016" PRIx64 "\n", page << 12u);

			Kyty::Loader::VirtualMemory::Protect(r.start << 12u, r.count * 0x1000, Kyty::Loader::VirtualMemory::Mode::ReadWrite, &old_mode);

			EXIT_NOT_IMPLEMENTED(old_mode != Kyty::Loader::VirtualMemory::Mode::Read);
		}
	}
}

void MemoryWatcher::Stop(const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("MemoryWatcher::Stop");

	if (!Enabled())
	{
		return;
	}

	Core::LockGuard lock(m_mutex);

	for (auto& b: m_blocks)
	{
		if (b.used && b.vaddr_num == vaddr_num)
		{
			bool equal = true;
			for (int i = 0; i < vaddr_num; i++)
			{
				if (GetOverlapType(b.vaddr[i], b.size[i], vaddr[i], size[i]) != OverlapType::Equals)
				{
					equal = false;
					break;
				}
			}
			if (equal)
			{
				Delete(&b);
			}
		}
	}
}

bool MemoryWatcher::AlreadyWatch(const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	KYTY_PROFILER_BLOCK("MemoryWatcher::AlreadyWatch");

	if (!Enabled())
	{
		return false;
	}

	Core::LockGuard lock(m_mutex);

	for (auto& b: m_blocks)
	{
		if (b.used && b.vaddr_num == vaddr_num)
		{
			bool equal = true;
			for (int i = 0; i < vaddr_num; i++)
			{
				if (GetOverlapType(b.vaddr[i], b.size[i], vaddr[i], size[i]) != OverlapType::Equals)
				{
					equal = false;
					break;
				}
			}
			if (equal)
			{
				return true;
			}
		}
	}

	return false;
}

bool MemoryWatcher::Check(uint64_t vaddr, uint64_t size)
{
	if (!Enabled())
	{
		return false;
	}

	//	printf("check begin\n");
	//
	//	Core::DebugStack s;
	//	Core::DebugStack::Trace(&s);
	//	s.Print(0);

	m_mutex.Lock();

	uint64_t page_start = vaddr >> 12u;
	uint64_t page_end   = (vaddr + size - 1) >> 12u;

	Vector<Callback> cbs;

	for (uint64_t page = page_start; page <= page_end; page++)
	{
		if (page < PAGES_NUM)
		{
			for (auto& b: m_blocks)
			{
				if (b.used)
				{
					for (int i = 0; i < b.vaddr_num; i++)
					{
						if (b.page_start[i] <= page && b.page_end[i] >= page)
						{
							cbs.Add(b.cb);
							Delete(&b);
							break;
						}
					}
				}
			}
		}
	}

	m_mutex.Unlock();

	for (const auto& cb: cbs)
	{
		cb.func(cb.arg0, cb.arg1);
	}

	// printf("check end\n");

	return !cbs.IsEmpty();
}

uint32_t GpuResources::AddOwner(const String& name)
{
	Core::LockGuard lock(m_mutex);

	Owner n;
	n.name = name;
	n.free = false;

	uint32_t index = 0;
	for (auto& b: m_owners)
	{
		if (b.free)
		{
			b = n;
			return index;
		}
		index++;
	}

	m_owners.Add(n);

	return index;
}

uint32_t GpuResources::AddResource(uint32_t owner_handle, uint64_t memory, size_t size, const String& name, uint32_t type,
                                   uint64_t user_data)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(!m_owners.IndexValid(owner_handle));
	EXIT_NOT_IMPLEMENTED(memory == 0);

	Info info;
	info.owner     = owner_handle;
	info.memory    = memory;
	info.free      = false;
	info.name      = name;
	info.size      = size;
	info.type      = type;
	info.user_data = user_data;

	uint32_t index = 0;
	for (auto& i: m_infos)
	{
		if (i.free)
		{
			i = info;
			return index;
		}
		index++;
	}

	m_infos.Add(info);

	return index;
}

void GpuResources::DeleteOwner(uint32_t owner_handle)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(!m_owners.IndexValid(owner_handle));

	for (auto& i: m_infos)
	{
		if (!i.free && i.owner == owner_handle)
		{
			i.free = true;
		}
	}

	EXIT_NOT_IMPLEMENTED(m_owners[owner_handle].free);

	m_owners[owner_handle].free = true;
}

void GpuResources::DeleteResources(uint32_t owner_handle)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(!m_owners.IndexValid(owner_handle));

	for (auto& i: m_infos)
	{
		if (!i.free && i.owner == owner_handle)
		{
			i.free = true;
		}
	}
}

void GpuResources::DeleteResource(uint32_t resource_handle)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(!m_infos.IndexValid(resource_handle));

	EXIT_NOT_IMPLEMENTED(m_infos[resource_handle].free);

	m_infos[resource_handle].free = true;
}

bool GpuResources::FindInfo(uint64_t memory, Info* dst)
{
	EXIT_IF(dst == nullptr);

	Core::LockGuard lock(m_mutex);

	// NOLINTNEXTLINE(readability-use-anyofallof)
	for (const auto& i: m_infos)
	{
		if (!i.free && memory >= i.memory && memory < i.memory + i.size)
		{
			*dst = i;
			return true;
		}
	}

	return false;
}

void GpuMemory::SetAllocatedRange(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(size == 0);

	EXIT_NOT_IMPLEMENTED(IsAllocated(vaddr, size));

	Core::LockGuard lock(m_mutex);

	AllocatedRange r {};
	r.vaddr = vaddr;
	r.size  = size;

	m_allocated.Add(r);
}

bool GpuMemory::IsAllocated(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(size == 0);

	Core::LockGuard lock(m_mutex);

	return std::any_of(m_allocated.begin(), m_allocated.end(),
	                   [vaddr, size](auto& r) {
		                   return ((vaddr >= r.vaddr && vaddr < r.vaddr + r.size) ||
		                           ((vaddr + size - 1) >= r.vaddr && (vaddr + size - 1) < r.vaddr + r.size));
	                   });
}

static uint64_t calc_hash(const uint8_t* buf, uint64_t size)
{
	KYTY_PROFILER_FUNCTION();

	return (size > 0 && buf != nullptr ? XXH64(buf, size, 0) : 0);
}

static uint64_t get_current_time()
{
	static std::atomic_uint64_t t(0);
	return ++t;
}

void GpuMemory::Link(int id1, int id2, OverlapType rel, GpuMemoryScenario scenario)
{
	OverlapType other_rel = OverlapType::None;
	switch (rel)
	{
		case OverlapType::Equals: other_rel = OverlapType::Equals; break;
		case OverlapType::IsContainedWithin: other_rel = OverlapType::Contains; break;
		case OverlapType::Contains: other_rel = OverlapType::IsContainedWithin; break;
		default: EXIT("invalid rel: %s\n", Core::EnumName(rel).C_Str());
	}

	auto& h1 = m_objects[id1];
	EXIT_IF(h1.free);

	auto& h2 = m_objects[id2];
	EXIT_IF(h2.free);

	h1.others.Add({rel, id2});
	h2.others.Add({other_rel, id1});

	h1.scenario = scenario;
	h2.scenario = scenario;
}

void GpuMemory::Update(GraphicContext* ctx, int obj_id)
{
	KYTY_PROFILER_BLOCK("GpuMemory::Update");

	auto& h           = m_objects[obj_id];
	auto& o           = h.info;
	bool  need_update = false;

	bool mem_watch = MemoryWatcher::Enabled();

	if ((mem_watch && o.cpu_update_time > o.gpu_update_time) || !mem_watch)
	{
		uint64_t hash[VADDR_BLOCKS_MAX] = {};

		for (int vi = 0; vi < h.block.vaddr_num; vi++)
		{
			EXIT_IF(h.block.size[vi] == 0);

			if (o.check_hash)
			{
				hash[vi] = calc_hash(reinterpret_cast<const uint8_t*>(h.block.vaddr[vi]), h.block.size[vi]);
			} else
			{
				hash[vi] = 0;
			}
		}

		for (int vi = 0; vi < h.block.vaddr_num; vi++)
		{
			if (o.hash[vi] != hash[vi])
			{
				printf("Update (CPU -> GPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n",
				       Core::EnumName(o.object.type).C_Str(), h.block.vaddr[vi], h.block.size[vi]);
				need_update = true;
				o.hash[vi]  = hash[vi];
			}
		}
	}

	if (need_update)
	{
		EXIT_IF(o.update_func == nullptr);
		o.update_func(ctx, o.params, o.object.obj, h.block.vaddr, h.block.size, h.block.vaddr_num);
		o.gpu_update_time = get_current_time();
	}

	g_gpu_memory_watcher->Watch(h.block.vaddr, h.block.size, h.block.vaddr_num, WatchCallback, this,
	                            reinterpret_cast<void*>(static_cast<intptr_t>(obj_id)));
}

void GpuMemory::WatchCallback(void* a0, void* a1)
{
	auto* m     = static_cast<GpuMemory*>(a0);
	auto  index = reinterpret_cast<intptr_t>(a1);

	m->m_mutex.Lock();

	EXIT_NOT_IMPLEMENTED(m->m_objects[index].free);

	m->m_objects[index].info.cpu_update_time = get_current_time();

	m->m_mutex.Unlock();
}

bool GpuMemory::create_existing(const Vector<OverlappedBlock>& others, const GpuObject& info, int* id)
{
	EXIT_IF(id == nullptr);

	uint64_t               max_gpu_update_time = 0;
	const OverlappedBlock* latest_block        = nullptr;

	for (const auto& obj: others)
	{
		auto& h = m_objects[obj.object_id];
		EXIT_IF(h.free);
		auto& o = h.info;
		if (h.scenario == GpuMemoryScenario::Common && obj.relation == OverlapType::Equals && o.object.type == info.type &&
		    info.Equal(o.params))
		{
			*id = obj.object_id;
			return true;
		}

		if (o.gpu_update_time > max_gpu_update_time)
		{
			max_gpu_update_time = o.gpu_update_time;
			latest_block        = &obj;
		}
	}

	if (latest_block != nullptr)
	{
		auto& h = m_objects[latest_block->object_id];
		auto& o = h.info;

		if (h.scenario == GpuMemoryScenario::GenerateMips && latest_block->relation == OverlapType::Equals && o.object.type == info.type &&
		    info.Equal(o.params))
		{
			*id = latest_block->object_id;
			return true;
		}
	}

	return false;
}

bool GpuMemory::create_generate_mips(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type)
{
	if (others.Size() == 3 && type == GpuMemoryObjectType::RenderTexture)
	{
		const auto&         b0    = others.At(0);
		const auto&         b1    = others.At(1);
		const auto&         b2    = others.At(2);
		OverlapType         rel0  = b0.relation;
		OverlapType         rel1  = b1.relation;
		OverlapType         rel2  = b2.relation;
		const auto&         o0    = m_objects[b0.object_id];
		const auto&         o1    = m_objects[b1.object_id];
		const auto&         o2    = m_objects[b2.object_id];
		GpuMemoryObjectType type0 = o0.info.object.type;
		GpuMemoryObjectType type1 = o1.info.object.type;
		GpuMemoryObjectType type2 = o2.info.object.type;

		if (rel0 == OverlapType::Contains && rel1 == OverlapType::Contains && rel2 == OverlapType::Contains &&
		    type0 == GpuMemoryObjectType::StorageBuffer && type1 == GpuMemoryObjectType::Texture &&
		    type2 == GpuMemoryObjectType::StorageTexture &&
		    ((o0.others.Size() == 2 && o0.scenario == GpuMemoryScenario::TextureTriplet && o1.others.Size() == 2 &&
		      o1.scenario == GpuMemoryScenario::TextureTriplet && o2.others.Size() == 2 &&
		      o2.scenario == GpuMemoryScenario::TextureTriplet) ||
		     (o0.others.Size() >= 3 && o0.scenario == GpuMemoryScenario::GenerateMips && o1.others.Size() >= 3 &&
		      o1.scenario == GpuMemoryScenario::GenerateMips && o2.others.Size() >= 3 && o2.scenario == GpuMemoryScenario::GenerateMips)))
		{
			return true;
		}
	} else if (others.Size() >= 3 && type == GpuMemoryObjectType::Texture)
	{
		const auto&         b0    = others.At(0);
		const auto&         b1    = others.At(1);
		const auto&         b2    = others.At(2);
		OverlapType         rel0  = b0.relation;
		OverlapType         rel1  = b1.relation;
		OverlapType         rel2  = b2.relation;
		const auto&         o0    = m_objects[b0.object_id];
		const auto&         o1    = m_objects[b1.object_id];
		const auto&         o2    = m_objects[b2.object_id];
		GpuMemoryObjectType type0 = o0.info.object.type;
		GpuMemoryObjectType type1 = o1.info.object.type;
		GpuMemoryObjectType type2 = o2.info.object.type;

		if (((rel0 == OverlapType::Contains && rel1 == OverlapType::Contains && rel2 == OverlapType::Contains) ||
		     (rel0 == OverlapType::Equals && rel1 == OverlapType::Equals && rel2 == OverlapType::Equals)) &&
		    type0 == GpuMemoryObjectType::StorageBuffer && type1 == GpuMemoryObjectType::Texture &&
		    type2 == GpuMemoryObjectType::StorageTexture && o0.scenario == GpuMemoryScenario::GenerateMips &&
		    o1.scenario == GpuMemoryScenario::GenerateMips && o2.scenario == GpuMemoryScenario::GenerateMips)
		{
			return true;
		}
	}

	return false;
}

bool GpuMemory::create_texture_triplet(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type)
{
	if (others.Size() == 2 && type == GpuMemoryObjectType::StorageTexture)
	{
		const auto&         b0    = others.At(0);
		const auto&         b1    = others.At(1);
		OverlapType         rel0  = b0.relation;
		OverlapType         rel1  = b1.relation;
		const auto&         o0    = m_objects[b0.object_id];
		const auto&         o1    = m_objects[b1.object_id];
		GpuMemoryObjectType type0 = o0.info.object.type;
		GpuMemoryObjectType type1 = o1.info.object.type;

		if (rel0 == OverlapType::Equals && rel1 == OverlapType::Equals && type0 == GpuMemoryObjectType::StorageBuffer &&
		    type1 == GpuMemoryObjectType::Texture &&
		    (o0.others.Size() == 1 && o0.scenario == GpuMemoryScenario::Common && o1.others.Size() == 1 &&
		     o1.scenario == GpuMemoryScenario::Common))
		{
			return true;
		}
	}
	return false;
}

bool GpuMemory::create_all_the_same(const Vector<OverlappedBlock>& others)
{
	OverlapType         rel  = others.At(0).relation;
	GpuMemoryObjectType type = m_objects[others.At(0).object_id].info.object.type;

	return std::all_of(others.begin(), others.end(),
	                   [rel, type, this](auto& r) { return (rel == r.relation && type == m_objects[r.object_id].info.object.type); });
}

void GpuMemory::create_dbg_exit(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const Vector<OverlappedBlock>& others,
                                GpuMemoryObjectType type)
{
	printf("Exit:\n");

	for (int vi = 0; vi < vaddr_num; vi++)
	{
		printf("\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", vaddr[vi], size[vi]);
	}

	printf("\t info.type = %s\n", Core::EnumName(type).C_Str());
	for (const auto& d: others)
	{
		printf("\t id = %d, rel = %s\n", d.object_id, Core::EnumName(d.relation).C_Str());
	}
	DbgDbDump();
	DbgDbSave(U"_gpu_memory.db");
	KYTY_NOT_IMPLEMENTED;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void* GpuMemory::CreateObject(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                              const GpuObject& info)
{
	KYTY_PROFILER_BLOCK("GpuMemory::CreateObject", profiler::colors::Green300);

	EXIT_IF(info.type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	bool overlap             = false;
	bool delete_all          = false;
	bool create_from_objects = false;

	GpuMemoryScenario scenario = GpuMemoryScenario::Common;

	auto others = FindBlocks(vaddr, size, vaddr_num);

	if (!others.IsEmpty())
	{
		int existing_id = -1;
		if (create_existing(others, info, &existing_id))
		{
			auto& h = m_objects[existing_id];
			EXIT_IF(h.free);
			auto& o = h.info;

			Update(ctx, existing_id);

			o.use_num++;
			o.use_last_frame = m_current_frame;
			o.in_use         = true;
			o.read_only      = info.read_only;
			o.check_hash     = info.check_hash;

			return o.object.obj;
		}

		if (others.Size() == 1)
		{
			const auto& obj = others.At(0);
			auto&       h   = m_objects[obj.object_id];
			EXIT_IF(h.free);
			auto& o = h.info;

			switch (ObjectsRelation(o.object.type, obj.relation, info.type))
			{
				case ObjectsRelation(GpuMemoryObjectType::StorageBuffer, OverlapType::Equals, GpuMemoryObjectType::RenderTexture):
				case ObjectsRelation(GpuMemoryObjectType::StorageBuffer, OverlapType::Equals, GpuMemoryObjectType::StorageTexture):
				case ObjectsRelation(GpuMemoryObjectType::StorageBuffer, OverlapType::Equals, GpuMemoryObjectType::Texture):
				case ObjectsRelation(GpuMemoryObjectType::VideoOutBuffer, OverlapType::Equals, GpuMemoryObjectType::StorageBuffer):
				{
					overlap = true;
					break;
				}
				case ObjectsRelation(GpuMemoryObjectType::StorageBuffer, OverlapType::Contains, GpuMemoryObjectType::Label):
				case ObjectsRelation(GpuMemoryObjectType::Label, OverlapType::Equals, GpuMemoryObjectType::Label):
				{
					delete_all = true;
					break;
				}
				case ObjectsRelation(GpuMemoryObjectType::StorageTexture, OverlapType::Equals, GpuMemoryObjectType::Texture):
				{
					overlap             = true;
					create_from_objects = true;
					break;
				}
				default:
					printf("unknown relation: %s - %s - %s\n", Core::EnumName(o.object.type).C_Str(), Core::EnumName(obj.relation).C_Str(),
					       Core::EnumName(info.type).C_Str());
					create_dbg_exit(vaddr, size, vaddr_num, others, info.type);
			}
		} else
		{
			if (create_generate_mips(others, info.type))
			{
				overlap             = true;
				create_from_objects = true;
				scenario            = GpuMemoryScenario::GenerateMips;
			} else if (create_texture_triplet(others, info.type))
			{
				overlap  = true;
				scenario = GpuMemoryScenario::TextureTriplet;
			} else
			{
				if (!create_all_the_same(others))
				{
					create_dbg_exit(vaddr, size, vaddr_num, others, info.type);
				}

				OverlapType         rel  = others.At(0).relation;
				GpuMemoryObjectType type = m_objects[others.At(0).object_id].info.object.type;

				switch (ObjectsRelation(type, rel, info.type))
				{
					case ObjectsRelation(GpuMemoryObjectType::Label, OverlapType::IsContainedWithin, GpuMemoryObjectType::StorageBuffer):
						delete_all = true;
						break;
					case ObjectsRelation(GpuMemoryObjectType::RenderTexture, OverlapType::IsContainedWithin, GpuMemoryObjectType::Texture):
						overlap             = true;
						create_from_objects = true;
						break;
					default:
						EXIT("unknown relation: %s - %s - %s\n", Core::EnumName(type).C_Str(), Core::EnumName(rel).C_Str(),
						     Core::EnumName(info.type).C_Str());
				}
			}
		}
	}

	EXIT_IF(delete_all && overlap);

	if (delete_all)
	{
		for (const auto& obj: others)
		{
			Free(ctx, obj.object_id);
		}
	}

	for (int vi = 0; vi < vaddr_num; vi++)
	{
		EXIT_NOT_IMPLEMENTED(!IsAllocated(vaddr[vi], size[vi]));
	}

	ObjectInfo o {};

	for (int i = 0; i < GpuObject::PARAMS_MAX; i++)
	{
		o.params[i] = info.params[i];
	}

	uint64_t hash[VADDR_BLOCKS_MAX] = {};

	for (int vi = 0; vi < vaddr_num; vi++)
	{
		EXIT_IF(size[vi] == 0);

		if (info.check_hash)
		{
			hash[vi] = calc_hash(reinterpret_cast<const uint8_t*>(vaddr[vi]), size[vi]);
		} else
		{
			hash[vi] = 0;
		}
	}

	o.object.type = info.type;
	o.object.obj  = nullptr;
	for (int vi = 0; vi < vaddr_num; vi++)
	{
		o.hash[vi] = hash[vi];
	}
	o.cpu_update_time = get_current_time();
	o.gpu_update_time = o.cpu_update_time;

	if (create_from_objects)
	{
		Vector<GpuMemoryObject> objects;
		for (const auto& obj: others)
		{
			const auto& o2 = m_objects[obj.object_id].info;
			objects.Add(o2.object);
		}
		auto create_func = info.GetCreateFromObjectsFunc();
		EXIT_IF(create_func == nullptr);
		o.object.obj = create_func(ctx, buffer, o.params, scenario, objects, &o.mem);
	} else
	{
		o.object.obj = info.GetCreateFunc()(ctx, o.params, vaddr, size, vaddr_num, &o.mem);
	}

	o.write_back_func = info.GetWriteBackFunc();
	o.delete_func     = info.GetDeleteFunc();
	o.update_func     = info.GetUpdateFunc();
	o.use_num         = 1;
	o.use_last_frame  = m_current_frame;
	o.in_use          = true;
	o.read_only       = info.read_only;
	o.check_hash      = info.check_hash;

	bool updated = false;

	int index = 0;
	for (auto& u: m_objects)
	{
		if (u.free)
		{
			u.free  = false;
			u.block = CreateBlock(vaddr, size, vaddr_num);
			u.info  = o;
			u.others.Clear();
			u.scenario = scenario;
			updated    = true;
			break;
		}
		index++;
	}

	if (!updated)
	{
		Object h {};
		h.block = CreateBlock(vaddr, size, vaddr_num);
		h.info  = o;
		h.others.Clear();
		h.scenario = scenario;
		h.free     = false;
		m_objects.Add(h);
	}

	if (overlap)
	{
		for (const auto& obj: others)
		{
			Link(index, obj.object_id, obj.relation, scenario);
		}
	}

	return o.object.obj;
}

Vector<GpuMemoryObject> GpuMemory::FindObjects(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, bool exact, bool only_first)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindObjects", profiler::colors::Green200);

	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	auto objects = FindBlocks(vaddr, size, vaddr_num, only_first);

	Vector<GpuMemoryObject> ret;

	// printf("GpuMemory::FindObjects()\n");

	//	for (int i = 0; i < vaddr_num; i++)
	//	{
	//		printf("\t vaddr = %016" PRIx64 ", size = %016" PRIx64 "\n", vaddr[i], size[i]);
	//	}

	for (const auto& obj: objects)
	{
		const auto& h = m_objects[obj.object_id];
		EXIT_IF(h.free);
		// printf("\t %s, %d, %s\n", Core::EnumName(obj.relation).C_Str(), obj.object_id, Core::EnumName(h.info.object.type).C_Str());
		if (obj.relation == OverlapType::Equals || (!exact && obj.relation == OverlapType::IsContainedWithin))
		{
			ret.Add(h.info.object);
		}
	}

	return ret;
}

void GpuMemory::ResetHash(GraphicContext* /*ctx*/, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type)
{
	EXIT_IF(type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	uint64_t new_hash = 0;

	auto object_ids = FindBlocks(vaddr, size, vaddr_num);

	if (!object_ids.IsEmpty())
	{
		for (const auto& obj: object_ids)
		{
			auto& h = m_objects[obj.object_id];
			EXIT_IF(h.free);

			auto& o = h.info;
			if (o.object.type == type)
			{
				EXIT_NOT_IMPLEMENTED(obj.relation != OverlapType::Equals);

				for (int vi = 0; vi < vaddr_num; vi++)
				{
					printf("ResetHash: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 ", old_hash = 0x%016" PRIx64
					       ", new_hash = 0x%016" PRIx64 "\n",
					       Core::EnumName(o.object.type).C_Str(), vaddr[vi], size[vi], o.hash[vi], new_hash);

					o.hash[vi] = new_hash;
				}
				o.gpu_update_time = get_current_time();
				g_gpu_memory_watcher->Watch(vaddr, size, vaddr_num, WatchCallback, this,
				                            reinterpret_cast<void*>(static_cast<intptr_t>(obj.object_id)));
			}
		}
	}
}

void GpuMemory::Free(GraphicContext* ctx, uint64_t vaddr, uint64_t size, bool unmap)
{
	Core::LockGuard lock(m_mutex);

	printf("Release gpu objects:\n");
	printf("\t gpu_vaddr = 0x%016" PRIx64 "\n", vaddr);
	printf("\t size   = 0x%016" PRIx64 "\n", size);

	if (unmap)
	{
		EXIT_NOT_IMPLEMENTED(!IsAllocated(vaddr, size));

		int index = 0;
		for (auto& a: m_allocated)
		{
			if (a.vaddr == vaddr && a.size == size)
			{
				m_allocated.RemoveAt(index);
				break;
			}
			index++;
		}

		EXIT_NOT_IMPLEMENTED(IsAllocated(vaddr, size));
	}

	auto object_ids = FindBlocks(&vaddr, &size, 1);

	for (const auto& obj: object_ids)
	{
		switch (obj.relation)
		{
			case OverlapType::IsContainedWithin:
			case OverlapType::Crosses: Free(ctx, obj.object_id); break;
			default: GpuMemoryDbgDump(); EXIT("unknown obj.relation: %s\n", Core::EnumName(obj.relation).C_Str());
		}
	}
}

void GpuMemory::Free(GraphicContext* ctx, int object_id)
{
	auto& h = m_objects[object_id];
	EXIT_IF(h.free);
	auto&       o     = h.info;
	const auto& block = h.block;

	EXIT_IF(o.delete_func == nullptr);

	if (o.delete_func != nullptr)
	{
		// EXIT_IF(block.free);
		for (int vi = 0; vi < block.vaddr_num; vi++)
		{
			printf("Delete: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", Core::EnumName(o.object.type).C_Str(),
			       block.vaddr[vi], block.size[vi]);
		}
		o.delete_func(ctx, o.object.obj, &o.mem);
	}
	g_gpu_memory_watcher->Stop(block.vaddr, block.size, block.vaddr_num);
	h.free = true;
	DeleteBlock(&h.block);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Vector<GpuMemory::OverlappedBlock> GpuMemory::FindBlocks(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, bool only_first)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindBlocks", profiler::colors::Green100);

	EXIT_IF(vaddr_num <= 0 || vaddr_num > VADDR_BLOCKS_MAX);
	EXIT_IF(vaddr == nullptr || size == nullptr);
	EXIT_IF(only_first && vaddr_num != 1);

	Vector<GpuMemory::OverlappedBlock> ret;

	if (vaddr_num != 1)
	{
		int index = 0;
		for (const auto& b: m_objects)
		{
			if (!b.free)
			{
				bool equal = true;
				for (int i = 0; i < vaddr_num; i++)
				{
					if (GetOverlapType(b.block.vaddr[i], b.block.size[i], vaddr[i], size[i]) != OverlapType::Equals)
					{
						equal = false;
						break;
					}
				}
				if (equal)
				{
					ret.Add({OverlapType::Equals, index});
				} else
				{
					bool cross = false;
					for (int i = 0; i < vaddr_num; i++)
					{
						for (int j = 0; j < b.block.vaddr_num; j++)
						{
							if (GetOverlapType(b.block.vaddr[j], b.block.size[j], vaddr[i], size[i]) != OverlapType::None)
							{
								cross = true;
								break;
							}
						}
						if (cross)
						{
							break;
						}
					}
					if (cross)
					{
						ret.Add({OverlapType::Crosses, index});
					}
				}
			}
			index++;
		}
	} else
	{
		int index = 0;
		for (const auto& b: m_objects)
		{
			if (!b.free)
			{
				if (b.block.vaddr_num == 1 || only_first)
				{
					auto type = GetOverlapType(b.block.vaddr[0], b.block.size[0], vaddr[0], size[0]);
					if (type != OverlapType::None)
					{
						ret.Add({type, index});
					}
				} else
				{
					for (int i = 0; i < b.block.vaddr_num; i++)
					{
						if (GetOverlapType(b.block.vaddr[i], b.block.size[i], vaddr[0], size[0]) != OverlapType::None)
						{
							ret.Add({OverlapType::Crosses, index});
							break;
						}
					}
				}
			}
			index++;
		}
	}
	return ret;
}

GpuMemory::Block GpuMemory::CreateBlock(const uint64_t* vaddr, const uint64_t* size, int vaddr_num)
{
	EXIT_IF(vaddr_num > VADDR_BLOCKS_MAX);
	EXIT_IF(vaddr == nullptr || size == nullptr);

	Block nb {};
	nb.vaddr_num = vaddr_num;
	for (int vi = 0; vi < vaddr_num; vi++)
	{
		nb.vaddr[vi] = vaddr[vi];
		nb.size[vi]  = size[vi];
		m_objects_size += size[vi];
	}
	return nb;
}

void GpuMemory::DeleteBlock(Block* b)
{
	for (int vi = 0; vi < b->vaddr_num; vi++)
	{
		m_objects_size -= b->size[vi];
	}
}

void GpuMemory::FrameDone()
{
	Core::LockGuard lock(m_mutex);

	m_current_frame++;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GpuMemory::WriteBack(GraphicContext* ctx)
{
	Core::LockGuard lock(m_mutex);

	int index = 0;
	for (auto& h: m_objects)
	{
		if (!h.free)
		{
			auto& o = h.info;
			if (o.in_use && /*o.use_last_frame >= m_current_frame &&*/ o.write_back_func != nullptr && !o.read_only)
			{
				auto& block = h.block;
				// EXIT_IF(block.free);

				g_gpu_memory_watcher->Stop(block.vaddr, block.size, block.vaddr_num);
				o.write_back_func(ctx, o.object.obj, block.vaddr, block.size, block.vaddr_num);
				o.cpu_update_time = get_current_time();

				if (!h.others.IsEmpty())
				{
					EXIT_NOT_IMPLEMENTED(h.others.Size() != 1);
					EXIT_NOT_IMPLEMENTED(h.others.At(0).relation != OverlapType::Equals);

					auto& o2 = m_objects[h.others.At(0).object_id].info;

					o2.cpu_update_time = o.cpu_update_time;

					Update(ctx, h.others.At(0).object_id);

					for (int vi = 0; vi < block.vaddr_num; vi++)
					{
						printf("WriteBack (GPU -> CPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64
						       ", old_hash = 0x%016" PRIx64 ", new_hash = 0x%016" PRIx64 "\n",
						       Core::EnumName(o.object.type).C_Str(), block.vaddr[vi], block.size[vi], o.hash[vi], o2.hash[vi]);

						o.hash[vi] = o2.hash[vi];
					}
				} else
				{
					for (int vi = 0; vi < block.vaddr_num; vi++)
					{
						uint64_t new_hash = 0;

						if (o.check_hash)
						{
							new_hash = calc_hash(reinterpret_cast<const uint8_t*>(block.vaddr[vi]), block.size[vi]);
						}

						printf("WriteBack (GPU -> CPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64
						       ", old_hash = 0x%016" PRIx64 ", new_hash = 0x%016" PRIx64 "\n",
						       Core::EnumName(o.object.type).C_Str(), block.vaddr[vi], block.size[vi], o.hash[vi], new_hash);

						o.hash[vi] = new_hash;
					}
				}

				o.in_use = false;
			}
		}
		index++;
	}
}

void GpuMemory::Flush(GraphicContext* ctx)
{
	Core::LockGuard lock(m_mutex);

	int index = 0;
	for (auto& h: m_objects)
	{
		if (!h.free)
		{
			auto& block = h.block;
			// EXIT_IF(block.free);

			auto& o = h.info;

			EXIT_IF(o.update_func == nullptr);
			o.update_func(ctx, o.params, o.object.obj, block.vaddr, block.size, block.vaddr_num);
			o.gpu_update_time = get_current_time();
			g_gpu_memory_watcher->Watch(block.vaddr, block.size, block.vaddr_num, WatchCallback, this,
			                            reinterpret_cast<void*>(static_cast<intptr_t>(index)));
		}
		index++;
	}
}

void GpuMemory::DbgInit()
{
	EXIT_IF(!m_db.IsInvalid());
	[[maybe_unused]] bool create = m_db.CreateInMemory();
	EXIT_IF(!create);
	if (!m_db.IsInvalid())
	{
		m_db.Exec(R"(
CREATE TABLE [objects](
  [dump_id] INT, 
  [id] INTEGER PRIMARY KEY UNIQUE, 
  [vaddr] TEXT, 
  [size] TEXT, 
  [vaddr2] TEXT, 
  [size2] TEXT, 
  [vaddr3] TEXT, 
  [size3] TEXT, 
  [obj] TEXT, 
  [type] TEXT, 
  [param0] INT64, 
  [param1] INT64, 
  [param2] INT64, 
  [param3] INT64, 
  [param4] INT64, 
  [param5] INT64, 
  [param6] INT64, 
  [param7] INT64, 
  [scenario] TEXT, 
  [others] TEXT, 
  [hash] TEXT, 
  [hash2] TEXT, 
  [hash3] TEXT, 
  [gpu_update_time] INT64, 
  [cpu_update_time] INT64, 
  [write_back_func] TEXT, 
  [delete_func] TEXT, 
  [update_func] TEXT, 
  [use_last_frame] INT64, 
  [use_num] INT64, 
  [in_use] BOOL, 
  [read_only] BOOL, 
  [check_hash] BOOL, 
  [vk_mem_size] TEXT, 
  [vk_mem_alignment] TEXT, 
  [vk_mem_memoryTypeBits] INT, 
  [vk_mem_property] INT, 
  [vk_mem_memory] TEXT, 
  [vk_mem_offset] INT64, 
  [vk_mem_type] INT, 
  [vk_mem_unique_id] INT64) WITHOUT ROWID;

CREATE TABLE [ranges](
  [dump_id] INT, 
  [vaddr] TEXT, 
  [size] TEXT);
)");

		m_db_add_range  = m_db.Prepare("insert into ranges(dump_id, vaddr, size) values(:dump_id, :vaddr, :size)");
		m_db_add_object = m_db.Prepare(
		    "insert into objects(dump_id, id, vaddr, vaddr2, vaddr3, size, size2, size3, obj, param0, param1, param2, param3, param4, "
		    "param5, param6, param7, type, hash, hash2, "
		    "hash3, gpu_update_time, cpu_update_time, scenario, others, write_back_func, delete_func, update_func, use_last_frame, "
		    "use_num, in_use, read_only, "
		    "check_hash, vk_mem_size, "
		    "vk_mem_alignment, vk_mem_memoryTypeBits, vk_mem_property, vk_mem_memory, vk_mem_offset, vk_mem_type, vk_mem_unique_id) "
		    "values(:dump_id, :id, :vaddr, :vaddr2, :vaddr3, :size, :size2, :size3, :obj, :param0, :param1, :param2, :param3, :param4, "
		    ":param5, "
		    ":param6, :param7, :type, :hash, "
		    ":hash2, :hash3, :gpu_update_time, :cpu_update_time, :scenario, :others, :write_back_func, :delete_func, :update_func, "
		    ":use_last_frame, :use_num, :in_use, "
		    ":read_only, :check_hash, "
		    ":vk_mem_size, :vk_mem_alignment, :vk_mem_memoryTypeBits, :vk_mem_property, :vk_mem_memory, :vk_mem_offset, :vk_mem_type, "
		    ":vk_mem_unique_id)");
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GpuMemory::DbgDbDump()
{
	KYTY_PROFILER_FUNCTION();

	Core::LockGuard lock(m_mutex);

	static int dump_id = 0;

	auto hex = [](auto u)
	{
		auto u64 = reinterpret_cast<uint64_t>(u);
		return (u64 == 0 ? U"0" : String::FromPrintf("0x%010" PRIx64, u64));
	};

	auto id = [](int id1, int id2) { return id1 * 1000000 + id2; };

	if (!m_db.IsInvalid())
	{
		m_db.Exec("BEGIN TRANSACTION");

		for (const auto& r: m_allocated)
		{
			m_db_add_range->Reset();
			m_db_add_range->BindInt(":dump_id", dump_id);
			m_db_add_range->BindString(":vaddr", hex(r.vaddr));
			m_db_add_range->BindString(":size", hex(r.size));
			m_db_add_range->Step();
		}

		int index = 0;
		for (const auto& r: m_objects)
		{
			if (!r.free)
			{
				m_db_add_object->Reset();
				m_db_add_object->BindInt(":dump_id", dump_id);
				m_db_add_object->BindInt(":id", id(dump_id, index));
				(r.block.vaddr_num > 0 ? m_db_add_object->BindString(":vaddr", hex(r.block.vaddr[0]))
				                       : m_db_add_object->BindNull(":vaddr"));
				(r.block.vaddr_num > 1 ? m_db_add_object->BindString(":vaddr2", hex(r.block.vaddr[1]))
				                       : m_db_add_object->BindNull(":vaddr2"));
				(r.block.vaddr_num > 2 ? m_db_add_object->BindString(":vaddr3", hex(r.block.vaddr[2]))
				                       : m_db_add_object->BindNull(":vaddr3"));
				(r.block.vaddr_num > 0 ? m_db_add_object->BindString(":size", hex(r.block.size[0])) : m_db_add_object->BindNull(":size"));
				(r.block.vaddr_num > 1 ? m_db_add_object->BindString(":size2", hex(r.block.size[1])) : m_db_add_object->BindNull(":size2"));
				(r.block.vaddr_num > 2 ? m_db_add_object->BindString(":size3", hex(r.block.size[2])) : m_db_add_object->BindNull(":size3"));
				m_db_add_object->BindString(":obj", hex(r.info.object.obj));
				int param0_index = m_db_add_object->GetIndex(":param0");
				for (int i = 0; i < GpuObject::PARAMS_MAX; i++)
				{
					m_db_add_object->BindInt64(param0_index + i, static_cast<int64_t>(r.info.params[i]));
				}
				m_db_add_object->BindString(":type", Core::EnumName(r.info.object.type).C_Str());
				int hash0_index = m_db_add_object->GetIndex(":hash");
				for (int i = 0; i < VADDR_BLOCKS_MAX; i++)
				{
					m_db_add_object->BindString(hash0_index + i, hex(r.info.hash[i]));
				}
				m_db_add_object->BindString(":write_back_func", hex(r.info.write_back_func));
				m_db_add_object->BindString(":delete_func", hex(r.info.delete_func));
				m_db_add_object->BindString(":update_func", hex(r.info.update_func));
				m_db_add_object->BindInt64(":use_last_frame", static_cast<int64_t>(r.info.use_last_frame));
				m_db_add_object->BindInt64(":use_num", static_cast<int64_t>(r.info.use_num));
				m_db_add_object->BindInt(":in_use", static_cast<int>(r.info.in_use));
				m_db_add_object->BindInt(":read_only", static_cast<int>(r.info.read_only));
				m_db_add_object->BindInt(":check_hash", static_cast<int>(r.info.check_hash));
				m_db_add_object->BindString(":vk_mem_size", hex(r.info.mem.requirements.size));
				m_db_add_object->BindString(":vk_mem_alignment", hex(r.info.mem.requirements.alignment));
				m_db_add_object->BindInt(":vk_mem_memoryTypeBits", static_cast<int>(r.info.mem.requirements.memoryTypeBits));
				m_db_add_object->BindInt(":vk_mem_property", static_cast<int>(r.info.mem.property));
				m_db_add_object->BindString(":vk_mem_memory", hex(r.info.mem.memory));
				m_db_add_object->BindInt64(":vk_mem_offset", static_cast<int64_t>(r.info.mem.offset));
				m_db_add_object->BindInt(":vk_mem_type", static_cast<int>(r.info.mem.type));
				m_db_add_object->BindInt64(":vk_mem_unique_id", static_cast<int64_t>(r.info.mem.unique_id));
				m_db_add_object->BindString(":scenario", Core::EnumName(r.scenario).C_Str());
				m_db_add_object->BindInt64(":gpu_update_time", static_cast<int64_t>(r.info.gpu_update_time));
				m_db_add_object->BindInt64(":cpu_update_time", static_cast<int64_t>(r.info.cpu_update_time));

				if (r.others.Size() > 0)
				{
					Core::StringList others;
					for (const auto& s: r.others)
					{
						others.Add(String::FromPrintf("[%s,%d]", Core::EnumName(s.relation).C_Str(), id(dump_id, s.object_id)));
					}
					m_db_add_object->BindString(":others", others.Concat(U','));
				} else
				{
					m_db_add_object->BindNull(":others");
				}

				m_db_add_object->Step();
			}
			index++;
		}

		m_db.Exec("END TRANSACTION");
	}

	dump_id++;
}

void GpuMemory::DbgDbSave(const String& file_name)
{
	KYTY_PROFILER_FUNCTION();

	Core::LockGuard lock(m_mutex);

	if (!m_db.IsInvalid())
	{
		Core::Database::Connection db;
		if (!db.Create(file_name) && !db.Open(file_name, Core::Database::Connection::Mode::ReadWrite))
		{
			printf("Can't open file: %s\n", file_name.C_Str());
			return;
		}
		m_db.CopyTo(&db);
		db.Close();
	}
}

void GpuMemoryInit()
{
	EXIT_IF(g_gpu_memory != nullptr);
	EXIT_IF(g_gpu_resources != nullptr);
	EXIT_IF(g_gpu_memory_watcher != nullptr);

	g_gpu_memory         = new GpuMemory;
	g_gpu_resources      = new GpuResources;
	g_gpu_memory_watcher = new MemoryWatcher;
}

void GpuMemorySetAllocatedRange(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->SetAllocatedRange(vaddr, size);
}

void GpuMemoryFree(GraphicContext* ctx, uint64_t vaddr, uint64_t size, bool unmap)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	g_gpu_memory->Free(ctx, vaddr, size, unmap);
}

void* GpuMemoryCreateObject(GraphicContext* ctx, CommandBuffer* buffer, uint64_t vaddr, uint64_t size, const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->CreateObject(ctx, buffer, &vaddr, &size, 1, info);
}

void* GpuMemoryCreateObject(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                            const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->CreateObject(ctx, buffer, vaddr, size, vaddr_num, info);
}

Vector<GpuMemoryObject> GpuMemoryFindObjects(uint64_t vaddr, uint64_t size, bool exact, bool only_first)
{
	EXIT_IF(g_gpu_memory == nullptr);

	return g_gpu_memory->FindObjects(&vaddr, &size, 1, exact, only_first);
}

void GpuMemoryResetHash(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	g_gpu_memory->ResetHash(ctx, vaddr, size, vaddr_num, type);
}

void GpuMemoryDbgDump()
{
	EXIT_IF(g_gpu_memory == nullptr);

	// g_gpu_memory->DbgDbDump();
	// g_gpu_memory->DbgDbSave(U"_gpu_memory.db");
}

void GpuMemoryFlush(GraphicContext* ctx)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	// update vulkan objects after CPU-drawing
	g_gpu_memory->Flush(ctx);
}

void GpuMemoryFrameDone()
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->FrameDone();
}

void GpuMemoryWriteBack(GraphicContext* ctx)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	// update CPU memory after GPU-drawing
	g_gpu_memory->WriteBack(ctx);
}

bool GpuMemoryCheckAccessViolation(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(g_gpu_memory_watcher == nullptr);

	return g_gpu_memory_watcher->Check(vaddr, size);
}

bool GpuMemoryWatcherEnabled()
{
	return MemoryWatcher::Enabled();
}

bool VulkanAllocate(GraphicContext* ctx, VulkanMemory* mem)
{
	static std::atomic<uint64_t> seq = 0;

	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(mem->memory != nullptr);
	EXIT_IF(mem->requirements.size == 0);

	VkPhysicalDeviceMemoryProperties memory_properties {};
	vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &memory_properties);

	uint32_t index = 0;
	for (; index < memory_properties.memoryTypeCount; index++)
	{
		if ((mem->requirements.memoryTypeBits & (static_cast<uint32_t>(1) << index)) != 0 &&
		    (memory_properties.memoryTypes[index].propertyFlags & mem->property) == mem->property)
		{
			break;
		}
	}

	mem->type   = index;
	mem->offset = 0;

	VkMemoryAllocateInfo alloc_info {};
	alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext           = nullptr;
	alloc_info.allocationSize  = mem->requirements.size;
	alloc_info.memoryTypeIndex = index;

	mem->unique_id = ++seq;

	return (vkAllocateMemory(ctx->device, &alloc_info, nullptr, &mem->memory) == VK_SUCCESS);
}

void VulkanFree(GraphicContext* ctx, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);

	vkFreeMemory(ctx->device, mem->memory, nullptr);

	mem->memory = nullptr;
}

void VulkanMapMemory(GraphicContext* ctx, VulkanMemory* mem, void** data)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(data == nullptr);

	vkMapMemory(ctx->device, mem->memory, mem->offset, mem->requirements.size, 0, data);
}

void VulkanUnmapMemory(GraphicContext* ctx, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);

	vkUnmapMemory(ctx->device, mem->memory);
}

void VulkanBindImageMemory(GraphicContext* ctx, VulkanImage* image, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(image == nullptr);

	vkBindImageMemory(ctx->device, image->image, mem->memory, mem->offset);
}

void VulkanBindBufferMemory(GraphicContext* ctx, VulkanBuffer* buffer, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(buffer == nullptr);

	vkBindBufferMemory(ctx->device, buffer->buffer, mem->memory, mem->offset);
}

void GpuMemoryRegisterOwner(uint32_t* owner_handle, const char* name)
{
	EXIT_IF(g_gpu_resources == nullptr);
	EXIT_IF(owner_handle == nullptr);
	EXIT_IF(name == nullptr);

	*owner_handle = g_gpu_resources->AddOwner(String::FromUtf8(name));
}

void GpuMemoryRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size, const char* name,
                               uint32_t type, uint64_t user_data)
{
	EXIT_IF(g_gpu_resources == nullptr);
	EXIT_IF(resource_handle == nullptr);
	EXIT_IF(name == nullptr);

	*resource_handle =
	    g_gpu_resources->AddResource(owner_handle, reinterpret_cast<uint64_t>(memory), size, String::FromUtf8(name), type, user_data);
}

void GpuMemoryUnregisterAllResourcesForOwner(uint32_t owner_handle)
{
	EXIT_IF(g_gpu_resources == nullptr);

	g_gpu_resources->DeleteResources(owner_handle);
}

void GpuMemoryUnregisterOwnerAndResources(uint32_t owner_handle)
{
	EXIT_IF(g_gpu_resources == nullptr);

	g_gpu_resources->DeleteOwner(owner_handle);
}

void GpuMemoryUnregisterResource(uint32_t resource_handle)
{
	EXIT_IF(g_gpu_resources == nullptr);

	g_gpu_resources->DeleteResource(resource_handle);
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
