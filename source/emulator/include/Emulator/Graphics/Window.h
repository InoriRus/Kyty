#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_WINDOW_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_WINDOW_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

struct VkSurfaceCapabilitiesKHR;

namespace Kyty::Libs::Graphics {

struct GraphicContext;
struct VideoOutVulkanImage;

VkSurfaceCapabilitiesKHR* VulkanGetSurfaceCapabilities();

GraphicContext* WindowGetGraphicContext();

void WindowInit(uint32_t width, uint32_t height);
void WindowRun();
void WindowWaitForGraphicInitialized();
void WindowDrawBuffer(VideoOutVulkanImage* image);

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_WINDOW_H_ */
