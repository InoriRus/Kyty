#include "Emulator/Graphics/Objects/VertexBuffer.h"

#include "Kyty/Core/DbgAssert.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/Utils.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static void update_func(GraphicContext* ctx, const uint64_t* /*params*/, void* obj, const uint64_t* vaddr, const uint64_t* size,
                        int vaddr_num)
{
	KYTY_PROFILER_BLOCK("VertexBufferGpuObject::update_func");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);
	EXIT_IF(obj == nullptr);

	auto* vk_obj = static_cast<VulkanBuffer*>(obj);

	VulkanBuffer staging_buffer {};
	staging_buffer.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer.memory.property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VulkanCreateBuffer(ctx, *size, &staging_buffer);
	EXIT_NOT_IMPLEMENTED(staging_buffer.buffer == nullptr);

	void* data = nullptr;
	VulkanMapMemory(ctx, &staging_buffer.memory, &data);
	memcpy(data, reinterpret_cast<void*>(*vaddr), *size);
	VulkanUnmapMemory(ctx, &staging_buffer.memory);

	UtilCopyBuffer(&staging_buffer, vk_obj, *size);

	VulkanDeleteBuffer(ctx, &staging_buffer);
}

static void* create_func(GraphicContext* ctx, const uint64_t* params, const uint64_t* vaddr, const uint64_t* size, int vaddr_num,
                         VulkanMemory* mem)
{
	KYTY_PROFILER_BLOCK("VertexBufferGpuObject::Create");

	EXIT_IF(vaddr_num != 1 || size == nullptr || vaddr == nullptr || *vaddr == 0);
	EXIT_IF(mem == nullptr);
	EXIT_IF(ctx == nullptr);

	auto* vk_obj = new VulkanBuffer;

	vk_obj->usage           = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vk_obj->memory.property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vk_obj->buffer          = nullptr;

	VulkanCreateBuffer(ctx, *size, vk_obj);
	EXIT_NOT_IMPLEMENTED(vk_obj->buffer == nullptr);

	update_func(ctx, params, vk_obj, vaddr, size, vaddr_num);

	return vk_obj;
}

static void delete_func(GraphicContext* ctx, void* obj, VulkanMemory* /*mem*/)
{
	KYTY_PROFILER_BLOCK("VertexBufferGpuObject::delete_func");

	auto* vk_obj = reinterpret_cast<VulkanBuffer*>(obj);

	EXIT_IF(vk_obj == nullptr);
	EXIT_IF(vk_obj->buffer == nullptr);
	EXIT_IF(ctx == nullptr);

	VulkanDeleteBuffer(ctx, vk_obj);

	delete vk_obj;
}

bool VertexBufferGpuObject::Equal(const uint64_t* /*other*/) const
{
	return true;
}

GpuObject::create_func_t VertexBufferGpuObject::GetCreateFunc() const
{
	return create_func;
}

GpuObject::delete_func_t VertexBufferGpuObject::GetDeleteFunc() const
{
	return delete_func;
}

GpuObject::update_func_t VertexBufferGpuObject::GetUpdateFunc() const
{
	return update_func;
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
