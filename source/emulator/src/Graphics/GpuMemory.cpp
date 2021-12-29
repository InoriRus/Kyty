#include "Emulator/Graphics/GpuMemory.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Profiler.h"

#include <algorithm>
#include <atomic>
#include <vulkan/vulkan_core.h>

//#define XXH_INLINE_ALL
#include <xxhash/xxhash.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class GpuMemory
{
public:
	GpuMemory() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~GpuMemory() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(GpuMemory);

	bool IsAllocated(uint64_t vaddr, uint64_t size);
	void SetAllocatedRange(uint64_t vaddr, uint64_t size);
	void Free(GraphicContext* ctx, uint64_t vaddr, uint64_t size);

	void* GetObject(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const GpuObject& info);
	void  ResetHash(GraphicContext* ctx, uint64_t* vaddr, uint64_t* size, int vaddr_num, GpuMemoryObjectType type);
	void  FrameDone();

	// Sync: GPU -> CPU
	void WriteBack(GraphicContext* ctx);

	// Sync: CPU -> GPU
	void Flush(GraphicContext* ctx);

	void DbgDump();

private:
	static constexpr int OBJ_OVERLAPS_MAX = 2;
	static constexpr int VADDR_BLOCKS_MAX = 3;

	struct AllocatedRange
	{
		uint64_t vaddr;
		uint64_t size;
	};

	struct ObjectInfo
	{
		void*                        obj                           = nullptr;
		uint64_t                     params[GpuObject::PARAMS_MAX] = {};
		GpuMemoryObjectType          type                          = GpuMemoryObjectType::Invalid;
		uint64_t                     hash[VADDR_BLOCKS_MAX]        = {};
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

	struct Object
	{
		uint64_t   vaddr[VADDR_BLOCKS_MAX] = {};
		uint64_t   size[VADDR_BLOCKS_MAX]  = {};
		int        vaddr_num               = 0;
		ObjectInfo overlaps[OBJ_OVERLAPS_MAX];
		int        overlaps_num = 0;
		bool       free         = true;
	};

	void Free(GraphicContext* ctx, Object& h);

	Core::Mutex m_mutex;

	Vector<AllocatedRange> m_allocated;
	Vector<Object>         m_objects;
	uint64_t               m_objects_size  = 0;
	uint64_t               m_current_frame = 0;
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

static bool vaddr_equal(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const uint64_t* vaddr2, const uint64_t* size2,
                        int vaddr_num2)
{
	if (vaddr_num != vaddr_num2)
	{
		return false;
	}
	for (int i = 0; i < vaddr_num; i++)
	{
		if (vaddr[i] != vaddr2[i] || size[i] != size2[i])
		{
			return false;
		}
	}
	return true;
}

static bool vaddr_overlap(const uint64_t* hvaddr, const uint64_t* hsize, int vaddr_num, uint64_t vaddr, uint64_t size)
{
	for (int i = 0; i < vaddr_num; i++)
	{
		if ((vaddr >= hvaddr[i] && vaddr < hvaddr[i] + hsize[i]) ||
		    ((vaddr + size - 1) >= hvaddr[i] && (vaddr + size - 1) < hvaddr[i] + hsize[i]))
		{
			return true;
		}
	}
	return false;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void* GpuMemory::GetObject(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const GpuObject& info)
{
	EXIT_IF(info.type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

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

	Object* update_object = nullptr;

	for (auto& h: m_objects)
	{
		if (!h.free && vaddr_equal(h.vaddr, h.size, h.vaddr_num, vaddr, size, vaddr_num))
		{
			for (int oi = 0; oi < h.overlaps_num; oi++)
			{
				auto& o = h.overlaps[oi];
				if (o.type == info.type && info.Equal(o.params))
				{
					bool need_update = false;
					for (int vi = 0; vi < h.vaddr_num; vi++)
					{
						if (o.hash[vi] != hash[vi])
						{
							printf("Update (CPU -> GPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n",
							       Core::EnumName(o.type).C_Str(), h.vaddr[vi], h.size[vi]);
							need_update = true;
							o.hash[vi]  = hash[vi];
						}
					}
					if (need_update)
					{
						EXIT_IF(o.update_func == nullptr);
						o.update_func(ctx, o.params, o.obj, vaddr, size, vaddr_num);
					}
					o.use_num++;
					o.use_last_frame = m_current_frame;
					o.in_use         = true;
					o.read_only      = info.read_only;
					o.check_hash     = info.check_hash;
					return o.obj;
				}
			}

			if (h.overlaps_num == 1 &&
			    (h.overlaps[0].type == GpuMemoryObjectType::VideoOutBuffer && info.type == GpuMemoryObjectType::StorageBuffer))
			{
				update_object = &h;
				break;
			}

			// EXIT("not implemented");

			Free(ctx, h);
			break;
		}

		for (int vi = 0; vi < vaddr_num; vi++)
		{
			if (!h.free && vaddr_overlap(h.vaddr, h.size, h.overlaps_num, vaddr[vi], size[vi]))
			{
				if (h.overlaps_num == 1 &&
				    (h.overlaps[0].type == GpuMemoryObjectType::Label || h.overlaps[0].type == GpuMemoryObjectType::StorageBuffer))
				{
					Free(ctx, h);
				} else
				{
					KYTY_NOT_IMPLEMENTED;
				}
			}
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

	o.type = info.type;
	o.obj  = nullptr;
	for (int vi = 0; vi < vaddr_num; vi++)
	{
		o.hash[vi] = hash[vi];
	}
	o.obj             = info.Create(ctx, vaddr, size, vaddr_num, &o.mem);
	o.write_back_func = info.GetWriteBackFunc();
	o.delete_func     = info.GetDeleteFunc();
	o.update_func     = info.GetUpdateFunc();
	o.use_num         = 1;
	o.use_last_frame  = m_current_frame;
	o.in_use          = true;
	o.read_only       = info.read_only;
	o.check_hash      = info.check_hash;

	bool updated = false;

	if (update_object != nullptr)
	{
		EXIT_IF(update_object->overlaps_num >= OBJ_OVERLAPS_MAX);
		update_object->overlaps[update_object->overlaps_num++] = o;

		updated = true;
	} else
	{
		for (auto& u: m_objects)
		{
			if (u.free)
			{
				u.overlaps_num = 1;
				u.overlaps[0]  = o;
				u.free         = false;
				for (int vi = 0; vi < vaddr_num; vi++)
				{
					u.vaddr[vi] = vaddr[vi];
					u.size[vi]  = size[vi];
					m_objects_size += size[vi];
				}
				u.vaddr_num = vaddr_num;
				updated     = true;
				break;
			}
		}
	}

	if (!updated)
	{
		Object h {};
		for (int vi = 0; vi < vaddr_num; vi++)
		{
			h.vaddr[vi] = vaddr[vi];
			h.size[vi]  = size[vi];
			m_objects_size += size[vi];
		}
		h.vaddr_num    = vaddr_num;
		h.overlaps_num = 1;
		h.overlaps[0]  = o;
		h.free         = false;
		m_objects.Add(h);
	}

	return o.obj;
}

void GpuMemory::ResetHash(GraphicContext* /*ctx*/, uint64_t* vaddr, uint64_t* size, int vaddr_num, GpuMemoryObjectType type)
{
	EXIT_IF(type == GpuMemoryObjectType::Invalid);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num > VADDR_BLOCKS_MAX || vaddr_num <= 0);

	Core::LockGuard lock(m_mutex);

	uint64_t new_hash = 0;

	for (auto& h: m_objects)
	{
		if (!h.free && vaddr_equal(h.vaddr, h.size, h.vaddr_num, vaddr, size, vaddr_num))
		{
			for (int oi = 0; oi < h.overlaps_num; oi++)
			{
				auto& o = h.overlaps[oi];
				if (o.type == type)
				{
					for (int vi = 0; vi < h.vaddr_num; vi++)
					{
						printf("ResetHash: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 ", old_hash = 0x%016" PRIx64
						       ", new_hash = 0x%016" PRIx64 "\n",
						       Core::EnumName(o.type).C_Str(), h.vaddr[vi], h.size[vi], o.hash[vi], new_hash);

						o.hash[vi] = new_hash;
					}
				}
			}
		}
	}
}

void GpuMemory::Free(GraphicContext* ctx, uint64_t vaddr, uint64_t size)
{
	Core::LockGuard lock(m_mutex);

	printf("Release gpu objects:\n");
	printf("\t gpu_vaddr = 0x%016" PRIx64 "\n", vaddr);
	printf("\t size   = 0x%016" PRIx64 "\n", size);

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

	for (auto& h: m_objects)
	{
		for (int vi = 0; vi < h.vaddr_num; vi++)
		{
			if (!h.free && (h.vaddr[vi] >= vaddr && h.vaddr[vi] < vaddr + size))
			{
				Free(ctx, h);
				break;
			}
		}
	}
}

void GpuMemory::Free(GraphicContext* ctx, Object& h)
{
	for (int oi = 0; oi < h.overlaps_num; oi++)
	{
		auto& o = h.overlaps[oi];

		EXIT_IF(o.delete_func == nullptr);

		if (o.delete_func != nullptr)
		{
			for (int vi = 0; vi < h.vaddr_num; vi++)
			{
				printf("Delete: type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", Core::EnumName(o.type).C_Str(),
				       h.vaddr[vi], h.size[vi]);
			}

			o.delete_func(ctx, o.obj, &o.mem);
		}
	}
	h.overlaps_num = 0;
	h.free         = true;
	for (int vi = 0; vi < h.vaddr_num; vi++)
	{
		m_objects_size -= h.size[vi];
	}
	h.vaddr_num = 0;
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

	for (auto& h: m_objects)
	{
		if (!h.free)
		{
			for (int oi = 0; oi < h.overlaps_num; oi++)
			{
				auto& o = h.overlaps[oi];
				if (o.in_use && /*o.use_last_frame >= m_current_frame &&*/ o.write_back_func != nullptr && !o.read_only)
				{
					o.write_back_func(ctx, o.obj, h.vaddr, h.size, h.vaddr_num);

					for (int vi = 0; vi < h.vaddr_num; vi++)
					{
						uint64_t new_hash = 0;

						if (o.check_hash)
						{
							new_hash = calc_hash(reinterpret_cast<const uint8_t*>(h.vaddr[vi]), h.size[vi]);
						}

						printf("WriteBack (GPU -> CPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64
						       ", old_hash = 0x%016" PRIx64 ", new_hash = 0x%016" PRIx64 "\n",
						       Core::EnumName(o.type).C_Str(), h.vaddr[vi], h.size[vi], o.hash[vi], new_hash);

						o.hash[vi] = new_hash;
					}

					for (int oi2 = 0; oi2 < h.overlaps_num; oi2++)
					{
						if (oi2 != oi)
						{
							auto& o2 = h.overlaps[oi2];

							bool need_update = false;

							for (int vi = 0; vi < h.vaddr_num; vi++)
							{
								uint64_t hash = o.hash[vi];

								if (o2.hash[vi] != hash)
								{
									printf("Update (CPU -> GPU): type = %s, vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64
									       ", old_hash = 0x%016" PRIx64 ", new_hash = 0x%016" PRIx64 "\n",
									       Core::EnumName(o2.type).C_Str(), h.vaddr[vi], h.size[vi], o2.hash[vi], hash);
									o2.hash[vi] = hash;
									need_update = true;
								}
							}

							if (need_update)
							{
								EXIT_IF(o2.update_func == nullptr);

								o2.update_func(ctx, o2.params, o2.obj, h.vaddr, h.size, h.vaddr_num);
							}
						}
					}

					o.in_use = false;
				}
			}
		}
	}
}

void GpuMemory::Flush(GraphicContext* ctx)
{
	Core::LockGuard lock(m_mutex);

	for (auto& h: m_objects)
	{
		if (!h.free)
		{
			for (int oi = 0; oi < h.overlaps_num; oi++)
			{
				auto& o = h.overlaps[oi];

				EXIT_IF(o.update_func == nullptr);
				o.update_func(ctx, o.params, o.obj, h.vaddr, h.size, h.vaddr_num);
			}
		}
	}
}

void GpuMemory::DbgDump()
{
	Core::LockGuard lock(m_mutex);

	printf("--- Gpu Memory ---\n");

	for (auto& o: m_allocated)
	{
		printf("Allocated block: vaddr = 0x%016" PRIx64 ", size = 0x%016" PRIx64 "\n", o.vaddr, o.size);
	}

	printf("m_current_frame = %" PRIu64 "\n", m_current_frame);
	printf("m_objects_size  = %" PRIu64 "\n", m_objects_size);

	for (auto& h: m_objects)
	{
		if (!h.free)
		{
			printf("Object:\n");
			for (int vi = 0; vi < h.vaddr_num; vi++)
			{
				printf("\t vaddr        = 0x%016" PRIx64 "\n", h.vaddr[vi]);
				printf("\t size         = 0x%016" PRIx64 "\n", h.size[vi]);
				GpuResources::Info res_info;
				if (g_gpu_resources->FindInfo(h.vaddr[vi], &res_info))
				{
					printf("\t {\n");
					printf("\t\t RegisteredResource: %s\n", res_info.name.C_Str());
					printf("\t\t addr:             %016" PRIx64 "\n", res_info.memory);
					printf("\t\t size:             %" PRIu64 "\n", res_info.size);
					printf("\t\t type:             %" PRIu32 "\n", res_info.type);
					printf("\t\t user_data:        %" PRIu64 "\n", res_info.user_data);
					printf("\t }\n");

					// EXIT_NOT_IMPLEMENTED(res_info.size != h.size[vi]);
					// EXIT_NOT_IMPLEMENTED(res_info.memory != h.vaddr[vi]);
				}
			}
			printf("\t overlaps_num = %d\n", h.overlaps_num);
			for (int oi = 0; oi < h.overlaps_num; oi++)
			{
				auto& o = h.overlaps[oi];
				printf("\t [%d] type           = %s\n", oi, Core::EnumName(o.type).C_Str());
				for (int vi = 0; vi < h.vaddr_num; vi++)
				{
					printf("\t [%d] hash           = 0x%016" PRIx64 "\n", oi, o.hash[vi]);
				}
				printf("\t [%d] vk_size        = 0x%016" PRIx64 "\n", oi, o.mem.requirements.size);
				printf("\t [%d] vk_align       = 0x%016" PRIx64 "\n", oi, o.mem.requirements.alignment);
				printf("\t [%d] vk_type        = 0x%08" PRIx32 "\n", oi, o.mem.type);
				printf("\t [%d] use_last_frame = %" PRIu64 "\n", oi, o.use_last_frame);
				printf("\t [%d] use_num        = %" PRIu64 "\n", oi, o.use_num);
				printf("\t [%d] in_use         = %s\n", oi, o.in_use ? "true" : "false");
				printf("\t [%d] read_only      = %s\n", oi, o.read_only ? "true" : "false");
				printf("\t [%d] check_hash     = %s\n", oi, o.check_hash ? "true" : "false");
			}
		}
	}
}

void GpuMemoryInit()
{
	EXIT_IF(g_gpu_memory != nullptr);
	EXIT_IF(g_gpu_resources != nullptr);

	g_gpu_memory    = new GpuMemory;
	g_gpu_resources = new GpuResources;
}

void GpuMemorySetAllocatedRange(uint64_t vaddr, uint64_t size)
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->SetAllocatedRange(vaddr, size);
}

void GpuMemoryFree(GraphicContext* ctx, uint64_t vaddr, uint64_t size)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	g_gpu_memory->Free(ctx, vaddr, size);
}

void* GpuMemoryGetObject(GraphicContext* ctx, uint64_t vaddr, uint64_t size, const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->GetObject(ctx, &vaddr, &size, 1, info);
}

void* GpuMemoryGetObject(GraphicContext* ctx, const uint64_t* vaddr, const uint64_t* size, int vaddr_num, const GpuObject& info)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	return g_gpu_memory->GetObject(ctx, vaddr, size, vaddr_num, info);
}

void GpuMemoryResetHash(GraphicContext* ctx, uint64_t vaddr, uint64_t size, GpuMemoryObjectType type)
{
	EXIT_IF(g_gpu_memory == nullptr);
	EXIT_IF(ctx == nullptr);

	g_gpu_memory->ResetHash(ctx, &vaddr, &size, 1, type);
}

void GpuMemoryDbgDump()
{
	EXIT_IF(g_gpu_memory == nullptr);

	g_gpu_memory->DbgDump();
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

void VulkanBindImageMemory(GraphicContext* ctx, TextureVulkanImage* image, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(image == nullptr);

	vkBindImageMemory(ctx->device, image->image, mem->memory, mem->offset);
}

void VulkanBindImageMemory(GraphicContext* ctx, VideoOutVulkanImage* image, VulkanMemory* mem)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(mem == nullptr);
	EXIT_IF(image == nullptr);

	vkBindImageMemory(ctx->device, image->image, mem->memory, mem->offset);
}

void VulkanBindImageMemory(GraphicContext* ctx, DepthStencilVulkanImage* image, VulkanMemory* mem)
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
