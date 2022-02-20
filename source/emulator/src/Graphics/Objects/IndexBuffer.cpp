#include "Emulator/Graphics/Objects/IndexBuffer.h"

#include "Kyty/Core/DbgAssert.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

#include <vulkan/vulkan_core.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static void* create_func(GraphicContext* ctx, const uint64_t* /*params*/, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("IndexBufferGpuObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);

	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto* vk_obj = new VulkanBuffer;

	vk_obj->usage           = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	vk_obj->memory.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vk_obj->buffer          = nullptr;

	VulkanCreateBuffer(ctx, *size, vk_obj);
	EXIT_NOT_IMPLEMENTED(vk_obj->buffer == nullptr);

	VulkanBuffer staging_buffer {};
	staging_buffer.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer.memory.property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VulkanCreateBuffer(ctx, *size, &staging_buffer);
	EXIT_NOT_IMPLEMENTED(staging_buffer.buffer == nullptr);

	void* data = nullptr;
	// vkMapMemory(ctx->device, staging_buffer.memory.memory, staging_buffer.memory.offset, *size, 0, &data);
	VulkanMapMemory(ctx, &staging_buffer.memory, &data);
	memcpy(data, reinterpret_cast<void*>(*vaddr), *size);
	// vkUnmapMemory(ctx->device, staging_buffer.memory.memory);
	VulkanUnmapMemory(ctx, &staging_buffer.memory);

	UtilCopyBuffer(&staging_buffer, vk_obj, *size);

	VulkanDeleteBuffer(ctx, &staging_buffer);

	return vk_obj;
}

static void update_func(GraphicContext* /*ctx*/, const uint64_t* /*params*/, void* /*obj*/, const uint64_t* /*vaddr*/,
                        const uint64_t* /*size*/, int /*vaddr_num*/)
{
	KYTY_PROFILER_BLOCK("IndexBufferGpuObject::update_func");

	KYTY_NOT_IMPLEMENTED;
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("IndexBufferGpuObject::delete_func");

	auto* vk_obj = reinterpret_cast<VulkanBuffer*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(vk_obj->buffer == nullptr);
	EXIT_IF(ctx == nullptr);

	VulkanDeleteBuffer(ctx, vk_obj);

	delete vk_obj;
}

bool IndexBufferGpuObject::Equal(const uint64_t* /*other*/) const
{
	return true;
}

GpuObject::create_func_t IndexBufferGpuObject::GetCreateFunc() const
{
	return create_func;
}

GpuObject::delete_func_t IndexBufferGpuObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t IndexBufferGpuObject::GetUpdateFunc() const
{
	return update_func;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
