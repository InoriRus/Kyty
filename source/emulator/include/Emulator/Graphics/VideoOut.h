#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_VIDEOOUT_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_VIDEOOUT_H_

#include "Kyty/Core/Common.h"
//#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/EventQueue.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {
// struct VulkanSwapchain;
struct VideoOutVulkanImage;
} // namespace Kyty::Libs::Graphics

namespace Kyty::Libs::VideoOut {

struct VideoOutResolutionStatus;
struct VideoOutBufferAttribute;
struct VideoOutBufferAttribute2;
struct VideoOutFlipStatus;
struct VideoOutVblankStatus;
struct VideoOutBuffers;

struct VideoOutBufferImageInfo
{
	Graphics::VideoOutVulkanImage* image        = nullptr;
	uint32_t                       index        = static_cast<uint32_t>(-1);
	uint64_t                       buffer_size  = 0;
	uint64_t                       buffer_pitch = 0;
};

void                    VideoOutInit(uint32_t width, uint32_t height);
VideoOutBufferImageInfo VideoOutGetImage(uint64_t addr);
void                    VideoOutWaitFlipDone(int handle, int index);

KYTY_SYSV_ABI int  VideoOutOpen(int user_id, int bus_type, int index, const void* param);
KYTY_SYSV_ABI int  VideoOutClose(int handle);
KYTY_SYSV_ABI int  VideoOutGetResolutionStatus(int handle, VideoOutResolutionStatus* status);
KYTY_SYSV_ABI void VideoOutSetBufferAttribute(VideoOutBufferAttribute* attribute, uint32_t pixel_format, uint32_t tiling_mode,
                                              uint32_t aspect_ratio, uint32_t width, uint32_t height, uint32_t pitch_in_pixel);
KYTY_SYSV_ABI void VideoOutSetBufferAttribute2(VideoOutBufferAttribute2* attribute, uint64_t pixel_format, uint32_t tiling_mode,
                                               uint32_t width, uint32_t height, uint64_t option, uint32_t dcc_control,
                                               uint64_t dcc_cb_register_clear_color);
KYTY_SYSV_ABI int  VideoOutSetFlipRate(int handle, int rate);
KYTY_SYSV_ABI int  VideoOutAddFlipEvent(LibKernel::EventQueue::KernelEqueue eq, int handle, void* udata);
KYTY_SYSV_ABI int  VideoOutAddVblankEvent(LibKernel::EventQueue::KernelEqueue eq, int handle, void* udata);
KYTY_SYSV_ABI int  VideoOutRegisterBuffers(int handle, int start_index, void* const* addresses, int buffer_num,
                                           const VideoOutBufferAttribute* attribute);
KYTY_SYSV_ABI int  VideoOutRegisterBuffers2(int handle, int set_index, int buffer_index_start, const VideoOutBuffers* buffers,
                                            int buffer_num, const VideoOutBufferAttribute2* attribute, int category, void* option);
KYTY_SYSV_ABI int  VideoOutSubmitFlip(int handle, int index, int flip_mode, int64_t flip_arg);
KYTY_SYSV_ABI int  VideoOutGetFlipStatus(int handle, VideoOutFlipStatus* status);
KYTY_SYSV_ABI int  VideoOutGetVblankStatus(int handle, VideoOutVblankStatus* status);
KYTY_SYSV_ABI int  VideoOutSetWindowModeMargins(int handle, int top, int bottom);

void VideoOutBeginVblank();
void VideoOutEndVblank();
bool VideoOutFlipWindow(uint32_t micros);

} // namespace Kyty::Libs::VideoOut

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_VIDEOOUT_H_ */
