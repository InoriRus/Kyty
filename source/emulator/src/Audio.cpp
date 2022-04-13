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
	bool     AudioOutValid(Id handle);
	bool     AudioOutSetVolume(Id handle, uint32_t bitflag, const int* volume);
	uint32_t AudioOutOutputs(OutputParam* params, uint32_t num);

	static constexpr int PORTS_MAX = 32;

private:
	struct Port
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

	Core::Mutex m_mutex;
	Port        m_ports[PORTS_MAX];
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

	for (int id = 0; id < PORTS_MAX; id++)
	{
		if (!m_ports[id].used)
		{
			auto& port = m_ports[id];

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

bool Audio::AudioOutValid(Id handle)
{
	Core::LockGuard lock(m_mutex);

	return (handle.GetId() >= 0 && handle.GetId() < PORTS_MAX && m_ports[handle.GetId()].used);
}

bool Audio::AudioOutSetVolume(Id handle, uint32_t bitflag, const int* volume)
{
	Core::LockGuard lock(m_mutex);

	if (AudioOutValid(handle))
	{
		auto& port = m_ports[handle.GetId()];

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

	const auto& first_port = m_ports[params[0].handle.GetId()];

	uint64_t block_time   = (1000000 * first_port.samples_num) / first_port.freq;
	uint64_t current_time = LibKernel::KernelGetProcessTime();

	uint64_t max_wait_time = 0;

	for (uint32_t i = 0; i < num; i++)
	{
		uint64_t next_time = m_ports[params[i].handle.GetId()].last_output_time + block_time;
		uint64_t wait_time = (next_time > current_time ? next_time - current_time : 0);
		max_wait_time      = (wait_time > max_wait_time ? wait_time : max_wait_time);
	}

	// Audio output is not yet implemented, so simulate audio delay
	Core::Thread::SleepMicro(max_wait_time);

	for (uint32_t i = 0; i < num; i++)
	{
		m_ports[params[i].handle.GetId()].last_output_time = LibKernel::KernelGetProcessTime();
	}

	return first_port.samples_num;
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

	EXIT_NOT_IMPLEMENTED(user_id != 255);
	EXIT_NOT_IMPLEMENTED(type != 0);
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

	EXIT_NOT_IMPLEMENTED(param == nullptr);
	EXIT_NOT_IMPLEMENTED(num != 1);

	Audio::OutputParam params[Audio::PORTS_MAX];

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

} // namespace AudioOut

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

} // namespace Kyty::Libs::Audio

#endif // KYTY_EMU_ENABLED
