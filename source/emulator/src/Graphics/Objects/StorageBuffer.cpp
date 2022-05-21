#include "Emulator/Graphics/Objects/StorageBuffer.h"

#include "Kyty/Core/DbgAssert.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/GraphicsRun.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static void update_func(GraphicContext* ctx, const uint64_t* /*params*/, void* obj, const uint64_t* vaddr, const uint64_t* size,
                        int vaddr_num)
{
	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::update_func");

	EXIT_IF(ctx == nullptr);
	EXIT_IF(obj == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	auto* vk_obj = reinterpret_cast<StorageVulkanBuffer*>(obj);

	void* data = nullptr;
	// vkMapMemory(ctx->device, vk_obj->memory.memory, vk_obj->memory.offset, *size, 0, &data);
	VulkanMapMemory(ctx, &vk_obj->memory, &data);
	memcpy(data, reinterpret_cast<void*>(*vaddr), *size);
	// vkUnmapMemory(ctx->device, vk_obj->memory.memory);
	VulkanUnmapMemory(ctx, &vk_obj->memory);
}

static void* create_func(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);

	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto* vk_obj = new StorageVulkanBuffer;

	vk_obj->usage           = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	vk_obj->memory.property = static_cast<uint32_t>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
	                          VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	vk_obj->buffer = nullptr;

	VulkanCreateBuffer(ctx, *size, vk_obj);
	EXIT_NOT_IMPLEMENTED(vk_obj->buffer == nullptr);

	update_func(ctx, params, vk_obj, vaddr, size, vaddr_num);

	return vk_obj;
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::delete_func");

	auto* vk_obj = reinterpret_cast<StorageVulkanBuffer*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(vk_obj->buffer == nullptr);
	EXIT_IF(ctx == nullptr);

	EXIT_IF(vk_obj->cp == nullptr);

	// All submitted commands that refer to any element of pDescriptorSets must have completed execution
	GraphicsRunCommandProcessorWait(vk_obj->cp);

	VulkanDeleteBuffer(ctx, vk_obj);

	delete vk_obj;
}

static void write_back(GraphicContext* ctx, const uint64_t* /*params*/, void* obj, const uint64_t* vaddr, const uint64_t* size,
                       int vaddr_num)
{
	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::write_back");

	EXIT_IF(ctx == nullptr);
	EXIT_IF(obj == nullptr);
	EXIT_IF(vaddr == nullptr || size == nullptr || vaddr_num != 1);

	auto* vk_obj = reinterpret_cast<StorageVulkanBuffer*>(obj);

	void* data = nullptr;

	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::write_back::vkMapMemory");
	// vkMapMemory(ctx->device, vk_obj->memory.memory, vk_obj->memory.offset, *size, 0, &data);
	VulkanMapMemory(ctx, &vk_obj->memory, &data);
	KYTY_PROFILER_END_BLOCK;

	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::write_back::memcpy");
	memcpy(reinterpret_cast<void*>(*vaddr), data, *size);
	KYTY_PROFILER_END_BLOCK;

	KYTY_PROFILER_BLOCK("StorageBufferGpuObject::write_back::vkUnmapMemory");
	// vkUnmapMemory(ctx->device, vk_obj->memory.memory);
	VulkanUnmapMemory(ctx, &vk_obj->memory);
	KYTY_PROFILER_END_BLOCK;
}

bool StorageBufferGpuObject::Equal(const uint64_t* other) const
{
	return params[0] == other[0] && params[1] == other[1];
}

GpuObject::create_func_t StorageBufferGpuObject::GetCreateFunc() const
{
	return create_func;
}

GpuObject::write_back_func_t StorageBufferGpuObject::GetWriteBackFunc() const
{
	return write_back;
}

GpuObject::delete_func_t StorageBufferGpuObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t StorageBufferGpuObject::GetUpdateFunc() const
{
	return update_func;
}

void StorageBufferSet(CommandBuffer* cmd_buffer, StorageVulkanBuffer* buffer)
{
	buffer->cp = cmd_buffer->GetParent();
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
