#include "Emulator/Audio.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Kernel/Semaphore.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#include <atomic>

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
		static Id Invalid() { return {}; }
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
	bool     AudioOutGetStatus(Id handle, int* type, int* channels_num);

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

bool Audio::AudioOutGetStatus(Id handle, int* type, int* channels_num)
{
	Core::LockGuard lock(m_mutex);

	if (AudioOutValid(handle))
	{
		auto& port = m_out_ports[handle.GetId()];

		*type         = port.type;
		*channels_num = port.channels_num;

		return true;
	}

	return false;
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

struct AudioOutPortState
{
	uint16_t output;
	uint8_t  channel;
	uint8_t  reserved1[1];
	int16_t  volume;
	uint16_t reroute_counter;
	uint64_t flag;
	uint64_t reserved2[2];
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
	EXIT_NOT_IMPLEMENTED(type != 0 && type != 1 && type != 3 && type != 4);
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

int KYTY_SYSV_ABI AudioOutGetPortState(int handle, AudioOutPortState* state)
{
	PRINT_NAME();

	int type         = 0;
	int channels_num = 0;

	if (!g_audio->AudioOutGetStatus(Audio::Id(handle), &type, &channels_num))
	{
		return AUDIO_OUT_ERROR_INVALID_PORT;
	}

	EXIT_NOT_IMPLEMENTED(state == nullptr);

	state->reroute_counter = 0;
	state->volume          = 127;

	switch (type)
	{
		case 0:
		case 1:
		case 2:
			state->output  = 1;
			state->channel = (channels_num > 2 ? 2 : channels_num);
			break;
		case 3:
		case 127:
			state->output  = 0;
			state->channel = 0;
			break;
		case 4:
			state->output  = 4;
			state->channel = 1;
			break;
		default: EXIT("unknown port type: %d\n", type);
	}

	printf("\t output  = %" PRIu16 "\n", state->output);
	printf("\t channel = %" PRIu8 "\n", state->channel);

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

	size_t luma_width        = width;
	size_t luma_height       = height;
	size_t chroma_width      = luma_width / 2;
	size_t chroma_height     = luma_height / 2;
	auto*  buffer            = static_cast<uint8_t*>(data);
	auto*  luma              = buffer;
	auto*  chroma            = buffer + luma_width * luma_height;
	size_t luma_strip_size   = luma_height / STRIPS_NUM;
	size_t chroma_strip_size = chroma_height / STRIPS_NUM;

	uint8_t color[STRIPS_NUM][3] = {};

	rgb_to_yuv(l, 0, 0, &color[0][0], &color[0][1], &color[0][2]);
	rgb_to_yuv(0, l, 0, &color[1][0], &color[1][1], &color[1][2]);
	rgb_to_yuv(0, 0, l, &color[2][0], &color[2][1], &color[2][2]);
	rgb_to_yuv(0, 0, 0, &color[3][0], &color[3][1], &color[3][2]);
	rgb_to_yuv(l, l, l, &color[4][0], &color[4][1], &color[4][2]);

	for (size_t y = 0; y < luma_strip_size; y++)
	{
		for (size_t x = 0; x < luma_width; x++)
		{
			for (int si = 0; si < STRIPS_NUM; si++)
			{
				luma[(y + luma_strip_size * si) * luma_width + x] = color[si][0];
			}
		}
	}
	for (size_t y = 0; y < chroma_strip_size; y++)
	{
		for (size_t x = 0; x < chroma_width; x++)
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

namespace Audio3d {

LIB_NAME("Audio3d", "Audio3d");

namespace Semaphore = LibKernel::Semaphore;

struct Audio3dOpenParameters
{
	size_t   size        = 0x20;
	uint32_t granularity = 256;
	uint32_t rate        = 0;
	uint32_t max_objects = 512;
	uint32_t queue_depth = 2;
	uint32_t buffer_mode = 2;
	uint32_t pad         = 0;
	// uint32_t num_beds;
};

struct Audio3dData
{
	enum class State
	{
		Empty,
		Ready,
		Play
	};

	std::atomic<State> state = State::Empty;
};

struct Audio3dInternal
{
	Audio3dData*          data                        = nullptr;
	Core::Mutex*          data_mutex                  = nullptr;
	uint64_t              data_delay                  = 0;
	Semaphore::KernelSema playback_sema               = nullptr;
	Audio3dOpenParameters params                      = {};
	int                   user_id                     = 0;
	float                 late_reverb_level           = 0.0f;
	float                 downmix_spread_radius       = 2.0f;
	int                   downmix_spread_height_aware = 0;
	uint32_t              data_index                  = 0;
	bool                  used                        = false;
	std::atomic_bool      playback_finished           = false;
};

constexpr uint32_t MAX_PORTS = 4;

static Audio3dInternal g_ports[MAX_PORTS] = {};

static void playback_simulate(void* arg)
{
	auto* port = static_cast<Audio3dInternal*>(arg);
	EXIT_IF(port == nullptr);
	EXIT_IF(port->data_mutex == nullptr);
	EXIT_IF(port->data == nullptr);

	for (;;)
	{
		int result = Semaphore::KernelWaitSema(port->playback_sema, 1, nullptr);

		if (result != OK)
		{
			break;
		}

		Audio3dData* play_data = nullptr;

		port->data_mutex->Lock();
		{
			for (uint32_t i = 0; i < port->params.queue_depth; i++)
			{
				uint32_t index = (port->data_index + i) % port->params.queue_depth;

				if (port->data[index].state == Audio3dData::State::Play)
				{
					play_data = &port->data[index];
					break;
				}
			}
		}
		port->data_mutex->Unlock();

		EXIT_IF(play_data == nullptr);

		if (play_data != nullptr)
		{
			// TODO(): Audio output is not yet implemented, so simulate audio delay
			Core::Thread::SleepMicro(port->data_delay);
			play_data->state = Audio3dData::State::Empty;
		}
	}

	port->playback_finished = true;
}

int KYTY_SYSV_ABI Audio3dInitialize(int64_t reserved)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(reserved != 0);

	return OK;
}

void KYTY_SYSV_ABI Audio3dGetDefaultOpenParameters(Audio3dOpenParameters* p)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(sizeof(Audio3dOpenParameters) != 0x20);

	*p = Audio3dOpenParameters();
}

int KYTY_SYSV_ABI Audio3dPortOpen(int user_id, const Audio3dOpenParameters* parameters, uint32_t* id)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(parameters == nullptr);
	EXIT_NOT_IMPLEMENTED(id == nullptr);
	EXIT_NOT_IMPLEMENTED(parameters->size != 0x20);

	printf("\t user_id     = %d\n", user_id);
	printf("\t granularity = %u\n", parameters->granularity);
	printf("\t rate        = %u\n", parameters->rate);
	printf("\t max_objects = %u\n", parameters->max_objects);
	printf("\t queue_depth = %u\n", parameters->queue_depth);
	printf("\t buffer_mode = %u\n", parameters->buffer_mode);

	EXIT_NOT_IMPLEMENTED(parameters->buffer_mode != 2);
	EXIT_NOT_IMPLEMENTED(user_id != 255 && user_id != 1);

	uint32_t port = 0;
	for (; port < MAX_PORTS; port++)
	{
		if (!g_ports[port].used)
		{
			break;
		}
	}

	EXIT_NOT_IMPLEMENTED(port >= MAX_PORTS);

	g_ports[port].user_id = user_id;
	g_ports[port].params  = *parameters;
	g_ports[port].used    = true;

	EXIT_IF(g_ports[port].data != nullptr);
	EXIT_IF(g_ports[port].data_mutex != nullptr);
	EXIT_IF(g_ports[port].playback_sema != nullptr);

	g_ports[port].data       = new Audio3dData[parameters->queue_depth];
	g_ports[port].data_index = 0;
	g_ports[port].data_mutex = new Core::Mutex;
	g_ports[port].data_delay = (1000000 * static_cast<uint64_t>(parameters->granularity)) / 48000;

	for (uint32_t d = 0; d < parameters->queue_depth; d++)
	{
		g_ports[port].data[d].state = Audio3dData::State::Empty;
	}

	int result = Semaphore::KernelCreateSema(&g_ports[port].playback_sema, "audio3d_play", 0x01, 0,
	                                         static_cast<int>(parameters->queue_depth), nullptr);
	EXIT_NOT_IMPLEMENTED(result != OK);

	g_ports[port].playback_finished = false;
	Core::Thread playback_thread(playback_simulate, &g_ports[port]);
	playback_thread.Detach();

	*id = port;

	return OK;
}

int KYTY_SYSV_ABI Audio3dPortSetAttribute(uint32_t port_id, uint32_t attribute_id, const void* attribute, size_t attribute_size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(port_id >= MAX_PORTS);
	EXIT_NOT_IMPLEMENTED(!g_ports[port_id].used);
	EXIT_NOT_IMPLEMENTED(attribute == nullptr);

	printf("\t attribute_id = 0x%" PRIx32 "\n", attribute_id);

	switch (attribute_id)
	{
		case 0x10001:
			EXIT_NOT_IMPLEMENTED(attribute_size != 4);
			g_ports[port_id].late_reverb_level = *static_cast<const float*>(attribute);
			printf("\t late_reverb_level = %f\n", g_ports[port_id].late_reverb_level);
			break;
		case 0x10002:
			EXIT_NOT_IMPLEMENTED(attribute_size != 4);
			g_ports[port_id].downmix_spread_radius = *static_cast<const float*>(attribute);
			printf("\t downmix_spread_radius = %f\n", g_ports[port_id].downmix_spread_radius);
			break;
		case 0x10003:
			EXIT_NOT_IMPLEMENTED(attribute_size != 4);
			g_ports[port_id].downmix_spread_height_aware = *static_cast<const int*>(attribute);
			printf("\t downmix_spread_height_aware = %d\n", g_ports[port_id].downmix_spread_height_aware);
			break;
		default: EXIT("unknown attribute: 0x%" PRIx32 "\n", attribute_id);
	}

	return OK;
}

int KYTY_SYSV_ABI Audio3dPortGetQueueLevel(uint32_t port_id, uint32_t* queue_level, uint32_t* queue_available)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(port_id >= MAX_PORTS);
	EXIT_NOT_IMPLEMENTED(!g_ports[port_id].used);
	EXIT_NOT_IMPLEMENTED(queue_level == nullptr && queue_available == nullptr);

	auto* port = &g_ports[port_id];

	uint32_t empty_num = 0;

	port->data_mutex->Lock();
	{
		for (uint32_t i = 0; i < port->params.queue_depth; i++)
		{
			uint32_t index = (port->data_index + i) % port->params.queue_depth;

			if (port->data[index].state == Audio3dData::State::Empty)
			{
				empty_num++;
			} else
			{
				break;
			}
		}
	}
	port->data_mutex->Unlock();

	EXIT_IF(empty_num > port->params.queue_depth);

	printf("\t queue_available = %u\n", empty_num);

	if (queue_level != nullptr)
	{
		*queue_level = port->params.queue_depth - empty_num;
	}
	if (queue_available != nullptr)
	{
		*queue_available = empty_num;
	}

	return OK;
}

int KYTY_SYSV_ABI Audio3dPortAdvance(uint32_t port_id)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(port_id >= MAX_PORTS);
	EXIT_NOT_IMPLEMENTED(!g_ports[port_id].used);

	auto* port = &g_ports[port_id];

	port->data_mutex->Lock();
	{
		uint32_t current_index = port->data_index;
		uint32_t next_index    = (current_index + 1) % port->params.queue_depth;

		if (port->data[current_index].state == Audio3dData::State::Empty)
		{
			port->data[current_index].state = Audio3dData::State::Ready;
		}

		EXIT_NOT_IMPLEMENTED(port->data[current_index].state != Audio3dData::State::Ready);

		port->data_index = next_index;

		printf("\t %u -> %u\n", current_index, next_index);
	}
	port->data_mutex->Unlock();

	return OK;
}

int KYTY_SYSV_ABI Audio3dPortPush(uint32_t port_id, uint32_t blocking)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(port_id >= MAX_PORTS);
	EXIT_NOT_IMPLEMENTED(!g_ports[port_id].used);

	auto* port = &g_ports[port_id];

	EXIT_NOT_IMPLEMENTED(blocking != 1);

	printf("\t blocking = %u\n", blocking);

	int          data_num   = 0;
	Audio3dData* first_data = nullptr;

	port->data_mutex->Lock();
	{
		first_data = port->data + port->data_index;

		for (uint32_t i = 0; i < port->params.queue_depth; i++)
		{
			uint32_t index = (port->data_index + i) % port->params.queue_depth;

			if (port->data[index].state == Audio3dData::State::Ready)
			{
				port->data[index].state = Audio3dData::State::Play;
				data_num++;
			}
		}
	}
	port->data_mutex->Unlock();

	printf("\t push num = %d\n", data_num);

	if (data_num > 0)
	{
		Semaphore::KernelSignalSema(port->playback_sema, data_num);

		if (blocking == 1)
		{
			auto wait_time = port->data_delay / 8;
			while (first_data->state != Audio3dData::State::Empty)
			{
				Core::Thread::SleepMicro(wait_time);
			}
		}
	}

	return OK;
}

} // namespace Audio3d

namespace Ngs2 {

LIB_NAME("Ngs2", "Ngs2");

struct Ngs2SystemOption
{
	size_t   size              = 0;
	char     name[16]          = {};
	uint32_t flags             = 0;
	uint32_t max_grain_samples = 0;
	uint32_t num_grain_samples = 0;
	uint32_t sample_rate       = 0;
	uint32_t reserved[6]       = {};
};

struct Ngs2RackOption
{
	size_t   size                   = 0;
	char     name[16]               = {};
	uint32_t flags                  = 0;
	uint32_t max_grain_samples      = 0;
	uint32_t max_voices             = 0;
	uint32_t max_input_delay_blocks = 0;
	uint32_t max_matrices           = 0;
	uint32_t max_ports              = 0;
	uint32_t reserved[20]           = {};
};

struct Ngs2MasteringRackOption
{
	Ngs2RackOption rack_option;
	uint32_t       max_channels          = 0;
	uint32_t       num_peak_meter_blocks = 0;
};

struct Ngs2SubmixerRackOption
{
	Ngs2RackOption rack_option;
	uint32_t       max_channels          = 0;
	uint32_t       max_envelope_points   = 0;
	uint32_t       max_filters           = 0;
	uint32_t       max_inputs            = 0;
	uint32_t       num_peak_meter_blocks = 0;
};

struct Ngs2SamplerRackOption
{
	Ngs2RackOption rack_option;
	uint32_t       max_channel_works        = 0;
	uint32_t       max_codec_caches         = 0;
	uint32_t       max_waveform_blocks      = 0;
	uint32_t       max_envelope_points      = 0;
	uint32_t       max_filters              = 0;
	uint32_t       max_atrac9_decoders      = 0;
	uint32_t       max_atrac9_channel_works = 0;
	uint32_t       max_ajm_atrac9_decoders  = 0;
	uint32_t       num_peak_meter_blocks    = 0;
};

struct Ngs2ReverbRackOption
{
	Ngs2RackOption rack_option;
	uint32_t       max_channels = 0;
	uint32_t       reverb_size  = 0;
};

struct Ngs2CustomModuleOption
{
	uint32_t size = 0;
};

struct Ngs2CustomRackModuleInfo
{
	const Ngs2CustomModuleOption* option           = nullptr;
	uint32_t                      module_id        = 0;
	uint32_t                      source_buffer_id = 0;
	uint32_t                      extra_buffer_id  = 0;
	uint32_t                      dest_buffer_id   = 0;
	uint32_t                      state_offset     = 0;
	uint32_t                      state_size       = 0;
	uint32_t                      reserved         = 0;
	uint32_t                      reserved2        = 0;
};

struct Ngs2CustomRackPortInfo
{
	uint32_t source_buffer_id = 0;
	uint32_t reserved         = 0;
};

struct Ngs2CustomRackOption
{
	Ngs2RackOption           rack_option;
	uint32_t                 state_size  = 0;
	uint32_t                 num_buffers = 0;
	uint32_t                 num_modules = 0;
	uint32_t                 reserved    = 0;
	Ngs2CustomRackModuleInfo module[24];
	Ngs2CustomRackPortInfo   port[16];
};

struct Ngs2CustomSubmixerRackOption
{
	Ngs2CustomRackOption custom_rack_option;
	uint32_t             max_channels = 0;
	uint32_t             max_inputs   = 0;
};

union Ngs2RackOptionUnion
{
	Ngs2RackOption               common;
	Ngs2SamplerRackOption        sampler;
	Ngs2MasteringRackOption      mastering;
	Ngs2SubmixerRackOption       submixer;
	Ngs2ReverbRackOption         reverb;
	Ngs2CustomSubmixerRackOption custom_submixer;
};

struct Ngs2ContextBufferInfo
{
	void*     host_buffer      = nullptr;
	size_t    host_buffer_size = 0;
	uintptr_t reserved[5]      = {};
	uintptr_t user_data        = 0;
};

using Ngs2BufferAllocHandler = int32_t KYTY_SYSV_ABI (*)(Ngs2ContextBufferInfo*);
using Ngs2BufferFreeHandler  = int32_t  KYTY_SYSV_ABI (*)(Ngs2ContextBufferInfo*);

struct Ngs2BufferAllocator
{
	Ngs2BufferAllocHandler alloc_handler = nullptr;
	Ngs2BufferFreeHandler  free_handler  = nullptr;
	uintptr_t              user_data     = 0;
};

struct Ngs2Internal
{
	Ngs2SystemOption    option;
	Ngs2BufferAllocator allocator;
	Ngs2Internal*       next = nullptr;
	Core::Mutex         mutex;
};

enum class Ngs2RackType
{
	Sampler,
	Submixer,
	Mastering,
	Reverb,
	CustomSubmixer,
};

struct Ngs2RackInternal
{
	Ngs2Internal*       ngs  = nullptr;
	Ngs2RackInternal*   next = nullptr;
	Ngs2RackType        type = Ngs2RackType::Sampler;
	Ngs2RackOptionUnion option;
	Ngs2BufferAllocator allocator;
};

enum class Ngs2VoicePlayState
{
	Empty,
	Playing,
	Paused,
	Stopped
};

enum class Ngs2VoicePlayEvent
{
	None,
	Play,
	Pause,
	Resume,
	Stop,
	StopImm,
	Kill
};

struct Ngs2VoiceInternal
{
	Ngs2VoicePlayEvent event = Ngs2VoicePlayEvent::None;
	Ngs2VoicePlayState state = Ngs2VoicePlayState::Empty;
	Ngs2RackInternal*  rack  = nullptr;
};

struct Ngs2VoiceParamHeader
{
	uint16_t size;
	int16_t  next;
	uint32_t id;
};

struct Ngs2VoiceEventParam
{
	Ngs2VoiceParamHeader header;
	uint32_t             event_id;
};

struct Ngs2VoicePatchParam
{
	Ngs2VoiceParamHeader header;
	uint32_t             port;
	uint32_t             dest_input_id;
	uintptr_t            dest_handle;
};

struct Ngs2VoicePortMatrixParam
{
	Ngs2VoiceParamHeader header;
	uint32_t             port;
	int32_t              matrix_id;
};

struct Ngs2VoiceState
{
	uint32_t state_flags;
};

struct Ngs2SamplerVoiceState
{
	Ngs2VoiceState voice_state;
	float          envelope_height;
	float          peak_height;
	uint32_t       reserved;
	uint64_t       num_decoded_samples;
	uint64_t       decoded_data_size;
	uint64_t       user_data;
	const void*    waveform_data;
};

static Ngs2Internal*     g_ngs_list   = nullptr;
static Ngs2RackInternal* g_racks_list = nullptr;

int KYTY_SYSV_ABI Ngs2RackQueryBufferSize(uint32_t rack_id, const Ngs2RackOption* option, Ngs2ContextBufferInfo* buffer_info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(option == nullptr);
	EXIT_NOT_IMPLEMENTED(buffer_info == nullptr);

	printf("\t rack_id    = 0x%" PRIx32 "\n", rack_id);
	printf("\t max_voices = %u\n", option->max_voices);

	buffer_info->host_buffer_size = sizeof(Ngs2RackInternal) + sizeof(Ngs2VoiceInternal) * option->max_voices;

	return OK;
}

int KYTY_SYSV_ABI Ngs2SystemCreateWithAllocator(const Ngs2SystemOption* option, const Ngs2BufferAllocator* allocator, uintptr_t* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(option == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator == nullptr);
	EXIT_NOT_IMPLEMENTED(handle == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator->alloc_handler == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator->free_handler == nullptr);

	EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2SystemOption));

	printf("\t name              = %.16s\n", option->name);
	printf("\t flags             = %u\n", option->flags);
	printf("\t max_grain_samples = %u\n", option->max_grain_samples);
	printf("\t num_grain_samples = %u\n", option->num_grain_samples);
	printf("\t sample_rate       = %u\n", option->sample_rate);
	printf("\t alloc_handler     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->alloc_handler));
	printf("\t free_handler      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->free_handler));
	printf("\t user_data         = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->user_data));

	Ngs2ContextBufferInfo buf {};
	buf.host_buffer      = nullptr;
	buf.host_buffer_size = sizeof(Ngs2Internal);
	buf.user_data        = allocator->user_data;

	int result = allocator->alloc_handler(&buf);

	EXIT_NOT_IMPLEMENTED(result != OK);
	EXIT_NOT_IMPLEMENTED(buf.host_buffer == nullptr);

	auto* ngs = new (buf.host_buffer) Ngs2Internal;

	ngs->option    = *option;
	ngs->allocator = *allocator;

	ngs->next  = g_ngs_list;
	g_ngs_list = ngs;

	*handle = reinterpret_cast<uintptr_t>(ngs);

	return OK;
}

int KYTY_SYSV_ABI Ngs2RackCreate(uintptr_t system_handle, uint32_t rack_id, const Ngs2RackOption* option,
                                 const Ngs2ContextBufferInfo* buffer_info, uintptr_t* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(option == nullptr);
	EXIT_NOT_IMPLEMENTED(buffer_info == nullptr);
	EXIT_NOT_IMPLEMENTED(handle == nullptr);
	EXIT_NOT_IMPLEMENTED(buffer_info->host_buffer == nullptr);
	EXIT_NOT_IMPLEMENTED(buffer_info->host_buffer_size == 0);
	EXIT_NOT_IMPLEMENTED(system_handle == 0);

	EXIT_NOT_IMPLEMENTED(option->size < sizeof(Ngs2RackOption));

	printf("\t rack_id                = 0x%" PRIx32 "\n", rack_id);
	printf("\t name                   = %.16s\n", option->name);
	printf("\t flags                  = %u\n", option->flags);
	printf("\t max_grain_samples      = %u\n", option->max_grain_samples);
	printf("\t max_voices             = %u\n", option->max_voices);
	printf("\t max_input_delay_blocks = %u\n", option->max_input_delay_blocks);
	printf("\t max_matrices           = %u\n", option->max_matrices);
	printf("\t max_ports              = %u\n", option->max_ports);
	printf("\t host_buffer            = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(buffer_info->host_buffer));
	printf("\t host_buffer_size      = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(buffer_info->host_buffer_size));

	auto* ngs    = reinterpret_cast<Ngs2Internal*>(system_handle);
	auto* rack   = static_cast<Ngs2RackInternal*>(buffer_info->host_buffer);
	auto* voices = reinterpret_cast<Ngs2VoiceInternal*>(rack + 1);

	Core::LockGuard lock(ngs->mutex);

	switch (rack_id)
	{
		case 0x1000:
			EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2SamplerRackOption));
			rack->option.sampler = *reinterpret_cast<const Ngs2SamplerRackOption*>(option);
			rack->type           = Ngs2RackType::Sampler;
			break;
		case 0x2000:
			EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2SubmixerRackOption));
			rack->option.submixer = *reinterpret_cast<const Ngs2SubmixerRackOption*>(option);
			rack->type            = Ngs2RackType::Submixer;
			break;
		case 0x2001:
			EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2ReverbRackOption));
			rack->option.reverb = *reinterpret_cast<const Ngs2ReverbRackOption*>(option);
			rack->type          = Ngs2RackType::Reverb;
			break;
		case 0x3000:
			EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2MasteringRackOption));
			rack->option.mastering = *reinterpret_cast<const Ngs2MasteringRackOption*>(option);
			rack->type             = Ngs2RackType::Mastering;
			break;
		case 0x4002:
			EXIT_NOT_IMPLEMENTED(option->size != sizeof(Ngs2CustomSubmixerRackOption));
			rack->option.custom_submixer = *reinterpret_cast<const Ngs2CustomSubmixerRackOption*>(option);
			rack->type                   = Ngs2RackType::CustomSubmixer;
			break;
		default: EXIT("unknown rack_id: 0x%" PRIx32 "\n", rack_id);
	}

	printf("\t type                   = %s\n", Core::EnumName(rack->type).C_Str());

	rack->allocator = Ngs2BufferAllocator();
	rack->ngs       = ngs;

	rack->next   = g_racks_list;
	g_racks_list = rack;

	for (uint32_t i = 0; i < option->max_voices; i++)
	{
		voices[i].rack  = rack;
		voices[i].event = Ngs2VoicePlayEvent::None;
		voices[i].state = Ngs2VoicePlayState::Empty;
	}

	*handle = reinterpret_cast<uintptr_t>(rack);

	return OK;
}

int KYTY_SYSV_ABI Ngs2RackCreateWithAllocator(uintptr_t system_handle, uint32_t rack_id, const Ngs2RackOption* option,
                                              const Ngs2BufferAllocator* allocator, uintptr_t* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(option == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator == nullptr);
	EXIT_NOT_IMPLEMENTED(handle == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator->alloc_handler == nullptr);
	EXIT_NOT_IMPLEMENTED(allocator->free_handler == nullptr);
	EXIT_NOT_IMPLEMENTED(system_handle == 0);

	EXIT_NOT_IMPLEMENTED(option->size < sizeof(Ngs2RackOption));

	printf("\t rack_id                = 0x%" PRIx32 "\n", rack_id);
	printf("\t name                   = %.16s\n", option->name);
	printf("\t flags                  = %u\n", option->flags);
	printf("\t max_grain_samples      = %u\n", option->max_grain_samples);
	printf("\t max_voices             = %u\n", option->max_voices);
	printf("\t max_input_delay_blocks = %u\n", option->max_input_delay_blocks);
	printf("\t max_matrices           = %u\n", option->max_matrices);
	printf("\t max_ports              = %u\n", option->max_ports);
	printf("\t alloc_handler          = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->alloc_handler));
	printf("\t free_handler           = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->free_handler));
	printf("\t user_data              = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(allocator->user_data));

	Ngs2ContextBufferInfo buf {};
	buf.host_buffer      = nullptr;
	buf.host_buffer_size = 0;
	buf.user_data        = allocator->user_data;

	Ngs2RackQueryBufferSize(rack_id, option, &buf);

	EXIT_NOT_IMPLEMENTED(buf.host_buffer_size == 0);

	int result = allocator->alloc_handler(&buf);

	EXIT_NOT_IMPLEMENTED(result != OK);
	EXIT_NOT_IMPLEMENTED(buf.host_buffer == nullptr);

	result = Ngs2RackCreate(system_handle, rack_id, option, &buf, handle);

	if (result == OK)
	{
		auto* rack      = static_cast<Ngs2RackInternal*>(buf.host_buffer);
		rack->allocator = *allocator;
	}

	return result;
}

int KYTY_SYSV_ABI Ngs2SystemRender(uintptr_t system_handle, const Ngs2RenderBufferInfo* buffer_info, uint32_t num_buffer_info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(buffer_info == nullptr);
	EXIT_NOT_IMPLEMENTED(system_handle == 0);
	EXIT_NOT_IMPLEMENTED(num_buffer_info == 0);

	auto* ngs = reinterpret_cast<Ngs2Internal*>(system_handle);

	Core::LockGuard lock(ngs->mutex);

	for (auto* rack = g_racks_list; rack != nullptr; rack = rack->next)
	{
		if (rack->ngs == ngs)
		{
			auto* voices = reinterpret_cast<Ngs2VoiceInternal*>(rack + 1);

			for (uint32_t i = 0; i < rack->option.common.max_voices; i++)
			{
				auto& voice = voices[i];
				switch (voice.event)
				{
					case Ngs2VoicePlayEvent::None:
						if (voice.state == Ngs2VoicePlayState::Playing || voice.state == Ngs2VoicePlayState::Stopped)
						{
							voice.state = Ngs2VoicePlayState::Empty;
						}
						break;
					case Ngs2VoicePlayEvent::Play:
						if (voice.state == Ngs2VoicePlayState::Empty)
						{
							voice.state = Ngs2VoicePlayState::Playing;
						}
						break;
					case Ngs2VoicePlayEvent::Pause:
						if (voice.state == Ngs2VoicePlayState::Playing)
						{
							voice.state = Ngs2VoicePlayState::Paused;
						}
						break;
					case Ngs2VoicePlayEvent::Resume:
						if (voice.state == Ngs2VoicePlayState::Paused)
						{
							voice.state = Ngs2VoicePlayState::Playing;
						}
						break;
					case Ngs2VoicePlayEvent::Stop:
						if (voice.state == Ngs2VoicePlayState::Playing)
						{
							voice.state = Ngs2VoicePlayState::Stopped;
						}
						break;
					case Ngs2VoicePlayEvent::StopImm:
					case Ngs2VoicePlayEvent::Kill: voice.state = Ngs2VoicePlayState::Empty; break;
				}
				voice.event = Ngs2VoicePlayEvent::None;
			}
		}
	}

	return OK;
}

int KYTY_SYSV_ABI Ngs2RackGetVoiceHandle(uintptr_t rack_handle, uint32_t voice_id, uintptr_t* handle)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(handle == nullptr);
	EXIT_NOT_IMPLEMENTED(rack_handle == 0);

	printf("\t voice_id = %u\n", voice_id);

	auto* rack   = reinterpret_cast<Ngs2RackInternal*>(rack_handle);
	auto* voices = reinterpret_cast<Ngs2VoiceInternal*>(rack_handle + sizeof(Ngs2RackInternal));

	EXIT_NOT_IMPLEMENTED(voice_id >= rack->option.common.max_voices);

	EXIT_IF(voices[voice_id].rack != rack);

	*handle = reinterpret_cast<uintptr_t>(voices + voice_id);

	return OK;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int KYTY_SYSV_ABI Ngs2VoiceControl(uintptr_t voice_handle, const Ngs2VoiceParamHeader* param_list)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(param_list == nullptr);
	EXIT_NOT_IMPLEMENTED(voice_handle == 0);

	auto* voice = reinterpret_cast<Ngs2VoiceInternal*>(voice_handle);

	Core::LockGuard lock(voice->rack->ngs->mutex);

	const auto* param = param_list;

	for (;;)
	{
		printf("\t id   = 0x%08" PRIx32 "\n", param->id);
		printf("\t size = %" PRIu16 "\n", param->size);
		printf("\t next = %" PRId16 "\n", param->next);

		auto rack_id = param->id >> 16u;

		EXIT_NOT_IMPLEMENTED(((param->id >> 15u) & 0x1u) != 0);

		switch (rack_id)
		{
			case 0x0000:
			{
				auto cid = param->id & 0x7fffu;
				switch (cid)
				{
					case 0x0002:
					{
						EXIT_NOT_IMPLEMENTED(param->size != sizeof(Ngs2VoicePortMatrixParam));
						const auto* pm = reinterpret_cast<const Ngs2VoicePortMatrixParam*>(param);
						printf("\t port      = %u\n", pm->port);
						printf("\t matrix_id = %d\n", pm->matrix_id);
						break;
					}
					case 0x0005:
					{
						EXIT_NOT_IMPLEMENTED(param->size != sizeof(Ngs2VoicePatchParam));
						const auto* patch = reinterpret_cast<const Ngs2VoicePatchParam*>(param);
						printf("\t connect->port          = %u\n", patch->port);
						printf("\t connect->dest_input_id = %u\n", patch->dest_input_id);
						printf("\t connect->dest_handle   = 0x%016" PRIx64 "\n", patch->dest_handle);
						break;
					}
					case 0x0006:
					{
						EXIT_NOT_IMPLEMENTED(param->size != sizeof(Ngs2VoiceEventParam));
						const auto* event = reinterpret_cast<const Ngs2VoiceEventParam*>(param);
						switch (event->event_id)
						{
							case 0: voice->event = Ngs2VoicePlayEvent::Play; break;
							case 1: voice->event = Ngs2VoicePlayEvent::Stop; break;
							case 2: voice->event = Ngs2VoicePlayEvent::StopImm; break;
							case 3: voice->event = Ngs2VoicePlayEvent::Kill; break;
							case 4: voice->event = Ngs2VoicePlayEvent::Pause; break;
							case 5: voice->event = Ngs2VoicePlayEvent::Resume; break;
							default: EXIT("unknown event_id: 0x%08" PRIx32 "\n", event->event_id);
						}
						printf("\t event = %u\n", event->event_id);
						break;
					}
					default: EXIT("unknown id: 0x%04" PRIx32 "\n", cid);
				}
				break;
			}
			case 0x1000: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::Sampler); break;
			case 0x2000: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::Submixer); break;
			case 0x2001: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::Reverb); break;
			case 0x3000: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::Mastering); break;
			case 0x4000: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::CustomSubmixer); break;
			case 0x4002: EXIT_NOT_IMPLEMENTED(voice->rack->type != Ngs2RackType::CustomSubmixer); break;
			default: EXIT("unknown rack_id: 0x%" PRIx32 "\n", rack_id);
		}

		if (param->next == 0)
		{
			break;
		}
		param = reinterpret_cast<const Ngs2VoiceParamHeader*>(reinterpret_cast<uintptr_t>(param) + param->next);
	}

	return OK;
}

int KYTY_SYSV_ABI Ngs2VoiceGetState(uintptr_t voice_handle, Ngs2VoiceState* state, size_t state_size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(state == nullptr);
	EXIT_NOT_IMPLEMENTED(voice_handle == 0);

	auto* voice = reinterpret_cast<Ngs2VoiceInternal*>(voice_handle);

	Core::LockGuard lock(voice->rack->ngs->mutex);

	switch (voice->rack->type)
	{
		case Ngs2RackType::Sampler:
		{
			EXIT_NOT_IMPLEMENTED(state_size != sizeof(Ngs2SamplerVoiceState));
			auto* sampler = reinterpret_cast<Ngs2SamplerVoiceState*>(state);
			switch (voice->state)
			{
				case Ngs2VoicePlayState::Empty: sampler->voice_state.state_flags = 0; break;
				case Ngs2VoicePlayState::Playing: sampler->voice_state.state_flags = 0x3; break;
				case Ngs2VoicePlayState::Paused: sampler->voice_state.state_flags = 0x5; break;
				case Ngs2VoicePlayState::Stopped: sampler->voice_state.state_flags = 0xb; break;
			}
			sampler->envelope_height     = 1.0f;
			sampler->peak_height         = 0.0f;
			sampler->reserved            = 0;
			sampler->num_decoded_samples = 0;
			sampler->user_data           = 0;
			sampler->waveform_data       = nullptr;
			printf("\t state_flags = %u\n", sampler->voice_state.state_flags);
			break;
		}
		default: EXIT("unknown type: %s\n", Core::EnumName(voice->rack->type).C_Str());
	}

	return OK;
}

} // namespace Ngs2

} // namespace Kyty::Libs::Audio

#endif // KYTY_EMU_ENABLED
