#include "Emulator/Audio.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Audio {

class Audio
{
public:
	Audio()          = default;
	virtual ~Audio() = default;

	KYTY_CLASS_NO_COPY(Audio);

private:
	Core::Mutex m_mutex;
};

static Audio* g_audio = nullptr;

KYTY_SUBSYSTEM_INIT(Audio)
{
	EXIT_IF(g_audio != nullptr);

	g_audio = new Audio;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Audio) {}

KYTY_SUBSYSTEM_DESTROY(Audio) {}

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
