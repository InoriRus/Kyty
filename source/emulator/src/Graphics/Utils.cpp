#include "Emulator/Graphics/Utils.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Graphics/GraphicContext.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static void set_image_layout(VkCommandBuffer buffer, VulkanImage* dst_image, uint32_t base_level, uint32_t levels,
                             VkImageAspectFlags aspect_mask, VkImageLayout old_image_layout, VkImageLayout new_image_layout)
{
	EXIT_IF(buffer == nullptr);

	EXIT_IF(old_image_layout != VK_IMAGE_LAYOUT_UNDEFINED && dst_image->layout != old_image_layout);

	VkImageMemoryBarrier image_memory_barrier {};
	image_memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext                           = nullptr;
	image_memory_barrier.srcAccessMask                   = 0;
	image_memory_barrier.dstAccessMask                   = 0;
	image_memory_barrier.oldLayout                       = old_image_layout;
	image_memory_barrier.newLayout                       = new_image_layout;
	image_memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image                           = dst_image->image;
	image_memory_barrier.subresourceRange.aspectMask     = aspect_mask;
	image_memory_barrier.subresourceRange.baseMipLevel   = base_level;
	image_memory_barrier.subresourceRange.levelCount     = levels;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount     = 1;

	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = 0; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = 0; // VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = 0; // VK_ACCESS_TRANSFER_READ_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = 0; // VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	if (old_image_layout == VK_IMAGE_LAYOUT_PREINITIALIZED)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = 0; /*VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT*/
		image_memory_barrier.dstAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = 0; // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = 0; // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	VkPipelineStageFlags src_stages  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(buffer, src_stages, dest_stages, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	dst_image->layout = new_image_layout;
}

void UtilBufferToImage(CommandBuffer* buffer, VulkanBuffer* src_buffer, uint32_t src_pitch, VulkanImage* dst_image, uint64_t dst_layout)
{
	EXIT_IF(src_buffer == nullptr);
	EXIT_IF(src_buffer->buffer == nullptr);
	EXIT_IF(dst_image == nullptr);
	EXIT_IF(dst_image->image == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	set_image_layout(vk_buffer, dst_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy region {};
	region.bufferOffset      = 0;
	region.bufferRowLength   = (src_pitch != dst_image->extent.width ? src_pitch : 0);
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {dst_image->extent.width, dst_image->extent.height, 1};

	vkCmdCopyBufferToImage(vk_buffer, src_buffer->buffer, dst_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	set_image_layout(vk_buffer, dst_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                 static_cast<VkImageLayout>(dst_layout));
}

void UtilImageToBuffer(CommandBuffer* buffer, VulkanImage* src_image, VulkanBuffer* dst_buffer, uint32_t dst_pitch, uint64_t src_layout)
{
	EXIT_IF(dst_buffer == nullptr);
	EXIT_IF(dst_buffer->buffer == nullptr);
	EXIT_IF(src_image == nullptr);
	EXIT_IF(src_image->image == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	set_image_layout(vk_buffer, src_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, src_image->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkBufferImageCopy region {};
	region.bufferOffset      = 0;
	region.bufferRowLength   = (dst_pitch != src_image->extent.width ? dst_pitch : 0);
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {src_image->extent.width, src_image->extent.height, 1};

	vkCmdCopyImageToBuffer(vk_buffer, src_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_buffer->buffer, 1, &region);

	set_image_layout(vk_buffer, src_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                 static_cast<VkImageLayout>(src_layout));
}

void UtilBufferToImage(CommandBuffer* buffer, VulkanBuffer* src_buffer, VulkanImage* dst_image, const Vector<BufferImageCopy>& regions,
                       uint64_t dst_layout)
{
	EXIT_IF(src_buffer == nullptr);
	EXIT_IF(src_buffer->buffer == nullptr);
	EXIT_IF(dst_image == nullptr);
	EXIT_IF(dst_image->image == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	EXIT_NOT_IMPLEMENTED(regions.Size() >= 16);

	VkBufferImageCopy region[16];

	uint32_t index = 0;
	for (const auto& r: regions)
	{
		region[index].bufferOffset                    = r.offset;
		region[index].bufferRowLength                 = (r.width != r.pitch ? r.pitch : 0);
		region[index].bufferImageHeight               = 0;
		region[index].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region[index].imageSubresource.mipLevel       = r.dst_level;
		region[index].imageSubresource.baseArrayLayer = 0;
		region[index].imageSubresource.layerCount     = 1;
		region[index].imageOffset                     = {r.dst_x, r.dst_y, 0};
		region[index].imageExtent                     = {r.width, r.height, 1};
		index++;
	}

	set_image_layout(vk_buffer, dst_image, 0, VK_REMAINING_MIP_LEVELS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	vkCmdCopyBufferToImage(vk_buffer, src_buffer->buffer, dst_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, index, region);

	set_image_layout(vk_buffer, dst_image, 0, VK_REMAINING_MIP_LEVELS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                 static_cast<VkImageLayout>(dst_layout));
}

void UtilImageToImage(CommandBuffer* buffer, const Vector<ImageImageCopy>& regions, VulkanImage* dst_image, uint64_t dst_layout)
{
	EXIT_IF(dst_image == nullptr);
	EXIT_IF(dst_image->image == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	EXIT_NOT_IMPLEMENTED(regions.Size() >= 16);

	set_image_layout(vk_buffer, dst_image, 0, VK_REMAINING_MIP_LEVELS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	for (const auto& r: regions)
	{
		VkImageCopy region;

		auto src_layout = r.src_image->layout;

		region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel       = r.src_level;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount     = 1;
		region.srcOffset                     = {r.src_x, r.src_y, 0};
		region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel       = r.dst_level;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount     = 1;
		region.dstOffset                     = {r.dst_x, r.dst_y, 0};
		region.extent                        = {r.width, r.height, 1};

		set_image_layout(vk_buffer, r.src_image, r.src_level, 1, VK_IMAGE_ASPECT_COLOR_BIT, src_layout,
		                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		vkCmdCopyImage(vk_buffer, r.src_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		set_image_layout(vk_buffer, r.src_image, r.src_level, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                 src_layout);
	}

	set_image_layout(vk_buffer, dst_image, 0, VK_REMAINING_MIP_LEVELS, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                 static_cast<VkImageLayout>(dst_layout));
}

void UtilBlitImage(CommandBuffer* buffer, VulkanImage* src_image, VulkanSwapchain* dst_swapchain)
{
	EXIT_IF(src_image == nullptr);
	EXIT_IF(src_image->image == nullptr);
	EXIT_IF(dst_swapchain == nullptr);

	auto* vk_buffer = buffer->GetPool()->buffers[buffer->GetIndex()];

	VulkanImage swapchain_image(VulkanImageType::Unknown);

	swapchain_image.image  = dst_swapchain->swapchain_images[dst_swapchain->current_index];
	swapchain_image.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	set_image_layout(vk_buffer, src_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	set_image_layout(vk_buffer, &swapchain_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkImageBlit region {};
	region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel       = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount     = 1;
	region.srcOffsets[0].x               = 0;
	region.srcOffsets[0].y               = 0;
	region.srcOffsets[0].z               = 0;
	region.srcOffsets[1].x               = static_cast<int>(src_image->extent.width);
	region.srcOffsets[1].y               = static_cast<int>(src_image->extent.height);
	region.srcOffsets[1].z               = 1;
	region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel       = 0;
	region.dstSubresource.baseArrayLayer = 0;
	region.dstSubresource.layerCount     = 1;
	region.dstOffsets[0].x               = 0;
	region.dstOffsets[0].y               = 0;
	region.dstOffsets[0].z               = 0;
	region.dstOffsets[1].x               = static_cast<int>(dst_swapchain->swapchain_extent.width);
	region.dstOffsets[1].y               = static_cast<int>(dst_swapchain->swapchain_extent.height);
	region.dstOffsets[1].z               = 1;

	vkCmdBlitImage(vk_buffer, src_image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain_image.image,
	               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

	set_image_layout(vk_buffer, src_image, 0, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void VulkanCreateBuffer(GraphicContext* gctx, uint64_t size, VulkanBuffer* buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(gctx == nullptr);
	EXIT_IF(buffer == nullptr);
	EXIT_IF(buffer->buffer != nullptr);

	VkBufferCreateInfo buffer_info {};
	buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size        = size;
	buffer_info.usage       = buffer->usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(gctx->device, &buffer_info, nullptr, &buffer->buffer);
	EXIT_NOT_IMPLEMENTED(buffer->buffer == nullptr);

	vkGetBufferMemoryRequirements(gctx->device, buffer->buffer, &buffer->memory.requirements);

	bool allocated = VulkanAllocate(gctx, &buffer->memory);
	EXIT_NOT_IMPLEMENTED(!allocated);

	// vkBindBufferMemory(gctx->device, buffer->buffer, buffer->memory.memory, buffer->memory.offset);
	VulkanBindBufferMemory(gctx, buffer, &buffer->memory);
}

void VulkanDeleteBuffer(GraphicContext* gctx, VulkanBuffer* buffer)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(buffer == nullptr);
	EXIT_IF(gctx == nullptr);

	DeleteDescriptor(buffer);

	vkDestroyBuffer(gctx->device, buffer->buffer, nullptr);
	VulkanFree(gctx, &buffer->memory);
	buffer->buffer = nullptr;
}

void UtilFillImage(GraphicContext* ctx, VulkanImage* dst_image, const void* src_data, uint64_t size, uint32_t src_pitch,
                   uint64_t dst_layout)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(dst_image == nullptr);
	EXIT_IF(src_data == nullptr);

	VulkanBuffer staging_buffer {};
	staging_buffer.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer.memory.property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanCreateBuffer(ctx, size, &staging_buffer);

	void* data = nullptr;
	VulkanMapMemory(ctx, &staging_buffer.memory, &data);
	std::memcpy(data, src_data, size);
	VulkanUnmapMemory(ctx, &staging_buffer.memory);

	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();
	UtilBufferToImage(&buffer, &staging_buffer, src_pitch, dst_image, dst_layout);
	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();

	VulkanDeleteBuffer(ctx, &staging_buffer);
}

void UtilFillBuffer(GraphicContext* ctx, void* dst_data, uint64_t size, uint32_t dst_pitch, VulkanImage* src_image, uint64_t src_layout)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(ctx == nullptr);
	EXIT_IF(src_image == nullptr);
	EXIT_IF(dst_data == nullptr);

	VulkanBuffer staging_buffer {};
	staging_buffer.usage           = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	staging_buffer.memory.property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanCreateBuffer(ctx, size, &staging_buffer);

	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();
	UtilImageToBuffer(&buffer, src_image, &staging_buffer, dst_pitch, src_layout);
	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();

	void* data = nullptr;
	VulkanMapMemory(ctx, &staging_buffer.memory, &data);
	std::memcpy(dst_data, data, size);
	VulkanUnmapMemory(ctx, &staging_buffer.memory);

	VulkanDeleteBuffer(ctx, &staging_buffer);
}

void UtilSetDepthLayoutOptimal(DepthStencilVulkanImage* image)
{
	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();

	auto* vk_buffer = buffer.GetPool()->buffers[buffer.GetIndex()];

	VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;

	if (image->format == VK_FORMAT_D24_UNORM_S8_UINT || image->format == VK_FORMAT_D32_SFLOAT_S8_UINT)
	{
		aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	set_image_layout(vk_buffer, image, 0, 1, aspect_mask, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();
}

void UtilSetImageLayoutOptimal(VulkanImage* image)
{
	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();

	auto* vk_buffer = buffer.GetPool()->buffers[buffer.GetIndex()];

	VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

	set_image_layout(vk_buffer, image, 0, 1, aspect_mask, image->layout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();
}

void UtilFillImage(GraphicContext* ctx, VulkanImage* image, const void* src_data, uint64_t size, const Vector<BufferImageCopy>& regions,
                   uint64_t dst_layout)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(image == nullptr);

	VulkanBuffer staging_buffer {};
	staging_buffer.usage           = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer.memory.property = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanCreateBuffer(ctx, size, &staging_buffer);

	void* data = nullptr;
	VulkanMapMemory(ctx, &staging_buffer.memory, &data);
	std::memcpy(data, src_data, size);
	VulkanUnmapMemory(ctx, &staging_buffer.memory);

	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();
	UtilBufferToImage(&buffer, &staging_buffer, image, regions, dst_layout);
	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();

	VulkanDeleteBuffer(ctx, &staging_buffer);
}

void UtilFillImage(GraphicContext* ctx, const Vector<ImageImageCopy>& regions, VulkanImage* dst_image, uint64_t dst_layout)
{
	EXIT_IF(ctx == nullptr);
	EXIT_IF(dst_image == nullptr);

	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	buffer.Begin();
	UtilImageToImage(&buffer, regions, dst_image, dst_layout);
	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();
}

void UtilCopyBuffer(VulkanBuffer* src_buffer, VulkanBuffer* dst_buffer, uint64_t size)
{
	EXIT_IF(src_buffer == nullptr);
	EXIT_IF(src_buffer->buffer == nullptr);
	EXIT_IF(dst_buffer == nullptr);
	EXIT_IF(dst_buffer->buffer == nullptr);

	CommandBuffer buffer(GraphicContext::QUEUE_UTIL);
	// buffer.SetQueue(GraphicContext::QUEUE_UTIL);

	EXIT_NOT_IMPLEMENTED(buffer.IsInvalid());

	auto* vk_buffer = buffer.GetPool()->buffers[buffer.GetIndex()];

	buffer.Begin();

	VkBufferCopy copy_region {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size      = size;

	vkCmdCopyBuffer(vk_buffer, src_buffer->buffer, dst_buffer->buffer, 1, &copy_region);

	buffer.End();
	buffer.Execute();
	buffer.WaitForFence();
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
