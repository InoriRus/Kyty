#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICCONTEXT_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICCONTEXT_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Common.h"

#include <vulkan/vulkan_core.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

struct VulkanSwapchain
{
	VkSwapchainKHR swapchain                  = nullptr;
	VkFormat       swapchain_format           = VK_FORMAT_UNDEFINED;
	VkExtent2D     swapchain_extent           = {};
	VkImage*       swapchain_images           = nullptr;
	VkImageView*   swapchain_image_views      = nullptr;
	uint32_t       swapchain_images_count     = 0;
	VkSemaphore    present_complete_semaphore = nullptr;
	VkFence        present_complete_fence     = nullptr;
	uint32_t       current_index              = 0;
};

struct VulkanCommandPool
{
	Core::Mutex      mutex;
	VkCommandPool    pool          = nullptr;
	VkCommandBuffer* buffers       = nullptr;
	VkFence*         fences        = nullptr;
	VkSemaphore*     semaphores    = nullptr;
	bool*            busy          = nullptr;
	uint32_t         buffers_count = 0;
};

struct GraphicContext
{
	static constexpr int QUEUES_NUM          = 11;
	static constexpr int QUEUE_GFX           = 8;
	static constexpr int QUEUE_UTIL          = 9;
	static constexpr int QUEUE_PRESENT       = 10;
	static constexpr int QUEUE_COMPUTE_START = 0;
	static constexpr int QUEUE_COMPUTE_NUM   = 8;

	uint32_t                 screen_width       = 0;
	uint32_t                 screen_height      = 0;
	VkInstance               instance           = nullptr;
	VkDebugUtilsMessengerEXT debug_messenger    = nullptr;
	VkPhysicalDevice         physical_device    = nullptr;
	VkDevice                 device             = nullptr;
	uint32_t                 queue_family_index = static_cast<uint32_t>(-1);
	VkQueue                  queue[QUEUES_NUM]  = {};
};

struct VulkanMemory
{
	VkMemoryRequirements  requirements = {};
	VkMemoryPropertyFlags property     = 0;
	VkDeviceMemory        memory       = nullptr;
	VkDeviceSize          offset       = 0;
	uint32_t              type         = 0;
	uint64_t              unique_id    = 0;
};

struct VulkanImage
{
	VkFormat               format     = VK_FORMAT_UNDEFINED;
	VkExtent2D             extent     = {};
	VkImage                image      = nullptr;
	VkImageView            image_view = nullptr;
	VkImageLayout          layout     = VK_IMAGE_LAYOUT_UNDEFINED;
	Graphics::VulkanMemory memory;
};

struct VideoOutVulkanImage: public VulkanImage
{
};

struct DepthStencilVulkanImage: public VulkanImage
{
};

struct TextureVulkanImage: public VulkanImage
{
};

struct RenderTextureVulkanImage: public VulkanImage
{
};

struct VulkanBuffer
{
	VkBuffer           buffer = nullptr;
	VulkanMemory       memory;
	VkBufferUsageFlags usage = 0;
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_GRAPHICCONTEXT_H_ */
