#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_GPUMEMORY_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_GPUMEMORY_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class CommandBuffer;
class CommandProcessor;
struct GraphicContext;
struct VulkanMemory;
struct VulkanBuffer;
struct VulkanImage;

enum class GpuMemoryMode
{
	NoAccess,
	Read,
	Write,
	ReadWrite
};

enum class GpuMemoryObjectType : uint64_t
{
	Invalid,
	VideoOutBuffer,
	DepthStencilBuffer,
	Label,
	IndexBuffer,
	VertexBuffer,
	StorageBuffer,
	Texture,
	RenderTexture,
	StorageTexture,

	Max
};

enum class GpuMemoryScenario
{
	Common,
	GenerateMips,
	TextureTriplet
};

struct GpuMemoryObject
{
	GpuMemoryObjectType type = GpuMemoryObjectType::Invalid;
	void*               obj  = nullptr;
};

class GpuObject
{
public:
	using create_func_t = void* (*)(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
	                                VulkanMemory* mem);
	using create_from_objects_func_t = void* (*)(GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* params,
	                                             GpuMemoryScenario scenario, const Vector<GpuMemoryObject>& objects, VulkanMemory* mem);
	using write_back_func_t = void (*)(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size,
	                                   int vaddr_num);
	using delete_func_t     = void (*)(GraphicContext* ctx, void* obj, VulkanMemory* mem);
	using update_func_t     = void (*)(GraphicContext* ctx, const uint64_t* params, void* obj, const uint64_t* vaddr, const uint64_t* size,
                                   int vaddr_num);

	static constexpr int PARAMS_MAX = 8;

	GpuObject()          = default;
	virtual ~GpuObject() = default;

	KYTY_CLASS_DEFAULT_COPY(GpuObject);

	virtual bool Equal(const uint64_t* other) const = 0;
	// virtual bool Reuse(const uint64_t* /*other*/) const { return false; };

	[[nodiscard]] virtual create_func_t              GetCreateFunc() const            = 0;
	[[nodiscard]] virtual create_from_objects_func_t GetCreateFromObjectsFunc() const = 0;
	[[nodiscard]] virtual write_back_func_t          GetWriteBackFunc() const         = 0;
	[[nodiscard]] virtual delete_func_t              GetDeleteFunc() const            = 0;
	[[nodiscard]] virtual update_func_t              GetUpdateFunc() const            = 0;

	uint64_t            params[PARAMS_MAX] = {};
	bool                check_hash         = false;
	bool                read_only          = false;
	GpuMemoryObjectType type               = GpuMemoryObjectType::Invalid;
};

void GpuMemoryInit();

void  GpuMemorySetAllocatedRange(uint64_t vaddr, uint64_t size);
void  GpuMemoryFree(GraphicContext* ctx, uint64_t vaddr, uint64_t size, bool unmap);
void* GpuMemoryCreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, uint64_t vaddr, uint64_t size,
                            const GpuObject& info);
void* GpuMemoryCreateObject(uint64_t submit_id, GraphicContext* ctx, CommandBuffer* buffer, const uint64_t* vaddr, const uint64_t* size,
                            int vaddr_num, const GpuObject& info);
void  GpuMemoryResetHash(const uint64_t* vaddr, const uint64_t* size, int vaddr_num, GpuMemoryObjectType type);
void  GpuMemoryDbgDump();
void  GpuMemoryFlush(GraphicContext* ctx, uint64_t vaddr, uint64_t size);
void  GpuMemoryFlushAll(GraphicContext* ctx);
void  GpuMemoryFrameDone();
void  GpuMemoryWriteBack(GraphicContext* ctx, CommandProcessor* cp);
bool  GpuMemoryCheckAccessViolation(uint64_t vaddr, uint64_t size);
bool  GpuMemoryWatcherEnabled();

Vector<GpuMemoryObject> GpuMemoryFindObjects(uint64_t vaddr, uint64_t size, GpuMemoryObjectType type, bool exact, bool only_first);

bool VulkanAllocate(GraphicContext* ctx, VulkanMemory* mem);
void VulkanFree(GraphicContext* ctx, VulkanMemory* mem);
void VulkanMapMemory(GraphicContext* ctx, VulkanMemory* mem, void** data);
void VulkanUnmapMemory(GraphicContext* ctx, VulkanMemory* mem);
void VulkanBindImageMemory(GraphicContext* ctx, VulkanImage* image, VulkanMemory* mem);
void VulkanBindBufferMemory(GraphicContext* ctx, VulkanBuffer* buffer, VulkanMemory* mem);

void GpuMemoryRegisterOwner(uint32_t* owner_handle, const char* name);
void GpuMemoryRegisterResource(uint32_t* resource_handle, uint32_t owner_handle, const void* memory, size_t size, const char* name,
                               uint32_t type, uint64_t user_data);
void GpuMemoryUnregisterAllResourcesForOwner(uint32_t owner_handle);
void GpuMemoryUnregisterOwnerAndResources(uint32_t owner_handle);
void GpuMemoryUnregisterResource(uint32_t resource_handle);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_OBJECTS_GPUMEMORY_H_ */
