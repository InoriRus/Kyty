#include "Emulator/Graphics/VideoOut.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/LinkList.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"
#include "Emulator/Config.h"
#include "Emulator/Graphics/GpuMemory.h"
#include "Emulator/Graphics/GraphicsRender.h"
#include "Emulator/Graphics/Tile.h"
#include "Emulator/Graphics/VideoOutBuffer.h"
#include "Emulator/Graphics/Window.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {
struct GraphicContext;
} // namespace Kyty::Libs::Graphics

namespace Kyty::Libs::VideoOut {

LIB_NAME("VideoOut", "VideoOut");

namespace EventQueue = LibKernel::EventQueue;

constexpr int VIDEO_OUT_EVENT_FLIP             = 0;
constexpr int VIDEO_OUT_EVENT_VBLANK           = 1;
constexpr int VIDEO_OUT_EVENT_PRE_VBLANK_START = 2;

struct VideoOutResolutionStatus
{
	uint32_t fullWidth        = 1280;
	uint32_t fullHeight       = 720;
	uint32_t paneWidth        = 1280;
	uint32_t paneHeight       = 720;
	uint64_t refreshRate      = 3;
	float    screenSizeInInch = 50;
	uint16_t flags            = 0;
	uint16_t reserved0        = 0;
	uint32_t reserved1[3]     = {0};
};

struct VideoOutBufferAttribute
{
	uint32_t pixelFormat;
	uint32_t tilingMode;
	uint32_t aspectRatio;
	uint32_t width;
	uint32_t height;
	uint32_t pitchInPixel;
	uint32_t option;
	uint32_t reserved0;
	uint64_t reserved1;
};

struct VideoOutFlipStatus
{
	uint64_t count          = 0;
	uint64_t processTime    = 0;
	uint64_t tsc            = 0;
	int64_t  flipArg        = 0;
	uint64_t submitTsc      = 0;
	uint64_t reserved0      = 0;
	int32_t  gcQueueNum     = 0;
	int32_t  flipPendingNum = 0;
	int32_t  currentBuffer  = 0;
	uint32_t reserved1      = 0;
};

struct VideoOutVblankStatus
{
	uint64_t count       = 0;
	uint64_t processTime = 0;
	uint64_t tsc         = 0;
	uint64_t reserved[1] = {0};
	uint8_t  flags       = 0;
	uint8_t  pad1[7]     = {};
};

struct VideoOutBufferSet
{
	VideoOutBufferAttribute attr        = {};
	int                     start_index = 0;
	int                     num         = 0;
};

struct VideoOutBufferInfo
{
	void*                          buffer        = nullptr;
	Graphics::VideoOutVulkanImage* buffer_vulkan = nullptr;
	uint64_t                       buffer_size   = 0;
	uint64_t                       buffer_pitch  = 0;
	int                            set_id        = 0;
};

struct VideoOutConfig
{
	Core::Mutex                      mutex;
	VideoOutResolutionStatus         resolution;
	bool                             opened    = false;
	int                              flip_rate = 0;
	Vector<EventQueue::KernelEqueue> flip_eqs;
	Vector<EventQueue::KernelEqueue> pre_vblank_eqs;
	Vector<EventQueue::KernelEqueue> vblank_eqs;
	VideoOutFlipStatus               flip_status;
	VideoOutVblankStatus             pre_vblank_status;
	VideoOutVblankStatus             vblank_status;
	VideoOutBufferInfo               buffers[16];
	VideoOutBufferSet                buffers_sets[16];
	int                              buffers_sets_num = 0;
};

class FlipQueue
{
public:
	FlipQueue() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~FlipQueue() { KYTY_NOT_IMPLEMENTED; }
	KYTY_CLASS_NO_COPY(FlipQueue);

	bool Submit(VideoOutConfig* cfg, int index, int64_t flip_arg);
	bool Flip(uint32_t micros);
	void GetFlipStatus(VideoOutConfig* cfg, VideoOutFlipStatus* out);
	void Wait(VideoOutConfig* cfg, int index);

private:
	struct Request
	{
		VideoOutConfig* cfg;
		int             index;
		int64_t         flip_arg;
		uint64_t        submit_tsc;
	};

	Core::Mutex         m_mutex;
	Core::CondVar       m_submit_cond_var;
	Core::CondVar       m_done_cond_var;
	Core::List<Request> m_requests;
};

class VideoOutContext
{
public:
	static constexpr int VIDEO_OUT_NUM_MAX = 2;

	VideoOutContext() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~VideoOutContext() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(VideoOutContext);

	int             Open();
	void            Close(int handle);
	VideoOutConfig* Get(int handle);

	VideoOutBufferImageInfo FindImage(void* buffer);

	void Init(uint32_t width, uint32_t height);

	Graphics::GraphicContext* GetGraphicCtx()
	{
		Core::LockGuard lock(m_mutex);

		if (m_graphic_ctx == nullptr)
		{
			m_graphic_ctx = Graphics::WindowGetGraphicContext();
		}

		return m_graphic_ctx;
	}

	FlipQueue& GetFlipQueue() { return m_flip_queue; }

	void VblankBegin();
	void VblankEnd();

private:
	Core::Mutex               m_mutex;
	VideoOutConfig            m_video_out_ctx[VIDEO_OUT_NUM_MAX];
	Graphics::GraphicContext* m_graphic_ctx = nullptr;
	FlipQueue                 m_flip_queue;
};

static VideoOutContext* g_video_out_context = nullptr;

static void calc_buffer_size(const VideoOutBufferAttribute* attribute, uint64_t* size, uint64_t* pitch)
{
	EXIT_IF(size == nullptr);
	EXIT_IF(pitch == nullptr);

	bool     tile   = attribute->tilingMode == 0;
	bool     neo    = Config::IsNeo();
	uint32_t width  = attribute->width;
	uint32_t height = attribute->height;

	EXIT_NOT_IMPLEMENTED(attribute->width != attribute->pitchInPixel);
	EXIT_NOT_IMPLEMENTED(attribute->option != 0);
	EXIT_NOT_IMPLEMENTED(attribute->aspectRatio != 0);
	EXIT_NOT_IMPLEMENTED(attribute->pixelFormat != 0x80000000);

	uint32_t size32  = 0;
	uint32_t pitch32 = 0;
	Graphics::TileGetVideoOutSize(width, height, tile, neo, &size32, &pitch32);

	*size  = size32;
	*pitch = pitch32;
}

void VideoOutInit(uint32_t width, uint32_t height)
{
	EXIT_IF(g_video_out_context != nullptr);

	g_video_out_context = new VideoOutContext;

	g_video_out_context->Init(width, height);
}

void VideoOutContext::Init(uint32_t width, uint32_t height)
{
	for (auto& ctx: m_video_out_ctx)
	{
		ctx.resolution.fullWidth  = width;
		ctx.resolution.fullHeight = height;
		ctx.resolution.paneWidth  = width;
		ctx.resolution.paneHeight = height;
	}
}

int VideoOutContext::Open()
{
	Core::LockGuard lock(m_mutex);

	int handle = -1;

	for (int i = 1; i < VIDEO_OUT_NUM_MAX; i++)
	{
		if (!m_video_out_ctx[i].opened)
		{
			handle = i;
			break;
		}
	}

	EXIT_IF(!m_video_out_ctx[handle].flip_eqs.IsEmpty());
	EXIT_IF(!m_video_out_ctx[handle].pre_vblank_eqs.IsEmpty());
	EXIT_IF(!m_video_out_ctx[handle].vblank_eqs.IsEmpty());
	EXIT_IF(m_video_out_ctx[handle].flip_rate != 0);

	m_video_out_ctx[handle].opened                    = true;
	m_video_out_ctx[handle].flip_status               = VideoOutFlipStatus();
	m_video_out_ctx[handle].flip_status.flipArg       = -1;
	m_video_out_ctx[handle].flip_status.currentBuffer = -1;
	m_video_out_ctx[handle].flip_status.count         = 0;
	m_video_out_ctx[handle].pre_vblank_status         = VideoOutVblankStatus();
	m_video_out_ctx[handle].vblank_status             = VideoOutVblankStatus();

	return handle;
}

void VideoOutContext::Close(int handle)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(handle >= VIDEO_OUT_NUM_MAX);
	EXIT_NOT_IMPLEMENTED(!m_video_out_ctx[handle].opened);

	m_video_out_ctx[handle].opened = false;

	m_video_out_ctx[handle].mutex.Lock();
	for (auto& flip_eq: m_video_out_ctx[handle].flip_eqs)
	{
		if (flip_eq != nullptr)
		{
			auto result = EventQueue::KernelDeleteEvent(flip_eq, VIDEO_OUT_EVENT_FLIP, EventQueue::KERNEL_EVFILT_VIDEO_OUT);
			EXIT_NOT_IMPLEMENTED(result != OK);
		}
	}
	m_video_out_ctx[handle].flip_eqs.Clear();
	for (auto& vblank_eq: m_video_out_ctx[handle].pre_vblank_eqs)
	{
		if (vblank_eq != nullptr)
		{
			auto result = EventQueue::KernelDeleteEvent(vblank_eq, VIDEO_OUT_EVENT_VBLANK, EventQueue::KERNEL_EVFILT_VIDEO_OUT);
			EXIT_NOT_IMPLEMENTED(result != OK);
		}
	}
	m_video_out_ctx[handle].pre_vblank_eqs.Clear();
	for (auto& vblank_eq: m_video_out_ctx[handle].vblank_eqs)
	{
		if (vblank_eq != nullptr)
		{
			auto result = EventQueue::KernelDeleteEvent(vblank_eq, VIDEO_OUT_EVENT_PRE_VBLANK_START, EventQueue::KERNEL_EVFILT_VIDEO_OUT);
			EXIT_NOT_IMPLEMENTED(result != OK);
		}
	}
	m_video_out_ctx[handle].vblank_eqs.Clear();
	m_video_out_ctx[handle].mutex.Unlock();

	m_video_out_ctx[handle].flip_rate = 0;

	for (int i = 0; i < 16; i++)
	{
		m_video_out_ctx[handle].buffers[i].buffer           = nullptr;
		m_video_out_ctx[handle].buffers[i].buffer_vulkan    = nullptr;
		m_video_out_ctx[handle].buffers[i].buffer_size      = 0;
		m_video_out_ctx[handle].buffers[i].set_id           = 0;
		m_video_out_ctx[handle].buffers_sets[i].num         = 0;
		m_video_out_ctx[handle].buffers_sets[i].start_index = 0;
	}

	m_video_out_ctx[handle].buffers_sets_num = 0;
}

VideoOutConfig* VideoOutContext::Get(int handle)
{
	EXIT_NOT_IMPLEMENTED(handle >= VIDEO_OUT_NUM_MAX);
	EXIT_NOT_IMPLEMENTED(!m_video_out_ctx[handle].opened);

	return m_video_out_ctx + handle;
}

void VideoOutContext::VblankBegin()
{
	Core::LockGuard lock(m_mutex);

	for (int i = 1; i < VIDEO_OUT_NUM_MAX; i++)
	{
		auto& ctx = m_video_out_ctx[i];
		if (ctx.opened)
		{
			ctx.mutex.Lock();
			ctx.pre_vblank_status.count++;
			ctx.pre_vblank_status.processTime = LibKernel::KernelGetProcessTime();
			ctx.pre_vblank_status.tsc         = LibKernel::KernelReadTsc();

			for (auto& vblank_eq: ctx.pre_vblank_eqs)
			{
				if (vblank_eq != nullptr)
				{
					auto result = EventQueue::KernelTriggerEvent(vblank_eq, VIDEO_OUT_EVENT_VBLANK, EventQueue::KERNEL_EVFILT_VIDEO_OUT,
					                                             reinterpret_cast<void*>(ctx.pre_vblank_status.count));
					EXIT_NOT_IMPLEMENTED(result != OK);
				}
			}
			ctx.mutex.Unlock();
		}
	}
}

void VideoOutContext::VblankEnd()
{
	Core::LockGuard lock(m_mutex);

	for (int i = 1; i < VIDEO_OUT_NUM_MAX; i++)
	{
		auto& ctx = m_video_out_ctx[i];
		if (ctx.opened)
		{
			ctx.mutex.Lock();
			ctx.vblank_status.count++;
			ctx.vblank_status.processTime = LibKernel::KernelGetProcessTime();
			ctx.vblank_status.tsc         = LibKernel::KernelReadTsc();

			for (auto& vblank_eq: ctx.vblank_eqs)
			{
				if (vblank_eq != nullptr)
				{
					auto result = EventQueue::KernelTriggerEvent(vblank_eq, VIDEO_OUT_EVENT_VBLANK, EventQueue::KERNEL_EVFILT_VIDEO_OUT,
					                                             reinterpret_cast<void*>(ctx.vblank_status.count));
					EXIT_NOT_IMPLEMENTED(result != OK);
				}
			}
			ctx.mutex.Unlock();
		}
	}
}

VideoOutBufferImageInfo VideoOutContext::FindImage(void* buffer)
{
	VideoOutBufferImageInfo ret;

	Core::LockGuard lock(m_mutex);

	for (auto& ctx: m_video_out_ctx)
	{
		if (ctx.opened)
		{
			for (int i = 0; i < ctx.buffers_sets_num; i++)
			{
				for (int j = ctx.buffers_sets[i].start_index; j < ctx.buffers_sets[i].num; j++)
				{
					if (ctx.buffers[j].buffer == buffer)
					{
						ret.image        = ctx.buffers[j].buffer_vulkan;
						ret.buffer_size  = ctx.buffers[j].buffer_size;
						ret.buffer_pitch = ctx.buffers[j].buffer_pitch;
						ret.index        = j - ctx.buffers_sets[i].start_index;
						goto END;
					}
				}
			}
		}
	}
END:
	return ret;
}

bool FlipQueue::Submit(VideoOutConfig* cfg, int index, int64_t flip_arg)
{
	Core::LockGuard lock(m_mutex);

	if (m_requests.Size() >= 2)
	{
		return false;
	}

	Request r {};
	r.cfg        = cfg;
	r.index      = index;
	r.flip_arg   = flip_arg;
	r.submit_tsc = LibKernel::KernelReadTsc();

	m_requests.Add(r);

	cfg->flip_status.flipPendingNum = static_cast<int>(m_requests.Size());
	cfg->flip_status.gcQueueNum     = 0;

	m_submit_cond_var.Signal();

	return true;
}

void FlipQueue::Wait(VideoOutConfig* cfg, int index)
{
	Core::LockGuard lock(m_mutex);

	while (
	    m_requests.IndexValid(m_requests.Find(cfg, index, [](auto r, auto cfg, auto index) { return r.cfg == cfg && r.index == index; })))
	{
		m_done_cond_var.Wait(&m_mutex);
	}
}

bool FlipQueue::Flip(uint32_t micros)
{
	KYTY_PROFILER_BLOCK("FlipQueue::Flip");

	m_mutex.Lock();
	if (m_requests.Size() == 0)
	{
		m_submit_cond_var.WaitFor(&m_mutex, micros);

		if (m_requests.Size() == 0)
		{
			m_mutex.Unlock();
			return false;
		}
	}
	auto first = m_requests.First();
	auto r     = m_requests.At(first);
	m_mutex.Unlock();

	auto* buffer = r.cfg->buffers[r.index].buffer_vulkan;

	Graphics::WindowDrawBuffer(buffer);

	m_mutex.Lock();

	r.cfg->mutex.Lock();
	for (auto& flip_eq: r.cfg->flip_eqs)
	{
		if (flip_eq != nullptr)
		{
			auto result = EventQueue::KernelTriggerEvent(flip_eq, VIDEO_OUT_EVENT_FLIP, EventQueue::KERNEL_EVFILT_VIDEO_OUT,
			                                             reinterpret_cast<void*>(r.flip_arg));
			EXIT_NOT_IMPLEMENTED(result != OK);
		}
	}
	r.cfg->mutex.Unlock();

	printf("Flip done: %d\n", r.index);

	// m_mutex.Lock();

	m_requests.Remove(first);
	m_done_cond_var.Signal();

	r.cfg->flip_status.count++;
	r.cfg->flip_status.processTime    = LibKernel::KernelGetProcessTime();
	r.cfg->flip_status.tsc            = LibKernel::KernelReadTsc();
	r.cfg->flip_status.submitTsc      = r.submit_tsc;
	r.cfg->flip_status.flipArg        = r.flip_arg;
	r.cfg->flip_status.currentBuffer  = r.index;
	r.cfg->flip_status.flipPendingNum = static_cast<int>(m_requests.Size());

	m_mutex.Unlock();

	Graphics::GpuMemoryFrameDone();
	Graphics::GpuMemoryDbgDump();

	return true;
}

void FlipQueue::GetFlipStatus(VideoOutConfig* cfg, VideoOutFlipStatus* out)
{
	EXIT_IF(cfg == nullptr);
	EXIT_IF(out == nullptr);

	Core::LockGuard lock(m_mutex);

	*out = cfg->flip_status;
}

bool VideoOutFlipWindow(uint32_t micros)
{
	EXIT_IF(g_video_out_context == nullptr);

	return g_video_out_context->GetFlipQueue().Flip(micros);
}

void VideoOutBeginVblank()
{
	EXIT_IF(g_video_out_context == nullptr);

	// g_video_out_context->VblankBegin();
}

void VideoOutEndVblank()
{
	EXIT_IF(g_video_out_context == nullptr);

	// g_video_out_context->VblankEnd();
}

KYTY_SYSV_ABI int VideoOutOpen(int user_id, int bus_type, int index, const void* param)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	EXIT_NOT_IMPLEMENTED(user_id != 255 && user_id != 0);
	EXIT_NOT_IMPLEMENTED(bus_type != 0);
	EXIT_NOT_IMPLEMENTED(index != 0);
	EXIT_NOT_IMPLEMENTED(param != nullptr);

	int handle = g_video_out_context->Open();

	if (handle < 0)
	{
		return VIDEO_OUT_ERROR_RESOURCE_BUSY;
	}

	return handle;
}

KYTY_SYSV_ABI int VideoOutClose(int handle)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	g_video_out_context->Close(handle);

	return OK;
}

KYTY_SYSV_ABI int VideoOutGetResolutionStatus(int handle, VideoOutResolutionStatus* status)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	EXIT_NOT_IMPLEMENTED(status == nullptr);

	*status = g_video_out_context->Get(handle)->resolution;

	return OK;
}

KYTY_SYSV_ABI void VideoOutSetBufferAttribute(VideoOutBufferAttribute* attribute, uint32_t pixel_format, uint32_t tiling_mode,
                                              uint32_t aspect_ratio, uint32_t width, uint32_t height, uint32_t pitch_in_pixel)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attribute == nullptr);

	printf("\tpixel_format   = %08" PRIx32 "\n", pixel_format);
	printf("\ttiling_mode    = %" PRIu32 "\n", tiling_mode);
	printf("\taspect_ratio   = %" PRIu32 "\n", aspect_ratio);
	printf("\twidth          = %" PRIu32 "\n", width);
	printf("\theight         = %" PRIu32 "\n", height);
	printf("\tpitch_in_pixel = %" PRIu32 "\n", pitch_in_pixel);

	memset(attribute, 0, sizeof(VideoOutBufferAttribute));

	attribute->pixelFormat  = pixel_format;
	attribute->tilingMode   = tiling_mode;
	attribute->aspectRatio  = aspect_ratio;
	attribute->width        = width;
	attribute->height       = height;
	attribute->pitchInPixel = pitch_in_pixel;
}

KYTY_SYSV_ABI int VideoOutSetFlipRate(int handle, int rate)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	EXIT_NOT_IMPLEMENTED(rate < 0 || rate > 2);

	printf("\trate = %d\n", rate);

	g_video_out_context->Get(handle)->flip_rate = rate;

	return OK;
}

static void flip_event_reset_func(LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	event->triggered    = false;
	event->event.fflags = 0;
	event->event.data   = 0;
}

static void flip_event_delete_func(EventQueue::KernelEqueue eq, LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	EXIT_IF(event->filter.data == nullptr);

	EXIT_NOT_IMPLEMENTED(event->event.ident != VIDEO_OUT_EVENT_FLIP);
	EXIT_NOT_IMPLEMENTED(event->event.filter != EventQueue::KERNEL_EVFILT_VIDEO_OUT);

	if (event->filter.data != nullptr)
	{
		auto* video_out = static_cast<VideoOutConfig*>(event->filter.data);
		video_out->mutex.Lock();
		EXIT_IF(video_out->flip_eqs.IsEmpty());
		auto index = video_out->flip_eqs.Find(eq);
		EXIT_NOT_IMPLEMENTED(!video_out->flip_eqs.IndexValid(index));
		video_out->flip_eqs[index] = nullptr;
		video_out->mutex.Unlock();
	}
}

static void flip_event_trigger_func(LibKernel::EventQueue::KernelEqueueEvent* event, void* trigger_data)
{
	EXIT_IF(event == nullptr);
	event->triggered = true;
	event->event.fflags++;
	event->event.data = reinterpret_cast<intptr_t>(trigger_data);
}

static void vblank_event_reset_func(LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	event->triggered    = false;
	event->event.fflags = 0;
	event->event.data   = 0;
}

static void vblank_event_delete_func(EventQueue::KernelEqueue eq, LibKernel::EventQueue::KernelEqueueEvent* event)
{
	EXIT_IF(event == nullptr);
	EXIT_IF(event->filter.data == nullptr);

	EXIT_NOT_IMPLEMENTED(event->event.ident != VIDEO_OUT_EVENT_VBLANK);
	EXIT_NOT_IMPLEMENTED(event->event.filter != EventQueue::KERNEL_EVFILT_VIDEO_OUT);

	if (event->filter.data != nullptr)
	{
		auto* video_out = static_cast<VideoOutConfig*>(event->filter.data);
		video_out->mutex.Lock();
		EXIT_IF(video_out->vblank_eqs.IsEmpty());
		auto index = video_out->vblank_eqs.Find(eq);
		EXIT_NOT_IMPLEMENTED(!video_out->vblank_eqs.IndexValid(index));
		video_out->vblank_eqs[index] = nullptr;
		video_out->mutex.Unlock();
	}
}

static void vblank_event_trigger_func(LibKernel::EventQueue::KernelEqueueEvent* event, void* trigger_data)
{
	EXIT_IF(event == nullptr);
	event->triggered = true;
	event->event.fflags++;
	event->event.data = reinterpret_cast<intptr_t>(trigger_data);
}

KYTY_SYSV_ABI int VideoOutAddFlipEvent(EventQueue::KernelEqueue eq, int handle, void* udata)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	auto* ctx = g_video_out_context->Get(handle);

	ctx->mutex.Lock();

	EXIT_NOT_IMPLEMENTED(ctx->flip_eqs.Contains(eq));

	if (eq == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
	}

	EventQueue::KernelEqueueEvent event;
	event.triggered                = false;
	event.event.ident              = VIDEO_OUT_EVENT_FLIP;
	event.event.filter             = EventQueue::KERNEL_EVFILT_VIDEO_OUT;
	event.event.udata              = udata;
	event.event.fflags             = 0;
	event.event.data               = 0;
	event.filter.delete_event_func = flip_event_delete_func;
	event.filter.reset_func        = flip_event_reset_func;
	event.filter.trigger_func      = flip_event_trigger_func;
	event.filter.data              = ctx;

	int result = EventQueue::KernelAddEvent(eq, event);

	ctx->flip_eqs.Add(eq);

	ctx->mutex.Unlock();

	return result;
}

KYTY_SYSV_ABI int VideoOutAddVblankEvent(LibKernel::EventQueue::KernelEqueue eq, int handle, void* udata)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	auto* ctx = g_video_out_context->Get(handle);

	ctx->mutex.Lock();

	EXIT_NOT_IMPLEMENTED(ctx->vblank_eqs.Contains(eq));

	if (eq == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_EVENT_QUEUE;
	}

	EventQueue::KernelEqueueEvent event;
	event.triggered                = false;
	event.event.ident              = VIDEO_OUT_EVENT_VBLANK;
	event.event.filter             = EventQueue::KERNEL_EVFILT_VIDEO_OUT;
	event.event.udata              = udata;
	event.event.fflags             = 0;
	event.event.data               = 0;
	event.filter.delete_event_func = vblank_event_delete_func;
	event.filter.reset_func        = vblank_event_reset_func;
	event.filter.trigger_func      = vblank_event_trigger_func;
	event.filter.data              = ctx;

	int result = EventQueue::KernelAddEvent(eq, event);

	ctx->vblank_eqs.Add(eq);

	ctx->mutex.Unlock();

	return result;
}

KYTY_SYSV_ABI int VideoOutRegisterBuffers(int handle, int start_index, void* const* addresses, int buffer_num,
                                          const VideoOutBufferAttribute* attribute)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	auto* ctx = g_video_out_context->Get(handle);

	if (addresses == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_ADDRESS;
	}

	if (attribute == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_OPTION;
	}

	if (start_index < 0 || start_index > 15 || buffer_num < 1 || buffer_num > 16 || start_index + buffer_num > 15)
	{
		return VIDEO_OUT_ERROR_INVALID_VALUE;
	}

	Graphics::WindowWaitForGraphicInitialized();
	Graphics::GraphicsRenderCreateContext();

	int set_index = ctx->buffers_sets_num++;

	if (set_index > 15)
	{
		return VIDEO_OUT_ERROR_NO_EMPTY_SLOT;
	}

	printf("\tstart_index = %d\n", start_index);
	printf("\tbuffer_num = %d\n", buffer_num);
	printf("\tpixel_format   = 0x%08" PRIx32 "\n", attribute->pixelFormat);
	printf("\ttiling_mode    = %" PRIu32 "\n", attribute->tilingMode);
	printf("\taspect_ratio   = %" PRIu32 "\n", attribute->aspectRatio);
	printf("\twidth          = %" PRIu32 "\n", attribute->width);
	printf("\theight         = %" PRIu32 "\n", attribute->height);
	printf("\tpitch_in_pixel = %" PRIu32 "\n", attribute->pitchInPixel);
	printf("\toption = %" PRIu32 "\n", attribute->option);

	EXIT_NOT_IMPLEMENTED(attribute->pixelFormat != 0x80000000);
	EXIT_NOT_IMPLEMENTED(attribute->tilingMode != 0);
	EXIT_NOT_IMPLEMENTED(attribute->aspectRatio != 0);
	EXIT_NOT_IMPLEMENTED(attribute->pitchInPixel != attribute->width);
	EXIT_NOT_IMPLEMENTED(attribute->option != 0);

	uint64_t buffer_size  = 0;
	uint64_t buffer_pitch = 0;
	calc_buffer_size(attribute, &buffer_size, &buffer_pitch);

	EXIT_NOT_IMPLEMENTED(buffer_size == 0);
	EXIT_NOT_IMPLEMENTED(buffer_pitch == 0);

	ctx->buffers_sets[set_index].start_index = start_index;
	ctx->buffers_sets[set_index].num         = buffer_num;
	ctx->buffers_sets[set_index].attr        = *attribute;

	Graphics::VideoOutBufferObject vulkan_buffer_info(attribute->pixelFormat, attribute->width, attribute->height,
	                                                  (attribute->tilingMode == 0), Config::IsNeo(), buffer_pitch);

	for (int i = 0; i < buffer_num; i++)
	{
		if (ctx->buffers[i + start_index].buffer != nullptr)
		{
			return VIDEO_OUT_ERROR_SLOT_OCCUPIED;
		}

		ctx->buffers[i + start_index].set_id        = set_index;
		ctx->buffers[i + start_index].buffer        = addresses[i];
		ctx->buffers[i + start_index].buffer_size   = buffer_size;
		ctx->buffers[i + start_index].buffer_pitch  = buffer_pitch;
		ctx->buffers[i + start_index].buffer_vulkan = static_cast<Graphics::VideoOutVulkanImage*>(Graphics::GpuMemoryGetObject(
		    g_video_out_context->GetGraphicCtx(), reinterpret_cast<uint64_t>(addresses[i]), buffer_size, vulkan_buffer_info));

		EXIT_NOT_IMPLEMENTED(ctx->buffers[i + start_index].buffer_vulkan == nullptr);

		printf("\tbuffers[%d] = %016" PRIx64 "\n", i + start_index, reinterpret_cast<uint64_t>(addresses[i]));
	}

	// Graphics::GpuMemoryDbgDump();

	return set_index;
}

VideoOutBufferImageInfo VideoOutGetImage(uint64_t addr)
{
	EXIT_IF(g_video_out_context == nullptr);

	return g_video_out_context->FindImage(reinterpret_cast<void*>(addr));
}

KYTY_SYSV_ABI int VideoOutSubmitFlip(int handle, int index, int flip_mode, int64_t flip_arg)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	auto* ctx = g_video_out_context->Get(handle);

	EXIT_NOT_IMPLEMENTED(flip_mode != 1);

	if (index < 0 || index > 15)
	{
		return VIDEO_OUT_ERROR_INVALID_INDEX;
	}

	if (!g_video_out_context->GetFlipQueue().Submit(ctx, index, flip_arg))
	{
		return VIDEO_OUT_ERROR_FLIP_QUEUE_FULL;
	}

	return OK;
}

void VideoOutWaitFlipDone(int handle, int index)
{
	EXIT_IF(g_video_out_context == nullptr);

	auto* ctx = g_video_out_context->Get(handle);

	EXIT_NOT_IMPLEMENTED(index < 0 || index > 15);

	g_video_out_context->GetFlipQueue().Wait(ctx, index);
}

KYTY_SYSV_ABI int VideoOutGetFlipStatus(int handle, VideoOutFlipStatus* status)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	if (status == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_ADDRESS;
	}

	auto* ctx = g_video_out_context->Get(handle);

	g_video_out_context->GetFlipQueue().GetFlipStatus(ctx, status);

	printf("\t count = %" PRIu64 "\n", status->count);
	printf("\t processTime = %" PRIu64 "\n", status->processTime);
	printf("\t tsc = %" PRIu64 "\n", status->tsc);
	printf("\t submitTsc = %" PRIu64 "\n", status->submitTsc);
	printf("\t flipArg = %" PRId64 "\n", status->flipArg);
	printf("\t gcQueueNum = %d\n", status->gcQueueNum);
	printf("\t flipPendingNum = %d\n", status->flipPendingNum);
	printf("\t currentBuffer = %d\n", status->currentBuffer);

	return OK;
}

KYTY_SYSV_ABI int VideoOutGetVblankStatus(int handle, VideoOutVblankStatus* status)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	if (status == nullptr)
	{
		return VIDEO_OUT_ERROR_INVALID_ADDRESS;
	}

	auto* ctx = g_video_out_context->Get(handle);

	ctx->mutex.Lock();
	*status = ctx->vblank_status;
	ctx->mutex.Unlock();

	printf("\t count = %" PRIu64 "\n", status->count);
	printf("\t processTime = %" PRIu64 "\n", status->processTime);
	printf("\t tsc = %" PRIu64 "\n", status->tsc);

	return OK;
}

KYTY_SYSV_ABI int VideoOutSetWindowModeMargins(int handle, int top, int bottom)
{
	PRINT_NAME();

	EXIT_IF(g_video_out_context == nullptr);

	[[maybe_unused]] auto* ctx = g_video_out_context->Get(handle);

	printf("\t top    = %d\n", top);
	printf("\t bottom = %d\n", bottom);

	return OK;
}

} // namespace Kyty::Libs::VideoOut

#endif // KYTY_EMU_ENABLED
