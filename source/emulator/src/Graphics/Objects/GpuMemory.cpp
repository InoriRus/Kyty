#include "Emulator/Graphics/Objects/GpuMemory.h"

#include "Kyty/Core/Database.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Profiler.h"

#include <algorithm>
#include <atomic>
#include <vulkan/vk_enum_string_helper.h>

//#define XXH_INLINE_ALL
#include <xxhash/xxh3.h>

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

	bool a_b  = addr_in_block(vaddr_a, size_a, vaddr_b);
	bool a_lb = addr_in_block(vaddr_a, size_a, vaddr_b + size_b - 1);
	bool b_a  = addr_in_block(vaddr_b, size_b, vaddr_a);
	bool b_la = addr_in_block(vaddr_b, size_b, vaddr_a + size_a - 1);

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

class GpuMap1
{
public:
	GpuMap1()  = default;
	~GpuMap1() = default;

	KYTY_CLASS_NO_COPY(GpuMap1);

	void Insert(uint64_t vaddr, int id)
	{
		auto& ids = m_map[vaddr];
		if (!ids.Contains(id))
		{
			ids.Add(id);
		}
	}

	void Erase(uint64_t vaddr, int id)
	{
		auto& ids = m_map[vaddr];
		ids.Remove(id);
		if (ids.IsEmpty())
		{
			m_map.Remove(vaddr);
		}
	}

	[[nodiscard]] Vector<int> FindAll(uint64_t vaddr) const { return m_map.Get(vaddr); }

	[[nodiscard]] bool IsEmpty() const
	{
		int num = 0;
		m_map.ForEach(
		    [](auto /*key*/, auto value, void* arg)
		    {
			    (*static_cast<int*>(arg)) += value->Size();
			    return true;
		    },
		    &num);
		return num == 0;
	}

private:
	Core::Hashmap<uint64_t, Vector<int>> m_map;
};

class GpuMap2
{
public:
	GpuMap2()  = default;
	~GpuMap2() = default;

	KYTY_CLASS_NO_COPY(GpuMap2);

	void Insert(uint64_t vaddr, uint64_t size, int id)
	{
		EXIT_IF(size == 0);
		auto first_page = CalcPageId(vaddr);
		auto last_page  = CalcPageId(vaddr + size - 1);
		EXIT_IF(last_page < first_page);
		for (auto page = first_page; page <= last_page; page++)
		{
			auto& ids = m_map[page];
			if (!ids.Contains(id))
			{
				ids.Add(id);
			}
		}
	}

	void Erase(uint64_t vaddr, uint64_t size, int id)
	{
		EXIT_IF(size == 0);
		auto first_page = CalcPageId(vaddr);
		auto last_page  = CalcPageId(vaddr + size - 1);
		EXIT_IF(last_page < first_page);
		for (auto page = first_page; page <= last_page; page++)
		{
			auto& ids = m_map[page];
			ids.Remove(id);
			if (ids.IsEmpty())
			{
				m_map.Remove(page);
			}
		}
	}

	[[nodiscard]] Vector<int> FindAll(uint64_t vaddr, uint64_t size) const
	{
		Vector<int> ret;
		EXIT_IF(size == 0);
		auto first_page = CalcPageId(vaddr);
		auto last_page  = CalcPageId(vaddr + size - 1);
		EXIT_IF(last_page < first_page);
		for (auto page = first_page; page <= last_page; page++)
		{
			for (int id: m_map.Get(page))
			{
				if (!ret.Contains(id))
				{
					ret.Add(id);
				}
			}
		}
		return ret;
	}

	[[nodiscard]] Vector<int> FindAll(const uint64_t* vaddr, const uint64_t* size, int vaddr_num) const
	{
		EXIT_IF(vaddr == nullptr);
		EXIT_IF(size == nullptr);
		Vector<int> ret;
		for (int i = 0; i < vaddr_num; i++)
		{
			EXIT_IF(size[i] == 0);
			auto first_page = CalcPageId(vaddr[i]);
			auto last_page  = CalcPageId(vaddr[i] + size[i] - 1);
			EXIT_IF(last_page < first_page);
			for (auto page = first_page; page <= last_page; page++)
			{
				for (int id: m_map.Get(page))
				{
					if (!ret.Contains(id))
					{
						ret.Add(id);
					}
				}
			}
		}
		return ret;
	}

	[[nodiscard]] bool IsEmpty() const
	{
		int num = 0;
		m_map.ForEach(
		    [](auto /*key*/, auto value, void* arg)
		    {
			    (*static_cast<int*>(arg)) += value->Size();
			    return true;
		    },
		    &num);
		return num == 0;
	}

private:
	static constexpr uint32_t PAGE_BITS = 14u;

	static uint32_t CalcPageId(uint64_t vaddr)
	{
		EXIT_IF((vaddr >> (PAGE_BITS + 32u)) != 0);
		return static_cast<uint32_t>(vaddr >> PAGE_BITS);
	}
	Core::Hashmap<uint32_t, Vector<int>> m_map;
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

	void* CreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size,
	                   int vaddr_num, const GpuObject& info);
	void  ResetHash(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type);
	void  FrameDone();

	Vector<GpuMemoryObject> FindObjects(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type, bool exact,
	                                    bool only_first);

	// Sync: GPU -> CPU
	void WriteBack(GraphicContext* ctx, CommandProcessor* cp);

	// Sync: CPU -> GPU
	void Flush(GraphicContext* ctx, uint64_t vaddr, uint64_t size);
	void FlushAll(GraphicContext* ctx);

	void DbgInit();
	void DbgDbDump();
	void DbgDbSave(const String& file_name);

private:
	static constexpr int OBJ_OVERLAPS_MAX = 2;

	struct AllocatedRange
	{
		uint64_t vaddr = 0;
		uint64_t size  = 0;
	};

	struct ObjectInfo
	{
		GpuMemoryObject              object;
		uint64_t                     params[GpuObject::PARAMS_MAX] = {};
		uint64_t                     hash[VADDR_BLOCKS_MAX]        = {};
		uint64_t                     cpu_update_time               = 0;
		uint64_t                     gpu_update_time               = 0;
		uint64_t                     submit_id                     = 0;
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
		GpuMemoryScenario       scenario     = GpuMemoryScenario::Common;
		bool                    free         = true;
		int                     next_free_id = -1;
	};

	struct Heap
	{
		AllocatedRange range;
		Vector<Object> objects;
		uint64_t       objects_size  = 0;
		int            first_free_id = -1;
		GpuMap1*       objects_map1  = nullptr;
		GpuMap2*       objects_map2  = nullptr;
	};

	struct Destructor
	{
		void*                    obj         = nullptr;
		GpuObject::delete_func_t delete_func = nullptr;
		VulkanMemory             mem;
	};

	[[nodiscard]] Destructor Free(int heap_id, int object_id);

	Vector<OverlappedBlock> FindBlocks_slow(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
	                                        bool only_first = false);
	Vector<OverlappedBlock> FindBlocks(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, bool only_first = false);
	bool  FindFast(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type, bool only_first,
	               int* id);
	Block CreateBlock(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, int heap_id, int obj_id);
	void  DeleteBlock(Block* b, int heap_id, int obj_id);
	void  Link(int heap_id, int id1, int id2, OverlapType rel, GpuMemoryScenario scenario);
	int   GetHeapId(uint64_t vaddr, uint64_t size);

	// Update (CPU -> GPU)
	void Update(uint64_t submit_id, GraphicContext* ctx, int heap_id, int obj_id);

	bool create_existing(const Vector<OverlappedBlock>& others, const GpuObject& info, int heap_id, int* id);
	bool create_generate_mips(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id);
	bool create_texture_triplet(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id);
	bool create_maybe_deleted(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id);
	bool create_all_the_same(const Vector<OverlappedBlock>& others, int heap_id);

	[[nodiscard]] String create_dbg_exit(const String& msg, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
	                                     const Vector<OverlappedBlock>& others, GpuMemoryObjectType type);

	Core::Mutex m_mutex;

	Vector<Heap> m_heaps;

	uint64_t m_current_frame = 0;

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

static GpuMemory*    g_gpu_memory    = nullptr;
static GpuResources* g_gpu_resources = nullptr;

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

	Heap h;
	h.range.vaddr  = vaddr;
	h.range.size   = size;
	h.objects_map1 = new GpuMap1;
	h.objects_map2 = new GpuMap2;

	m_heaps.Add(h);
}

bool GpuMemory::IsAllocated(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(size == 0);

	Core::LockGuard lock(m_mutex);

	return (GetHeapId(vaddr, size) >= 0);
}

int GpuMemory::GetHeapId(uint64_t vaddr, uint64_t size)
{
	int index = 0;
	for (const auto& heap: m_heaps)
	{
		const auto& r = heap.range;
		if ((vaddr >= r.vaddr && vaddr < r.vaddr + r.size) || ((vaddr + size - 1) >= r.vaddr && (vaddr + size - 1) < r.vaddr + r.size))
		{
			return index;
		}
		index++;
	}
	return -1;
}

static uint64_t calc_hash(const uint8_t* buf, uint64_t size)
{
	KYTY_PROFILER_FUNCTION();

	return (size > 0 && buf != nullptr ? XXH3_64bits(buf, size) : 0);
}

static uint64_t get_current_time()
{
	static std::atomic_uint64_t t(0);
	return ++t;
}

void GpuMemory::Link(int heap_id, int id1, int id2, OverlapType rel, GpuMemoryScenario scenario)
{
	OverlapType other_rel = OverlapType::None;
	switch (rel)
	{
		case OverlapType::Equals: other_rel = OverlapType::Equals; break;
		case OverlapType::IsContainedWithin: other_rel = OverlapType::Contains; break;
		case OverlapType::Contains: other_rel = OverlapType::IsContainedWithin; break;
		default: EXIT("invalid rel: %s\n", Core::EnumName(rel).C_Str());
	}

	auto& heap = m_heaps[heap_id];

	auto& h1 = heap.objects[id1];
	EXIT_IF(h1.free);

	auto& h2 = heap.objects[id2];
	EXIT_IF(h2.free);

	h1.others.Add({rel, id2});
	h2.others.Add({other_rel, id1});

	h1.scenario = scenario;
	h2.scenario = scenario;
}

void GpuMemory::Update(uint64_t submit_id, GraphicContext* ctx, int heap_id, int obj_id)
{
	KYTY_PROFILER_BLOCK("GpuMemory::Update");

	auto& heap = m_heaps[heap_id];

	auto& h           = heap.objects[obj_id];
	auto& o           = h.info;
	bool  need_update = false;

	bool mem_watch = false;

	if ((mem_watch && o.cpu_update_time > o.gpu_update_time) || (!mem_watch && submit_id > o.submit_id))
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

		if (submit_id != UINT64_MAX)
		{
			o.submit_id = submit_id;
		}
	}

	if (need_update)
	{
		EXIT_IF(o.update_func == nullptr);
		o.update_func(ctx, o.params, o.object.obj, h.block.vaddr, h.block.size, h.block.vaddr_num);
		o.gpu_update_time = get_current_time();
	}
}

bool GpuMemory::create_existing(const Vector<OverlappedBlock>& others, const GpuObject& info, int heap_id, int* id)
{
	EXIT_IF(id == nullptr);

	auto& heap = m_heaps[heap_id];

	uint64_t               max_gpu_update_time = 0;
	const OverlappedBlock* latest_block        = nullptr;

	for (const auto& obj: others)
	{
		auto& h = heap.objects[obj.object_id];
		EXIT_IF(h.free);
		auto& o = h.info;

		if (h.scenario == GpuMemoryScenario::Common && obj.relation == OverlapType::Equals && o.object.type == info.type &&
		    info.Equal(o.params))
		{
			*id = obj.object_id;
			return true;
		}

		//		if (h.scenario == GpuMemoryScenario::Common && obj.relation == OverlapType::Contains && o.object.type == info.type &&
		//		    info.Reuse(o.params))
		//		{
		//			*id = obj.object_id;
		//			return true;
		//		}

		if (o.gpu_update_time > max_gpu_update_time)
		{
			max_gpu_update_time = o.gpu_update_time;
			latest_block        = &obj;
		}
	}

	if (latest_block != nullptr)
	{
		auto& h = heap.objects[latest_block->object_id];
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

bool GpuMemory::create_generate_mips(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id)
{
	auto& heap = m_heaps[heap_id];

	if (others.Size() == 3 && type == GpuMemoryObjectType::RenderTexture)
	{
		const auto&         b0    = others.At(0);
		const auto&         b1    = others.At(1);
		const auto&         b2    = others.At(2);
		OverlapType         rel0  = b0.relation;
		OverlapType         rel1  = b1.relation;
		OverlapType         rel2  = b2.relation;
		const auto&         o0    = heap.objects[b0.object_id];
		const auto&         o1    = heap.objects[b1.object_id];
		const auto&         o2    = heap.objects[b2.object_id];
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
		const auto&         o0    = heap.objects[b0.object_id];
		const auto&         o1    = heap.objects[b1.object_id];
		const auto&         o2    = heap.objects[b2.object_id];
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

bool GpuMemory::create_texture_triplet(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id)
{
	auto& heap = m_heaps[heap_id];

	if (others.Size() == 2 && type == GpuMemoryObjectType::StorageTexture)
	{
		const auto&         b0    = others.At(0);
		const auto&         b1    = others.At(1);
		OverlapType         rel0  = b0.relation;
		OverlapType         rel1  = b1.relation;
		const auto&         o0    = heap.objects[b0.object_id];
		const auto&         o1    = heap.objects[b1.object_id];
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

bool GpuMemory::create_maybe_deleted(const Vector<OverlappedBlock>& others, GpuMemoryObjectType type, int heap_id)
{
	auto& heap = m_heaps[heap_id];

	if (type == GpuMemoryObjectType::VertexBuffer || type == GpuMemoryObjectType::IndexBuffer)
	{
		return std::all_of(others.begin(), others.end(),
		                   [heap](auto& r)
		                   {
			                   OverlapType         rel    = r.relation;
			                   const auto&         o      = heap.objects[r.object_id];
			                   GpuMemoryObjectType o_type = o.info.object.type;
			                   return ((rel == OverlapType::IsContainedWithin || rel == OverlapType::Crosses) &&
			                           (o_type == GpuMemoryObjectType::VertexBuffer || o_type == GpuMemoryObjectType::IndexBuffer));
		                   });
	}
	if (type == GpuMemoryObjectType::Texture)
	{
		return std::all_of(others.begin(), others.end(),
		                   [heap](auto& r)
		                   {
			                   OverlapType         rel    = r.relation;
			                   const auto&         o      = heap.objects[r.object_id];
			                   GpuMemoryObjectType o_type = o.info.object.type;
			                   return ((rel == OverlapType::IsContainedWithin || rel == OverlapType::Crosses) &&
			                           o_type == GpuMemoryObjectType::Texture);
		                   });
	}
	if (type == GpuMemoryObjectType::RenderTexture)
	{
		return std::all_of(others.begin(), others.end(),
		                   [heap](auto& r)
		                   {
			                   OverlapType         rel    = r.relation;
			                   const auto&         o      = heap.objects[r.object_id];
			                   GpuMemoryObjectType o_type = o.info.object.type;
			                   return ((rel == OverlapType::IsContainedWithin || rel == OverlapType::Crosses) &&
			                           (o_type == GpuMemoryObjectType::RenderTexture || o_type == GpuMemoryObjectType::DepthStencilBuffer));
		                   });
	}
	return false;
}

bool GpuMemory::create_all_the_same(const Vector<OverlappedBlock>& others, int heap_id)
{
	auto&               heap = m_heaps[heap_id];
	OverlapType         rel  = others.At(0).relation;
	GpuMemoryObjectType type = heap.objects[others.At(0).object_id].info.object.type;

	return std::all_of(others.begin(), others.end(),
	                   [rel, type, heap](auto& r) { return (rel == r.relation && type == heap.objects[r.object_id].info.object.type); });
}

String GpuMemory::create_dbg_exit(const String& msg, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                                  const Vector<OverlappedBlock>& others, GpuMemoryObjectType type)
{
	Core::StringList list;
	list.Add(String::FromPrintf("Exit:"));
	list.Add(msg);
	for (int vi = 0; vi < vaddr_num; vi++)
	{
		list.Add(String::FromPrintf("\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "", vaddr[vi], size[vi]));
	}

	list.Add(String::FromPrintf("\t info.type = %s", Core::EnumName(type).C_Str()));
	for (const auto& d: others)
	{
		list.Add(String::FromPrintf("\t id = %d, rel = %s", d.object_id, Core::EnumName(d.relation).C_Str()));
	}
	DbgDbDump();
	DbgDbSave(U"_gpu_memory.db");
	auto str = list.Concat(U'\n');
	printf("%s\n", str.C_Str());
	return str;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void* GpuMemory::CreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size,
                              int vaddr_num, const GpuObject& info)
{
	KYTY_PROFILER_BLOCK("GpuMemory::CreateObject", profiler::colors::Green300);

	EXIT_IF(info.type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	int heap_id = GetHeapId(vaddr[0], size[0]);

	EXIT_NOT_IMPLEMENTED(heap_id < 0);

	auto& heap = m_heaps[heap_id];

	bool overlap             = false;
	bool delete_all          = false;
	bool create_from_objects = false;

	GpuMemoryScenario scenario = GpuMemoryScenario::Common;

	int fast_id = -1;
	if (FindFast(heap_id, vaddr, size, vaddr_num, info.type, false, &fast_id))
	{
		auto& h = heap.objects[fast_id];
		EXIT_IF(h.free);
		auto& o = h.info;

		if (h.scenario == GpuMemoryScenario::Common && info.Equal(o.params))
		{
			Update(submit_id, ctx, heap_id, fast_id);

			o.use_num++;
			o.use_last_frame = m_current_frame;
			o.in_use         = true;
			o.read_only      = info.read_only;
			o.check_hash     = info.check_hash;

			return o.object.obj;
		}
	}

	auto others = FindBlocks(heap_id, vaddr, size, vaddr_num);

	if (!others.IsEmpty())
	{
		int existing_id = -1;
		if (create_existing(others, info, heap_id, &existing_id))
		{
			auto& h = heap.objects[existing_id];
			EXIT_IF(h.free);
			auto& o = h.info;

			Update(submit_id, ctx, heap_id, existing_id);

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
			auto&       h   = heap.objects[obj.object_id];
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
				case ObjectsRelation(GpuMemoryObjectType::DepthStencilBuffer, OverlapType::Contains,
				                     GpuMemoryObjectType::DepthStencilBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::Crosses, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::Contains, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::IsContainedWithin, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::Crosses, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::Contains, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::IndexBuffer, OverlapType::IsContainedWithin, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::Crosses, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::Contains, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::IsContainedWithin, GpuMemoryObjectType::VertexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::Crosses, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::Contains, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::VertexBuffer, OverlapType::IsContainedWithin, GpuMemoryObjectType::IndexBuffer):
				case ObjectsRelation(GpuMemoryObjectType::Texture, OverlapType::Crosses, GpuMemoryObjectType::Texture):
				case ObjectsRelation(GpuMemoryObjectType::Texture, OverlapType::Contains, GpuMemoryObjectType::Texture):
				case ObjectsRelation(GpuMemoryObjectType::Texture, OverlapType::IsContainedWithin, GpuMemoryObjectType::Texture):
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
				{
					auto msg = String::FromPrintf("unknown relation: %s - %s - %s\n", Core::EnumName(o.object.type).C_Str(),
					                              Core::EnumName(obj.relation).C_Str(), Core::EnumName(info.type).C_Str());
					EXIT("%s\n", create_dbg_exit(msg, vaddr, size, vaddr_num, others, info.type).C_Str());
				}
			}
		} else
		{
			if (create_generate_mips(others, info.type, heap_id))
			{
				overlap             = true;
				create_from_objects = true;
				scenario            = GpuMemoryScenario::GenerateMips;
			} else if (create_texture_triplet(others, info.type, heap_id))
			{
				overlap  = true;
				scenario = GpuMemoryScenario::TextureTriplet;
			} else if (create_maybe_deleted(others, info.type, heap_id))
			{
				delete_all = true;
			} else
			{
				if (!create_all_the_same(others, heap_id))
				{
					EXIT("%s\n", create_dbg_exit(U"!create_all_the_same", vaddr, size, vaddr_num, others, info.type).C_Str());
				}

				OverlapType         rel  = others.At(0).relation;
				GpuMemoryObjectType type = heap.objects[others.At(0).object_id].info.object.type;

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
					{
						auto msg = String::FromPrintf("unknown relation: %s - %s - %s\n", Core::EnumName(type).C_Str(),
						                              Core::EnumName(rel).C_Str(), Core::EnumName(info.type).C_Str());
						EXIT("%s\n", create_dbg_exit(msg, vaddr, size, vaddr_num, others, info.type).C_Str());
					}
				}
			}
		}
	}

	EXIT_IF(delete_all && overlap);

	Vector<Destructor> destructors;

	if (delete_all)
	{
		for (const auto& obj: others)
		{
			destructors.Add(Free(heap_id, obj.object_id));
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
	o.submit_id       = submit_id;

	if (create_from_objects)
	{
		Vector<GpuMemoryObject> objects;
		for (const auto& obj: others)
		{
			const auto& o2 = heap.objects[obj.object_id].info;
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

	int index = 0;

	if (heap.first_free_id != -1)
	{
		index              = heap.first_free_id;
		auto& u            = heap.objects[heap.first_free_id];
		heap.first_free_id = u.next_free_id;
		u.free             = false;
		u.block            = CreateBlock(vaddr, size, vaddr_num, heap_id, index);
		u.info             = o;
		u.others.Clear();
		u.scenario = scenario;
	} else
	{
		index = static_cast<int>(heap.objects.Size());

		Object h {};
		h.block = CreateBlock(vaddr, size, vaddr_num, heap_id, index);
		h.info  = o;
		h.others.Clear();
		h.scenario = scenario;
		h.free     = false;
		heap.objects.Add(h);
	}

	if (overlap)
	{
		for (const auto& obj: others)
		{
			Link(heap_id, index, obj.object_id, obj.relation, scenario);
		}
	}

	m_mutex.Unlock();
	for (auto& d: destructors)
	{
		d.delete_func(ctx, d.obj, &d.mem);
	}
	m_mutex.Lock();

	return o.object.obj;
}

Vector<GpuMemoryObject> GpuMemory::FindObjects(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type,
                                               bool exact, bool only_first)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindObjects", profiler::colors::Green200);

	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	Vector<GpuMemoryObject> ret;

	int heap_id = GetHeapId(vaddr[0], size[0]);

	EXIT_NOT_IMPLEMENTED(heap_id < 0);

	auto& heap = m_heaps[heap_id];

	if (exact)
	{
		int fast_id = -1;
		if (FindFast(heap_id, vaddr, size, vaddr_num, type, only_first, &fast_id))
		{
			const auto& h = heap.objects[fast_id];
			EXIT_IF(h.free);
			ret.Add(h.info.object);
		}
		return ret;
	}

	auto objects = FindBlocks(heap_id, vaddr, size, vaddr_num, only_first);

	for (const auto& obj: objects)
	{
		const auto& h = heap.objects[obj.object_id];
		EXIT_IF(h.free);
		if (h.info.object.type == type &&
		    (obj.relation == OverlapType::Equals || (!exact && obj.relation == OverlapType::IsContainedWithin)))
		{
			ret.Add(h.info.object);
		}
	}

	return ret;
}

void GpuMemory::ResetHash(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type)
{
	EXIT_IF(type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	int heap_id = GetHeapId(vaddr[0], size[0]);

	EXIT_NOT_IMPLEMENTED(heap_id < 0);

	auto& heap = m_heaps[heap_id];

	uint64_t new_hash = 0;

	int fast_id = -1;
	if (FindFast(heap_id, vaddr, size, vaddr_num, type, false, &fast_id))
	{
		auto& h = heap.objects[fast_id];
		EXIT_IF(h.free);
		auto& o = h.info;

		if (h.scenario == GpuMemoryScenario::Common)
		{
			for (int vi = 0; vi < vaddr_num; vi++)
			{
				printf("ResetHash: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 ", old_hash = 0x%016" PRIx64
				       ", new_hash = 0x%016" PRIx64 "\n",
				       Core::EnumName(o.object.type).C_Str(), vaddr[vi], size[vi], o.hash[vi], new_hash);
			}
			o.gpu_update_time = get_current_time();

			return;
		}
	}

	auto object_ids = FindBlocks(heap_id, vaddr, size, vaddr_num);

	if (!object_ids.IsEmpty())
	{
		for (const auto& obj: object_ids)
		{
			auto& h = heap.objects[obj.object_id];
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
				}
				o.gpu_update_time = get_current_time();
			}
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GpuMemory::Free(GraphicContext* ctx, uint64_t vaddr, uint64_t size, bool unmap)
{
	KYTY_PROFILER_BLOCK("GpuMemory::Free", profiler::colors::Green300);

	m_mutex.Lock();

	printf("Release gpu objects:\n");
	printf("\t gpu_vaddr = 0x%016" PRIx64 "\n", vaddr);
	printf("\t size   = 0x%016" PRIx64 "\n", size);

	int heap_id = GetHeapId(vaddr, size);

	EXIT_NOT_IMPLEMENTED(heap_id < 0);

	auto object_ids = FindBlocks(heap_id, &vaddr, &size, 1);

	Vector<Destructor> destructors;

	for (const auto& obj: object_ids)
	{
		switch (obj.relation)
		{
			case OverlapType::IsContainedWithin:
			case OverlapType::Crosses: destructors.Add(Free(heap_id, obj.object_id)); break;
			default: GpuMemoryDbgDump(); EXIT("unknown obj.relation: %s\n", Core::EnumName(obj.relation).C_Str());
		}
	}

	if (unmap)
	{
		EXIT_NOT_IMPLEMENTED(!IsAllocated(vaddr, size));

		int index = 0;
		for (auto& a: m_heaps)
		{
			if (a.range.vaddr == vaddr && a.range.size == size)
			{
				EXIT_IF(a.objects_map1 == nullptr);
				EXIT_IF(a.objects_map2 == nullptr);
				EXIT_NOT_IMPLEMENTED(heap_id != index);
				EXIT_NOT_IMPLEMENTED(a.objects_size != 0);
				EXIT_NOT_IMPLEMENTED(!a.objects_map1->IsEmpty());
				EXIT_NOT_IMPLEMENTED(!a.objects_map2->IsEmpty());

				delete a.objects_map1;
				delete a.objects_map2;

				m_heaps.RemoveAt(index);
				break;
			}
			index++;
		}

		EXIT_NOT_IMPLEMENTED(IsAllocated(vaddr, size));
	}

	m_mutex.Unlock();

	for (auto& d: destructors)
	{
		d.delete_func(ctx, d.obj, &d.mem);
	}
}

GpuMemory::Destructor GpuMemory::Free(int heap_id, int object_id)
{
	KYTY_PROFILER_BLOCK("GpuMemory::Free", profiler::colors::Green400);

	auto& heap = m_heaps[heap_id];

	auto& h = heap.objects[object_id];
	EXIT_IF(h.free);
	auto&       o     = h.info;
	const auto& block = h.block;

	EXIT_IF(o.delete_func == nullptr);

	Destructor ret {};

	if (o.delete_func != nullptr)
	{
		if (Config::GetPrintfDirection() != Log::Direction::Silent)
		{
			for (int vi = 0; vi < block.vaddr_num; vi++)
			{
				printf("Delete: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", Core::EnumName(o.object.type).C_Str(),
				       block.vaddr[vi], block.size[vi]);
			}
		}
		// o.delete_func(ctx, o.object.obj, &o.mem);
		ret.delete_func = o.delete_func;
		ret.obj         = o.object.obj;
		ret.mem         = o.mem;
	}

	h.free             = true;
	h.next_free_id     = heap.first_free_id;
	heap.first_free_id = object_id;
	DeleteBlock(&h.block, heap_id, object_id);

	return ret;
}

bool GpuMemory::FindFast(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type, bool only_first,
                         int* id)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindFast", profiler::colors::Green200);

	auto& heap = m_heaps[heap_id];

	EXIT_IF(id == nullptr);

	for (int vi = 0; vi < vaddr_num; vi++)
	{
		for (int obj_id: heap.objects_map1->FindAll(vaddr[vi]))
		{
			auto& b = heap.objects[obj_id];
			EXIT_IF(b.free);
			if (b.info.object.type == type)
			{
				bool equal = true;
				if (b.block.vaddr_num == 1 || only_first)
				{
					if (GetOverlapType(b.block.vaddr[0], b.block.size[0], vaddr[0], size[0]) != OverlapType::Equals)
					{
						equal = false;
					}
				} else
				{
					for (int i = 0; i < vaddr_num; i++)
					{
						if (GetOverlapType(b.block.vaddr[i], b.block.size[i], vaddr[i], size[i]) != OverlapType::Equals)
						{
							equal = false;
							break;
						}
					}
				}
				if (equal)
				{
					*id = obj_id;
					return true;
				}
			}
		}
	}

	return false;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Vector<GpuMemory::OverlappedBlock> GpuMemory::FindBlocks_slow(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                                                              bool only_first)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindBlocks", profiler::colors::Green100);

	auto& heap = m_heaps[heap_id];

	EXIT_IF(vaddr_num <= 0 || vaddr_num > VADDR_BLOCKS_MAX);
	EXIT_IF(vaddr == nullptr || size == nullptr);
	EXIT_IF(only_first && vaddr_num != 1);

	Vector<GpuMemory::OverlappedBlock> ret;

	// TODO(): implement interval-tree

	if (vaddr_num != 1)
	{
		int index = 0;
		for (const auto& b: heap.objects)
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
		for (const auto& b: heap.objects)
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

	//	printf("FindBlocks:\n");
	//	for (int vi = 0; vi < vaddr_num; vi++)
	//	{
	//		printf("\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", vaddr[vi], size[vi]);
	//	}
	//	for (const auto& d: ret)
	//	{
	//		printf("\t id = %d, rel = %s\n", d.object_id, Core::EnumName(d.relation).C_Str());
	//		const auto& b = m_objects[d.object_id];
	//		for (int vi = 0; vi < b.block.vaddr_num; vi++)
	//		{
	//			printf("\t\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", b.block.vaddr[vi], b.block.size[vi]);
	//		}
	//	}

	return ret;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Vector<GpuMemory::OverlappedBlock> GpuMemory::FindBlocks(int heap_id, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                                                         bool only_first)
{
	KYTY_PROFILER_BLOCK("GpuMemory::FindBlocks", profiler::colors::Green100);

	auto& heap = m_heaps[heap_id];

	EXIT_IF(vaddr_num <= 0 || vaddr_num > VADDR_BLOCKS_MAX);
	EXIT_IF(vaddr == nullptr || size == nullptr);
	EXIT_IF(only_first && vaddr_num != 1);

	Vector<GpuMemory::OverlappedBlock> ret;

	// TODO(): implement interval-tree

	if (vaddr_num != 1)
	{
		for (int index: heap.objects_map2->FindAll(vaddr, size, vaddr_num))
		{
			const auto& b = heap.objects[index];
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
		}
	} else
	{
		for (int index: heap.objects_map2->FindAll(vaddr[0], size[0]))
		{
			const auto& b = heap.objects[index];
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
		}
	}

	{
		KYTY_PROFILER_BLOCK("sort");
		ret.Sort([](auto& b1, auto& b2) { return b1.object_id < b2.object_id; });
	}

	//
	//	printf("FindBlocks:\n");
	//	for (int vi = 0; vi < vaddr_num; vi++)
	//	{
	//		printf("\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", vaddr[vi], size[vi]);
	//	}
	//	for (const auto& d: ret)
	//	{
	//		printf("\t id = %d, rel = %s\n", d.object_id, Core::EnumName(d.relation).C_Str());
	//		const auto& b = m_objects[d.object_id];
	//		for (int vi = 0; vi < b.block.vaddr_num; vi++)
	//		{
	//			printf("\t\t vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", b.block.vaddr[vi], b.block.size[vi]);
	//		}
	//	}

	return ret;
}

GpuMemory::Block GpuMemory::CreateBlock(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, int heap_id, int obj_id)
{
	EXIT_IF(vaddr_num > VADDR_BLOCKS_MAX);
	EXIT_IF(vaddr == nullptr || size == nullptr);

	auto& heap = m_heaps[heap_id];

	Block nb {};
	nb.vaddr_num = vaddr_num;
	for (int vi = 0; vi < vaddr_num; vi++)
	{
		nb.vaddr[vi] = vaddr[vi];
		nb.size[vi]  = size[vi];
		heap.objects_size += size[vi];
		heap.objects_map1->Insert(vaddr[vi], obj_id);
		heap.objects_map2->Insert(vaddr[vi], size[vi], obj_id);
	}
	return nb;
}

void GpuMemory::DeleteBlock(Block* b, int heap_id, int obj_id)
{
	auto& heap = m_heaps[heap_id];

	for (int vi = 0; vi < b->vaddr_num; vi++)
	{
		heap.objects_size -= b->size[vi];
		heap.objects_map1->Erase(b->vaddr[vi], obj_id);
		heap.objects_map2->Erase(b->vaddr[vi], b->size[vi], obj_id);
	}
}

void GpuMemory::FrameDone()
{
	Core::LockGuard lock(m_mutex);

	m_current_frame++;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GpuMemory::WriteBack(GraphicContext* ctx, CommandProcessor* cp)
{
	GraphicsRunCommandProcessorLock(cp);

	Core::LockGuard lock(m_mutex);

	struct WriteBackObject
	{
		int heap_id   = -1;
		int object_id = -1;
	};

	Vector<WriteBackObject> objects;

	int heap_id = 0;
	for (auto& heap: m_heaps)
	{
		int index = 0;
		for (auto& h: heap.objects)
		{
			if (!h.free)
			{
				auto& o = h.info;
				if (o.in_use && o.write_back_func != nullptr && !o.read_only)
				{
					objects.Add(WriteBackObject({heap_id, index}));
				}
			}
			index++;
		}
		heap_id++;
	}

	if (!objects.IsEmpty())
	{
		// Guarantee that any previously submitted commands have completed
		GraphicsRunCommandProcessorFlush(cp);
		GraphicsRunCommandProcessorWait(cp);

		for (const auto& obj: objects)
		{
			auto& heap  = m_heaps[obj.heap_id];
			auto& h     = heap.objects[obj.object_id];
			auto& o     = h.info;
			auto& block = h.block;

			o.write_back_func(ctx, o.params, o.object.obj, block.vaddr, block.size, block.vaddr_num);
			o.cpu_update_time = get_current_time();

			if (!h.others.IsEmpty())
			{
				EXIT_NOT_IMPLEMENTED(h.others.Size() != 1);
				EXIT_NOT_IMPLEMENTED(h.others.At(0).relation != OverlapType::Equals);

				auto& o2 = heap.objects[h.others.At(0).object_id].info;

				o2.cpu_update_time = o.cpu_update_time;
				o2.submit_id       = 0;
				for (int vi = 0; vi < block.vaddr_num; vi++)
				{
					o2.hash[vi] = 0;
				}
				Update(o.submit_id, ctx, obj.heap_id, h.others.At(0).object_id);

				for (int vi = 0; vi < block.vaddr_num; vi++)
				{
					printf("WriteBack (GPU -> CPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 ", old_hash = 0x%016" PRIx64
					       ", new_hash = 0x%016" PRIx64 "\n",
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

					printf("WriteBack (GPU -> CPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 ", old_hash = 0x%016" PRIx64
					       ", new_hash = 0x%016" PRIx64 "\n",
					       Core::EnumName(o.object.type).C_Str(), block.vaddr[vi], block.size[vi], o.hash[vi], new_hash);

					o.hash[vi] = new_hash;
				}
			}

			o.in_use = false;
		}
	}

	GraphicsRunCommandProcessorUnlock(cp);
}

void GpuMemory::Flush(GraphicContext* ctx, uint64_t vaddr, uint64_t size)
{
	Core::LockGuard lock(m_mutex);

	int heap_id = GetHeapId(vaddr, size);

	EXIT_NOT_IMPLEMENTED(heap_id < 0);

	auto& heap = m_heaps[heap_id];

	auto object_ids = FindBlocks(heap_id, &vaddr, &size, 1);

	for (const auto& obj: object_ids)
	{
		auto& h = heap.objects[obj.object_id];
		EXIT_IF(h.free);

		Update(UINT64_MAX, ctx, heap_id, obj.object_id);
	}
}

void GpuMemory::FlushAll(GraphicContext* ctx)
{
	Core::LockGuard lock(m_mutex);

	int heap_id = 0;
	for (auto& heap: m_heaps)
	{
		int index = 0;
		for (auto& h: heap.objects)
		{
			if (!h.free)
			{
				Update(UINT64_MAX, ctx, heap_id, index);
			}
			index++;
		}
		heap_id++;
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
  [heap_id] INTEGER, 
  [id] INTEGER, 
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
  [submit_id] INT64, 
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
  [vk_mem_unique_id] INT64, 
  PRIMARY KEY([heap_id], [id]), 
  UNIQUE([heap_id], [id])) WITHOUT ROWID;

CREATE TABLE [ranges](
  [dump_id] INT, 
  [vaddr] TEXT, 
  [size] TEXT);
)");

		m_db_add_range  = m_db.Prepare("insert into ranges(dump_id, vaddr, size) values(:dump_id, :vaddr, :size)");
		m_db_add_object = m_db.Prepare(
		    "insert into objects(dump_id, heap_id, id, vaddr, vaddr2, vaddr3, size, size2, size3, obj, param0, param1, param2, param3, "
		    "param4, "
		    "param5, param6, param7, type, hash, hash2, "
		    "hash3, gpu_update_time, cpu_update_time, submit_id, scenario, others, write_back_func, delete_func, update_func, "
		    "use_last_frame, "
		    "use_num, in_use, read_only, "
		    "check_hash, vk_mem_size, "
		    "vk_mem_alignment, vk_mem_memoryTypeBits, vk_mem_property, vk_mem_memory, vk_mem_offset, vk_mem_type, vk_mem_unique_id) "
		    "values(:dump_id, :heap_id, :id, :vaddr, :vaddr2, :vaddr3, :size, :size2, :size3, :obj, :param0, :param1, :param2, :param3, "
		    ":param4, "
		    ":param5, "
		    ":param6, :param7, :type, :hash, "
		    ":hash2, :hash3, :gpu_update_time, :cpu_update_time, :submit_id, :scenario, :others, :write_back_func, :delete_func, "
		    ":update_func, "
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

		m_db.Exec("delete from ranges");

		int heap_id = 0;
		for (const auto& heap: m_heaps)
		{
			m_db_add_range->Reset();
			m_db_add_range->BindInt(":dump_id", dump_id);
			m_db_add_range->BindString(":vaddr", hex(heap.range.vaddr));
			m_db_add_range->BindString(":size", hex(heap.range.size));
			m_db_add_range->Step();

			int index = 0;
			for (const auto& r: heap.objects)
			{
				if (!r.free)
				{
					m_db_add_object->Reset();
					m_db_add_object->BindInt(":dump_id", dump_id);
					m_db_add_object->BindInt(":heap_id", heap_id);
					m_db_add_object->BindInt(":id", id(dump_id, index));
					(r.block.vaddr_num > 0 ? m_db_add_object->BindString(":vaddr", hex(r.block.vaddr[0]))
					                       : m_db_add_object->BindNull(":vaddr"));
					(r.block.vaddr_num > 1 ? m_db_add_object->BindString(":vaddr2", hex(r.block.vaddr[1]))
					                       : m_db_add_object->BindNull(":vaddr2"));
					(r.block.vaddr_num > 2 ? m_db_add_object->BindString(":vaddr3", hex(r.block.vaddr[2]))
					                       : m_db_add_object->BindNull(":vaddr3"));
					(r.block.vaddr_num > 0 ? m_db_add_object->BindString(":size", hex(r.block.size[0]))
					                       : m_db_add_object->BindNull(":size"));
					(r.block.vaddr_num > 1 ? m_db_add_object->BindString(":size2", hex(r.block.size[1]))
					                       : m_db_add_object->BindNull(":size2"));
					(r.block.vaddr_num > 2 ? m_db_add_object->BindString(":size3", hex(r.block.size[2]))
					                       : m_db_add_object->BindNull(":size3"));
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
					m_db_add_object->BindInt64(":submit_id", static_cast<int64_t>(r.info.submit_id));

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
			heap_id++;
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

struct VulkanMemoryStat
{
	std::atomic_uint64_t allocated[VK_MAX_MEMORY_TYPES];
	std::atomic_uint64_t count[VK_MAX_MEMORY_TYPES];
};

static VulkanMemoryStat* g_mem_stat = nullptr;

void GpuMemoryInit()
{
	EXIT_IF(g_gpu_memory != nullptr);
	EXIT_IF(g_gpu_resources != nullptr);

	g_gpu_memory    = new GpuMemory;
	g_gpu_resources = new GpuResources;

	g_mem_stat = new VulkanMemoryStat;

	for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
	{
		g_mem_stat->allocated[i] = 0;
		g_mem_stat->count[i]     = 0;
	}
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

void* GpuMemoryCreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, uint64_t vaddr, uint64_t size,
                            const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->CreateObject(submit_id, ctx, buffer, &vaddr, &size, 1, info);
}

void* GpuMemoryCreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size,
                            int vaddr_num, const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->CreateObject(submit_id, ctx, buffer, vaddr, size, vaddr_num, info);
}

Vector<GpuMemoryObject> GpuMemoryFindObjects(uint64_t vaddr, uint64_t size, GpuMemoryObjectType type, bool exact, bool only_first)
{
	EXIT_IF(g_gpu_memory == nullptr);

	return g_gpu_memory->FindObjects(&vaddr, &size, 1, type, exact, only_first);
}

void GpuMemoryResetHash(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type)
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->ResetHash(vaddr, size, vaddr_num, type);
}

void GpuMemoryDbgDump()
{
	EXIT_IF(g_gpu_memory == nullptr);

	// g_gpu_memory->DbgDbDump();
	// g_gpu_memory->DbgDbSave(U"_gpu_memory.db");

	// static int test_ms = 0; // Core::mem_new_state();

	// Core::Thread::Sleep(2000);
	//	Core::MemStats test_mem_stat = {test_ms, 0, 0};
	//	Core::mem_get_stat(&test_mem_stat);
	//	size_t   ut_total_allocated = test_mem_stat.total_allocated;
	//	uint32_t ut_blocks_num      = test_mem_stat.blocks_num;
	//	std::printf("mem stat: state = %d, blocks_num = %u, total_allocated = %" PRIu64 "\n", test_ms, ut_blocks_num, ut_total_allocated);
	// Core::mem_print(6);
	// test_ms = Core::mem_new_state();
}

void GpuMemoryFlush(GraphicContext* ctx, uint64_t vaddr, uint64_t size)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	// update vulkan objects after CPU-drawing
	g_gpu_memory->Flush(ctx, vaddr, size);
}

void GpuMemoryFlushAll(GraphicContext* ctx)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	// update vulkan objects after CPU-drawing
	g_gpu_memory->FlushAll(ctx);
}

void GpuMemoryFrameDone()
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->FrameDone();
}

void GpuMemoryWriteBack(GraphicContext* ctx, CommandProcessor* cp)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	// update CPU memory after GPU-drawing
	g_gpu_memory->WriteBack(ctx, cp);
}

bool GpuMemoryCheckAccessViolation(uint64_t /*vaddr*/, uint64_t /*size*/)
{
	return false;
}

bool GpuMemoryWatcherEnabled()
{
	return false;
}

bool VulkanAllocate(GraphicContext* ctx, VulkanMemory* mem)
{
	KYTY_PROFILER_FUNCTION();

	static std::atomic_uint64_t seq = 0;

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

	auto result = vkAllocateMemory(ctx->device, &alloc_info, nullptr, &mem->memory);

	if (result == VK_SUCCESS)
	{
		g_mem_stat->allocated[index] += mem->requirements.size;
		g_mem_stat->count[index]++;
		return true;
	}

	Core::StringList stat;
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		uint64_t allocated = g_mem_stat->allocated[i];
		uint64_t count     = g_mem_stat->count[i];
		stat.Add(String::FromPrintf("%u, %" PRIu64 ", %" PRIu64 "", i, count, allocated));
	}
	g_gpu_memory->DbgDbDump();
	g_gpu_memory->DbgDbSave(U"_gpu_memory.db");
	EXIT("size = %" PRIu64 ", index = %u, error: %s:%s\n", mem->requirements.size, index, string_VkResult(result),
	     stat.Concat(U'\n').C_Str());

	return false;
}

void VulkanFree(GraphicContext* ctx, VulkanMemory* mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);

	vkFreeMemory(ctx->device, mem->memory, nullptr);

	g_mem_stat->allocated[mem->type] -= mem->requirements.size;
	g_mem_stat->count[mem->type]--;

	mem->memory = nullptr;
}

void VulkanMapMemory(GraphicContext* ctx, VulkanMemory* mem, void** data)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(data == nullptr);

	vkMapMemory(ctx->device, mem->memory, mem->offset, mem->requirements.size, 0, data);
}

void VulkanUnmapMemory(GraphicContext* ctx, VulkanMemory* mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);

	vkUnmapMemory(ctx->device, mem->memory);
}

void VulkanBindImageMemory(GraphicContext* ctx, VulkanImage* image, VulkanMemory* mem)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(image == nullptr);

	vkBindImageMemory(ctx->device, image->image, mem->memory, mem->offset);
}

void VulkanBindBufferMemory(GraphicContext* ctx, VulkanBuffer* buffer, VulkanMemory* mem)
{
	KYTY_PROFILER_FUNCTION();

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
