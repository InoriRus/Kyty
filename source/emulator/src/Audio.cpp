#include "Emulator/Audio.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Audio {

class Audio
{
public:
	enum class Format
	{
		Unknown,
		Signed16bitMono,
		Signed16bitStereo,
		Signed16bit8Ch,
		FloatMono,
		FloatStereo,
		Float8Ch,
		Signed16bit8ChStd,
		Float8ChStd,
	};

	class Id
	{
	public:
		explicit Id(int id): m_id(id - 1) {}
		[[nodiscard]] int  ToInt() const { return m_id + 1; }
		[[nodiscard]] bool IsValid() const { return m_id >= 0; }

		friend class Audio;

	private:
		Id() = default;
		static Id Invalid() { return Id(); }
		static Id Create(int audio_id)
		{
			Id r;
			r.m_id = audio_id;
			return r;
		}
		[[nodiscard]] int GetId() const { return m_id; }

		int m_id = -1;
	};

	struct OutputParam
	{
		Id          handle;
		const void* data = nullptr;
	};

	Audio()          = default;
	virtual ~Audio() = default;

	KYTY_CLASS_NO_COPY(Audio);

	Id       AudioOutOpen(int type, uint32_t samples_num, uint32_t freq, Format format);
	bool     AudioOutClose(Id handle);
	bool     AudioOutValid(Id handle);
	bool     AudioOutSetVolume(Id handle, uint32_t bitflag, const int* volume);
	uint32_t AudioOutOutputs(OutputParam* params, uint32_t num);

	Id       AudioInOpen(uint32_t type, uint32_t samples_num, uint32_t freq, Format format);
	bool     AudioInValid(Id handle);
	uint32_t AudioInInput(Id handle, void* dest);

	static constexpr int OUT_PORTS_MAX = 32;
	static constexpr int IN_PORTS_MAX  = 8;

private:
	struct PortOut
	{
		bool     used             = false;
		int      type             = 0;
		uint32_t samples_num      = 0;
		uint32_t freq             = 0;
		Format   format           = Format::Unknown;
		uint64_t last_output_time = 0;
		int      channels_num     = 0;
		int      volume[8]        = {};
	};

	struct PortIn
	{
		bool     used            = false;
		uint32_t type            = 0;
		uint32_t samples_num     = 0;
		uint32_t freq            = 0;
		Format   format          = Format::Unknown;
		uint64_t last_input_time = 0;
	};

	Core::Mutex m_mutex;
	PortOut     m_out_ports[OUT_PORTS_MAX];
	PortIn      m_in_ports[IN_PORTS_MAX];
};

static Audio* g_audio = nullptr;

KYTY_SUBSYSTEM_INIT(Audio)
{
	EXIT_IF(g_audio != nullptr);

	g_audio = new Audio;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Audio) {}

KYTY_SUBSYSTEM_DESTROY(Audio) {}

Audio::Id Audio::AudioOutOpen(int type, uint32_t samples_num, uint32_t freq, Format format)
{
	Core::LockGuard lock(m_mutex);

	for (int id = 0; id < OUT_PORTS_MAX; id++)
	{
		if (!m_out_ports[id].used)
		{
			auto& port = m_out_ports[id];

			port.used             = true;
			port.type             = type;
			port.samples_num      = samples_num;
			port.freq             = freq;
			port.format           = format;
			port.last_output_time = 0;

			switch (format)
			{
				case Format::Signed16bitMono:
				case Format::FloatMono: port.channels_num = 1; break;
				case Format::Signed16bitStereo:
				case Format::FloatStereo: port.channels_num = 2; break;
				case Format::Signed16bit8Ch:
				case Format::Float8Ch:
				case Format::Signed16bit8ChStd:
				case Format::Float8ChStd: port.channels_num = 8; break;
				default: EXIT("unknown format");
			}

			for (int i = 0; i < port.channels_num; i++)
			{
				port.volume[i] = 32768;
			}

			return Id::Create(id);
		}
	}

	return Id::Invalid();
}

bool Audio::AudioOutClose(Id handle)
{
	Core::LockGuard lock(m_mutex);

	if (AudioOutValid(handle))
	{
		m_out_ports[handle.GetId()].used = false;
		return true;
	}

	return false;
}

bool Audio::AudioOutValid(Id handle)
{
	Core::LockGuard lock(m_mutex);

	return (handle.GetId() >= 0 && handle.GetId() < OUT_PORTS_MAX && m_out_ports[handle.GetId()].used);
}

bool Audio::AudioOutSetVolume(Id handle, uint32_t bitflag, const int* volume)
{
	Core::LockGuard lock(m_mutex);

	if (AudioOutValid(handle))
	{
		auto& port = m_out_ports[handle.GetId()];

		for (int i = 0; i < port.channels_num; i++, bitflag >>= 1u)
		{
			auto bit = bitflag & 0x1u;

			if (bit == 1)
			{
				int src_index = i;
				if (port.format == Format::Float8ChStd || port.format == Format::Signed16bit8ChStd)
				{
					switch (i)
					{
						case 4: src_index = 6; break;
						case 5: src_index = 7; break;
						case 6: src_index = 4; break;
						case 7: src_index = 5; break;
						default:;
					}
				}
				port.volume[i] = volume[src_index];

				printf("\t port.volume[%d] = volume[%d] (%d)\n", i, src_index, volume[src_index]);
			}
		}

		return true;
	}

	return false;
}

uint32_t Audio::AudioOutOutputs(OutputParam* params, uint32_t num)
{
	EXIT_NOT_IMPLEMENTED(num == 0);
	EXIT_NOT_IMPLEMENTED(!AudioOutValid(params[0].handle));

	const auto& first_port = m_out_ports[params[0].handle.GetId()];

	uint64_t block_time   = (params->data != nullptr ? (1000000 * first_port.samples_num) / first_port.freq : 0);
	uint64_t current_time = LibKernel::KernelGetProcessTime();

	uint64_t max_wait_time = 0;

	for (uint32_t i = 0; i < num; i++)
	{
		uint64_t next_time = m_out_ports[params[i].handle.GetId()].last_output_time + block_time;
		uint64_t wait_time = (next_time > current_time ? next_time - current_time : 0);
		max_wait_time      = (wait_time > max_wait_time ? wait_time : max_wait_time);
	}

	// TODO(): Audio output is not yet implemented, so simulate audio delay
	Core::Thread::SleepMicro(max_wait_time);

	for (uint32_t i = 0; i < num; i++)
	{
		m_out_ports[params[i].handle.GetId()].last_output_time = LibKernel::KernelGetProcessTime();
	}

	return first_port.samples_num;
}

Audio::Id Audio::AudioInOpen(uint32_t type, uint32_t samples_num, uint32_t freq, Format format)
{
	Core::LockGuard lock(m_mutex);

	for (int id = 0; id < IN_PORTS_MAX; id++)
	{
		if (!m_in_ports[id].used)
		{
			auto& port = m_in_ports[id];

			port.used        = true;
			port.type        = type;
			port.samples_num = samples_num;
			port.freq        = freq;
			port.format      = format;

			switch (format)
			{
				case Format::Signed16bitMono:
				case Format::Signed16bitStereo: break;
				default: EXIT("unknown format");
			}

			return Id::Create(id);
		}
	}

	return Id::Invalid();
}

bool Audio::AudioInValid(Id handle)
{
	Core::LockGuard lock(m_mutex);

	return (handle.GetId() >= 0 && handle.GetId() < IN_PORTS_MAX && m_in_ports[handle.GetId()].used);
}

uint32_t Audio::AudioInInput(Id handle, void* dest)
{
	EXIT_NOT_IMPLEMENTED(!AudioInValid(handle));
	EXIT_NOT_IMPLEMENTED(dest == nullptr);

	const auto& port = m_in_ports[handle.GetId()];

	uint64_t block_time   = (1000000 * port.samples_num) / port.freq;
	uint64_t current_time = LibKernel::KernelGetProcessTime();

	uint64_t next_time = m_in_ports[handle.GetId()].last_input_time + block_time;
	uint64_t wait_time = (next_time > current_time ? next_time - current_time : 0);

	// TODO(): Audio input is not yet implemented, so simulate audio delay
	Core::Thread::SleepMicro(wait_time);

	m_in_ports[handle.GetId()].last_input_time = LibKernel::KernelGetProcessTime();

	return port.samples_num;
}

namespace AudioOut {

LIB_NAME("AudioOut", "AudioOut");

struct AudioOutOutputParam
{
	int         handle;
	const void* ptr;
};

int KYTY_SYSV_ABI AudioOutInit()
{
	PRINT_NAME();

	return OK;
}

int KYTY_SYSV_ABI AudioOutOpen(int user_id, int type, int index, uint32_t len, uint32_t freq, uint32_t param)
{
	PRINT_NAME();

	printf("\t user_id = %d\n", user_id);
	printf("\t type    = %d\n", type);
	printf("\t index   = %d\n", index);
	printf("\t len     = %u\n", len);
	printf("\t freq    = %u\n", freq);

	EXIT_NOT_IMPLEMENTED(user_id != 255 && user_id != 1);
	EXIT_NOT_IMPLEMENTED(type != 0 && type != 3 && type != 4);
	EXIT_NOT_IMPLEMENTED(index != 0);

	Audio::Format format = Audio::Format::Unknown;

	switch (param)
	{
		case 0: format = Audio::Format::Signed16bitMono; break;
		case 1: format = Audio::Format::Signed16bitStereo; break;
		case 2: format = Audio::Format::Signed16bit8Ch; break;
		case 3: format = Audio::Format::FloatMono; break;
		case 4: format = Audio::Format::FloatStereo; break;
		case 5: format = Audio::Format::Float8Ch; break;
		case 6: format = Audio::Format::Signed16bit8ChStd; break;
		case 7: format = Audio::Format::Float8ChStd; break;
		default:;
	}

	printf("\t param   = %u (%s)\n", param, Core::EnumName(format).C_Str());

	EXIT_NOT_IMPLEMENTED(format == Audio::Format::Unknown);

	EXIT_IF(g_audio == nullptr);

	auto id = g_audio->AudioOutOpen(type, len, freq, format);

	if (!id.IsValid())
	{
		return AUDIO_OUT_ERROR_PORT_FULL;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI AudioOutClose(int handle)
{
	PRINT_NAME();

	if (!g_audio->AudioOutClose(Audio::Id(handle)))
	{
		return AUDIO_OUT_ERROR_INVALID_PORT;
	}

	return OK;
}

int KYTY_SYSV_ABI AudioOutSetVolume(int handle, uint32_t flag, int* vol)
{
	PRINT_NAME();

	printf("\t handle = %d\n", handle);
	printf("\t flag   = %u\n", flag);

	EXIT_IF(g_audio == nullptr);
	EXIT_NOT_IMPLEMENTED(vol == nullptr);

	if (!g_audio->AudioOutSetVolume(Audio::Id(handle), flag, vol))
	{
		return AUDIO_OUT_ERROR_INVALID_PORT;
	}

	return OK;
}

int KYTY_SYSV_ABI AudioOutOutputs(AudioOutOutputParam* param, uint32_t num)
{
	PRINT_NAME();

	for (uint32_t i = 0; i < num; i++)
	{
		printf("\t handle[%u] = %d\n", i, param[i].handle);
	}

	EXIT_NOT_IMPLEMENTED(param == nullptr);

	Audio::OutputParam params[Audio::OUT_PORTS_MAX];

	EXIT_IF(g_audio == nullptr);

	for (uint32_t i = 0; i < num; i++)
	{
		params[i].handle = Audio::Id(param[i].handle);
		params[i].data   = param[i].ptr;

		if (!g_audio->AudioOutValid(params[i].handle))
		{
			return AUDIO_OUT_ERROR_INVALID_PORT;
		}
	}

	return static_cast<int>(g_audio->AudioOutOutputs(params, num));
}

int KYTY_SYSV_ABI AudioOutOutput(int handle, const void* ptr)
{
	PRINT_NAME();

	printf("\t handle = %d\n", handle);

	// EXIT_NOT_IMPLEMENTED(ptr == nullptr);

	Audio::OutputParam params[1];

	EXIT_IF(g_audio == nullptr);

	params[0].handle = Audio::Id(handle);
	params[0].data   = ptr;

	if (!g_audio->AudioOutValid(params[0].handle))
	{
		return AUDIO_OUT_ERROR_INVALID_PORT;
	}

	return static_cast<int>(g_audio->AudioOutOutputs(params, 1));
}

} // namespace AudioOut

namespace AudioIn {

LIB_NAME("AudioIn", "AudioIn");

int KYTY_SYSV_ABI AudioInOpen(int user_id, uint32_t type, uint32_t index, uint32_t len, uint32_t freq, uint32_t param)
{
	PRINT_NAME();

	printf("\t user_id = %d\n", user_id);
	printf("\t type    = %u\n", type);
	printf("\t index   = %d\n", index);
	printf("\t len     = %u\n", len);
	printf("\t freq    = %u\n", freq);

	EXIT_NOT_IMPLEMENTED(user_id != 255 && user_id != 1);
	EXIT_NOT_IMPLEMENTED(type != 1);
	EXIT_NOT_IMPLEMENTED(index != 0);

	Audio::Format format = Audio::Format::Unknown;

	switch (param)
	{
		case 0: format = Audio::Format::Signed16bitMono; break;
		case 2: format = Audio::Format::Signed16bitStereo; break;
		default:;
	}

	printf("\t param   = %u (%s)\n", param, Core::EnumName(format).C_Str());

	EXIT_NOT_IMPLEMENTED(format == Audio::Format::Unknown);

	EXIT_IF(g_audio == nullptr);

	auto id = g_audio->AudioInOpen(type, len, freq, format);

	if (!id.IsValid())
	{
		return AUDIO_IN_ERROR_PORT_FULL;
	}

	return id.ToInt();
}

int KYTY_SYSV_ABI AudioInInput(int handle, void* dest)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(dest == nullptr);

	EXIT_IF(g_audio == nullptr);

	if (!g_audio->AudioInValid(Audio::Id(handle)))
	{
		return AUDIO_IN_ERROR_INVALID_HANDLE;
	}

	return static_cast<int>(g_audio->AudioInInput(Audio::Id(handle), dest));
}

} // namespace AudioIn

namespace VoiceQoS {

LIB_NAME("VoiceQoS", "VoiceQoS");

int KYTY_SYSV_ABI VoiceQoSInit(void* mem_block, uint32_t mem_size, int32_t app_type)
{
	PRINT_NAME();

	printf("\t mem_block = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(mem_block));
	printf("\t mem_size = %" PRIu32 "\n", mem_size);
	printf("\t app_type = %" PRId32 "\n", app_type);

	return OK;
}

} // namespace VoiceQoS

namespace Ajm {

LIB_NAME("Ajm", "Ajm");

int KYTY_SYSV_ABI AjmInitialize(int64_t reserved, uint32_t* context)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(context == nullptr);
	EXIT_NOT_IMPLEMENTED(reserved != 0);

	*context = 1;

	return OK;
}

int KYTY_SYSV_ABI AjmModuleRegister(uint32_t context, uint32_t codec, int64_t reserved)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(context != 1);
	EXIT_NOT_IMPLEMENTED(reserved != 0);

	printf("\t codec = %u\n", codec);

	switch (codec)
	{
		case 1: printf("\t %s\n", "ATRAC9 decoder"); break;
		case 2: printf("\t %s\n", "MPEG4-AAC decoder"); break;
		case 0: printf("\t %s\n", "MP3 decoder"); break;
		case 4: printf("\t %s\n", "CELP8 encoder"); break;
		case 3: printf("\t %s\n", "CELP8 decoder"); break;
		case 13: printf("\t %s\n", "CELP16 encoder"); break;
		case 12: printf("\t %s\n", "CELP16 decoder"); break;
		default: EXIT("unknown codec\n");
	}

	return OK;
}

} // namespace Ajm

namespace AvPlayer {

LIB_NAME("AvPlayer", "AvPlayer");

using AvPlayerAllocate          = KYTY_SYSV_ABI void* (*)(void*, uint32_t, uint32_t);
using AvPlayerDeallocate        = KYTY_SYSV_ABI void (*)(void*, void*);
using AvPlayerAllocateTexture   = KYTY_SYSV_ABI void* (*)(void*, uint32_t, uint32_t);
using AvPlayerDeallocateTexture = KYTY_SYSV_ABI void (*)(void*, void*);
using AvPlayerOpenFile          = KYTY_SYSV_ABI int (*)(void*, const char*);
using AvPlayerCloseFile         = KYTY_SYSV_ABI int (*)(void*);
using AvPlayerReadOffsetFile    = KYTY_SYSV_ABI int (*)(void*, uint8_t*, uint64_t, uint32_t);
using AvPlayerSizeFile          = KYTY_SYSV_ABI uint64_t (*)(void*);
using AvPlayerEventCallback     = KYTY_SYSV_ABI void (*)(void*, int32_t, int32_t, void*);

struct AvPlayerMemAllocator
{
	void*                     object_pointer     = nullptr;
	AvPlayerAllocate          allocate           = nullptr;
	AvPlayerDeallocate        deallocate         = nullptr;
	AvPlayerAllocateTexture   allocate_texture   = nullptr;
	AvPlayerDeallocateTexture deallocate_texture = nullptr;
};

struct AvPlayerFileReplacement
{
	void*                  object_pointer = nullptr;
	AvPlayerOpenFile       open           = nullptr;
	AvPlayerCloseFile      close          = nullptr;
	AvPlayerReadOffsetFile read_offset    = nullptr;
	AvPlayerSizeFile       size           = nullptr;
};

struct AvPlayerEventReplacement
{
	void*                 object_pointer = nullptr;
	AvPlayerEventCallback event_callback = nullptr;
};

enum AvPlayerDebuglevels
{
	AvplayerDbgNone,
	AvplayerDbgInfo,
	AvplayerDbgWarnings,
	AvplayerDbgAll
};

struct AvPlayerInitData
{
	AvPlayerMemAllocator     memory_replacement;
	AvPlayerFileReplacement  file_replacement;
	AvPlayerEventReplacement event_replacement;
	AvPlayerDebuglevels      debug_level                   = AvPlayerDebuglevels::AvplayerDbgNone;
	uint32_t                 base_priority                 = 0;
	int32_t                  num_output_video_framebuffers = 0;
	Bool                     auto_start                    = 0;
	uint8_t                  reserved[3]                   = {};
	const char*              default_language              = nullptr;
};

struct AvPlayerAudioEx
{
	uint16_t channel_count;
	uint8_t  reserved[2];
	uint32_t sample_rate;
	uint32_t size;
	uint8_t  language_code[4];
	uint8_t  reserved1[64];
};

struct AvPlayerVideoEx
{
	uint32_t width;
	uint32_t height;
	float    aspect_ratio;
	uint8_t  language_code[4];
	uint32_t framerate;
	uint32_t crop_left_offset;
	uint32_t crop_right_offset;
	uint32_t crop_top_offset;
	uint32_t crop_bottom_offset;
	uint32_t pitch;
	uint8_t  luma_bit_depth;
	uint8_t  chroma_bit_depth;
	Bool     video_full_tange_flag;
	uint8_t  reserved1[37];
};

struct AvPlayerTimedTextEx
{
	uint8_t language_code[4];
	uint8_t reserved[12];
	uint8_t reserved1[64];
};

union AvPlayerStreamDetailsEx
{
	AvPlayerAudioEx     audio;
	AvPlayerVideoEx     video;
	AvPlayerTimedTextEx subs;
	uint8_t             reserved1[80];
};

struct AvPlayerFrameInfoEx
{
	void*                   data;
	uint8_t                 reserved[4];
	uint64_t                time_stamp;
	AvPlayerStreamDetailsEx details;
};

struct AvPlayerInternal
{
	String               filename;
	bool                 loop = false;
	AvPlayerMemAllocator mem;
	Core::Mutex          mutex;
	void*                fake_frame        = nullptr;
	uint32_t             fake_width        = 0;
	uint32_t             fake_height       = 0;
	float                fake_frame_rate   = 0.0f;
	uint32_t             fake_frame_num    = 0;
	uint32_t             fake_obtained_num = 0;
};

static void rgb_to_yuv(float r, float g, float b, uint8_t* y, uint8_t* u, uint8_t* v)
{
	int yf = static_cast<int>(16.0f + 65.481f * r + 128.553f * g + 24.966f * b);
	int uf = static_cast<int>(128.0f + -37.797f * r + -74.203f * g + 112.0f * b);
	int vf = static_cast<int>(128.0f + 112.0f * r + -93.786f * g + -18.214f * b);
	*y     = (yf < 0 ? 0 : (yf > 255 ? 255 : yf));
	*u     = (uf < 0 ? 0 : (uf > 255 ? 255 : uf));
	*v     = (vf < 0 ? 0 : (vf > 255 ? 255 : vf));
}

static void draw_fake_frame(uint32_t width, uint32_t height, void* data, float l)
{
	constexpr int STRIPS_NUM = 5;

	uint32_t luma_width        = width;
	uint32_t luma_height       = height;
	uint32_t chroma_width      = luma_width / 2;
	uint32_t chroma_height     = luma_height / 2;
	auto*    buffer            = static_cast<uint8_t*>(data);
	auto*    luma              = buffer;
	auto*    chroma            = buffer + luma_width * luma_height;
	uint32_t luma_strip_size   = luma_height / STRIPS_NUM;
	uint32_t chroma_strip_size = chroma_height / STRIPS_NUM;

	uint8_t color[STRIPS_NUM][3] = {};

	rgb_to_yuv(l, 0, 0, &color[0][0], &color[0][1], &color[0][2]);
	rgb_to_yuv(0, l, 0, &color[1][0], &color[1][1], &color[1][2]);
	rgb_to_yuv(0, 0, l, &color[2][0], &color[2][1], &color[2][2]);
	rgb_to_yuv(0, 0, 0, &color[3][0], &color[3][1], &color[3][2]);
	rgb_to_yuv(l, l, l, &color[4][0], &color[4][1], &color[4][2]);

	for (uint32_t y = 0; y < luma_strip_size; y++)
	{
		for (uint32_t x = 0; x < luma_width; x++)
		{
			for (int si = 0; si < STRIPS_NUM; si++)
			{
				luma[(y + luma_strip_size * si) * luma_width + x] = color[si][0];
			}
		}
	}
	for (uint32_t y = 0; y < chroma_strip_size; y++)
	{
		for (uint32_t x = 0; x < chroma_width; x++)
		{
			for (int si = 0; si < STRIPS_NUM; si++)
			{
				chroma[(y + chroma_strip_size * si) * chroma_width * 2 + x * 2 + 0] = color[si][1];
				chroma[(y + chroma_strip_size * si) * chroma_width * 2 + x * 2 + 1] = color[si][2];
			}
		}
	}
}

static void create_fake_video(AvPlayerInternal* r)
{
	uint32_t luma_width    = 1920;
	uint32_t luma_height   = 1080;
	uint32_t chroma_width  = luma_width / 2;
	uint32_t chroma_height = luma_height / 2;
	uint32_t size          = luma_width * luma_height + chroma_width * chroma_height * 2;
	auto*    buffer        = static_cast<uint8_t*>(r->mem.allocate_texture(r->mem.object_pointer, 256, size));

	r->fake_frame        = buffer;
	r->fake_width        = luma_width;
	r->fake_height       = luma_height;
	r->fake_frame_rate   = 59.94f;
	r->fake_frame_num    = 90;
	r->fake_obtained_num = 0;
}

static void delete_fake_video(AvPlayerInternal* r)
{
	r->mem.deallocate_texture(r->mem.object_pointer, r->fake_frame);
	r->fake_frame        = nullptr;
	r->fake_width        = 0;
	r->fake_height       = 0;
	r->fake_frame_rate   = 0.0f;
	r->fake_frame_num    = 0;
	r->fake_obtained_num = 0;
}

static bool get_fake_video(AvPlayerInternal* r, AvPlayerFrameInfoEx* info)
{
	if (r->fake_obtained_num < r->fake_frame_num)
	{
		info->data       = r->fake_frame;
		info->time_stamp = static_cast<uint64_t>(1000.0f * (static_cast<float>(r->fake_obtained_num) / r->fake_frame_rate));

		info->details.video.width                 = r->fake_width;
		info->details.video.height                = r->fake_height;
		info->details.video.aspect_ratio          = static_cast<float>(r->fake_width) / static_cast<float>(r->fake_height);
		info->details.video.language_code[0]      = 'e';
		info->details.video.language_code[1]      = 'n';
		info->details.video.language_code[2]      = 'g';
		info->details.video.language_code[3]      = '\0';
		info->details.video.framerate             = 0;
		info->details.video.crop_left_offset      = 0;
		info->details.video.crop_right_offset     = 0;
		info->details.video.crop_top_offset       = 0;
		info->details.video.crop_bottom_offset    = 0;
		info->details.video.pitch                 = r->fake_width;
		info->details.video.luma_bit_depth        = 8;
		info->details.video.chroma_bit_depth      = 8;
		info->details.video.video_full_tange_flag = 0;

		float pos   = static_cast<float>(r->fake_obtained_num) / static_cast<float>(r->fake_frame_num);
		float level = 1.0f;

		if (pos < 0.2f)
		{
			level = pos * pos * ((1.0f / 0.2f) * (1.0f / 0.2f));
		} else if (pos > 0.5f)
		{
			level = 1.0f - (1.0f - pos * (1.0f / 0.5f)) * (1.0f - pos * (1.0f / 0.5f));
		}

		draw_fake_frame(r->fake_width, r->fake_height, r->fake_frame, level * 0.7f);

		r->fake_obtained_num++;
		return true;
	}
	return false;
}

static bool fake_is_playing(AvPlayerInternal* r)
{
	return r->fake_obtained_num < r->fake_frame_num;
}

AvPlayerInternal* KYTY_SYSV_ABI AvPlayerInit(AvPlayerInitData* init)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(init == nullptr);

	printf("\t memory_replacement.object_pointer     = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->memory_replacement.object_pointer));
	printf("\t memory_replacement.allocate           = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->memory_replacement.allocate));
	printf("\t memory_replacement.deallocate         = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->memory_replacement.deallocate));
	printf("\t memory_replacement.allocate_texture   = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->memory_replacement.allocate_texture));
	printf("\t memory_replacement.deallocate_texture = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->memory_replacement.deallocate_texture));
	printf("\t file_replacement.object_pointer       = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->file_replacement.object_pointer));
	printf("\t file_replacement.open                 = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->file_replacement.open));
	printf("\t file_replacement.close                = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->file_replacement.close));
	printf("\t file_replacement.read_offset          = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->file_replacement.read_offset));
	printf("\t file_replacement.size                 = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(init->file_replacement.size));
	printf("\t event_replacement.object_pointer      = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->event_replacement.object_pointer));
	printf("\t event_replacement.event_callback      = %016" PRIx64 "\n",
	       reinterpret_cast<uint64_t>(init->event_replacement.event_callback));
	printf("\t debug_level                           = %s\n", Core::EnumName(init->debug_level).C_Str());
	printf("\t num_output_video_framebuffers         = %d\n", init->num_output_video_framebuffers);
	printf("\t base_priority                         = %u\n", init->base_priority);
	printf("\t auto_start                            = %u\n", init->auto_start);
	printf("\t default_language                      = %s\n", init->default_language == nullptr ? "(null)" : init->default_language);

	auto* r = new AvPlayerInternal;

	EXIT_NOT_IMPLEMENTED(init->auto_start != 0);
	EXIT_NOT_IMPLEMENTED(init->file_replacement.object_pointer != nullptr);
	EXIT_NOT_IMPLEMENTED(init->file_replacement.open != nullptr);
	EXIT_NOT_IMPLEMENTED(init->file_replacement.close != nullptr);
	EXIT_NOT_IMPLEMENTED(init->file_replacement.read_offset != nullptr);
	EXIT_NOT_IMPLEMENTED(init->file_replacement.size != nullptr);
	EXIT_NOT_IMPLEMENTED(init->event_replacement.object_pointer != nullptr);
	EXIT_NOT_IMPLEMENTED(init->event_replacement.event_callback != nullptr);

	r->mem = init->memory_replacement;

	create_fake_video(r);

	return r;
}

int KYTY_SYSV_ABI AvPlayerAddSource(AvPlayerInternal* h, const char* filename)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(h == nullptr);

	printf("\t filename = %s\n", filename);

	Core::LockGuard lock(h->mutex);

	h->filename = filename;

	return 0;
}

int KYTY_SYSV_ABI AvPlayerSetLooping(AvPlayerInternal* h, Bool loop)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(h == nullptr);

	printf("\t loop = %u\n", loop);

	Core::LockGuard lock(h->mutex);

	h->loop = (loop != 0);

	return 0;
}

Bool KYTY_SYSV_ABI AvPlayerGetVideoDataEx(AvPlayerInternal* h, AvPlayerFrameInfoEx* video_info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(h == nullptr);
	EXIT_NOT_IMPLEMENTED(video_info == nullptr);

	Core::LockGuard lock(h->mutex);

	EXIT_NOT_IMPLEMENTED(h->loop);

	if (get_fake_video(h, video_info))
	{
		return 1; // true
	}

	return 0; // false
}

Bool KYTY_SYSV_ABI AvPlayerGetAudioData(AvPlayerInternal* h, AvPlayerFrameInfo* audio_info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(h == nullptr);
	EXIT_NOT_IMPLEMENTED(audio_info == nullptr);

	return 0; // false
}

Bool KYTY_SYSV_ABI AvPlayerIsActive(AvPlayerInternal* h)
{
	PRINT_NAME();

	if (h != nullptr)
	{
		Core::LockGuard lock(h->mutex);

		EXIT_NOT_IMPLEMENTED(h->loop);

		if (fake_is_playing(h))
		{
			return 1; // true
		}
	}

	return 0; // false
}

int KYTY_SYSV_ABI AvPlayerClose(AvPlayerInternal* h)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(h == nullptr);

	delete_fake_video(h);

	delete h;

	return 0;
}

} // namespace AvPlayer

} // namespace Kyty::Libs::Audio

#endif // KYTY_EMU_ENABLED
