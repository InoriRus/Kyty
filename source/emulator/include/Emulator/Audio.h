#ifndef EMULATOR_INCLUDE_EMULATOR_AUDIO_H_
#define EMULATOR_INCLUDE_EMULATOR_AUDIO_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Audio {

KYTY_SUBSYSTEM_DEFINE(Audio);

namespace AudioOut {

struct AudioOutOutputParam;

int KYTY_SYSV_ABI AudioOutInit();
int KYTY_SYSV_ABI AudioOutOpen(int user_id, int type, int index, uint32_t len, uint32_t freq, uint32_t param);
int KYTY_SYSV_ABI AudioOutSetVolume(int handle, uint32_t flag, int* vol);
int KYTY_SYSV_ABI AudioOutOutputs(AudioOutOutputParam* param, uint32_t num);

} // namespace AudioOut

namespace VoiceQoS {

int KYTY_SYSV_ABI VoiceQoSInit(void* mem_block, uint32_t mem_size, int32_t app_type);

} // namespace VoiceQoS

} // namespace Kyty::Libs::Audio

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_AUDIO_H_ */
