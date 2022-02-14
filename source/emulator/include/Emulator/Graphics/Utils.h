#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_UTILS_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_UTILS_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty {
template <typename T>
class Vector;
} // namespace Kyty

namespace Kyty::Libs::Graphics {

class CommandBuffer;
struct GraphicContext;
struct VulkanBuffer;
struct VulkanImage;
struct TextureVulkanImage;
struct DepthStencilVulkanImage;
struct VulkanSwapchain;

struct BufferImageCopy
{
	uint32_t offset;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
};

void UtilBufferToImage(CommandBuffer* buffer, VulkanBuffer* src_buffer, uint32_t src_pitch, VulkanImage* dst_image);
void UtilBufferToImage(CommandBuffer* buffer, VulkanBuffer* src_buffer, TextureVulkanImage* dst_image,
                       const Vector<BufferImageCopy>& regions, uint64_t dst_layout);
void UtilBlitImage(CommandBuffer* buffer, VulkanImage* src_image, VulkanSwapchain* dst_swapchain);
void UtilFillImage(GraphicContext* ctx, VulkanImage* dst_image, const void* src_data, uint64_t size, uint32_t src_pitch);
void UtilFillImage(GraphicContext* ctx, TextureVulkanImage* dst_image, const void* src_data, uint64_t size,
                   const Vector<BufferImageCopy>& regions, uint64_t dst_layout);
void UtilCopyBuffer(VulkanBuffer* src_buffer, VulkanBuffer* dst_buffer, uint64_t size);
void UtilSetDepthLayoutOptimal(DepthStencilVulkanImage* image);
void UtilSetImageLayoutOptimal(VulkanImage* image);

void VulkanCreateBuffer(GraphicContext* gctx, uint64_t size, VulkanBuffer* buffer);
void VulkanDeleteBuffer(GraphicContext* gctx, VulkanBuffer* buffer);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_UTILS_H_ */
