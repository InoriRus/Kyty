#include "Emulator/Audio.h"
#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibVoiceQoS {

LIB_VERSION("VoiceQoS", 1, "VoiceQoS", 0, 0);

namespace VoiceQoS = Audio::VoiceQoS;

LIB_DEFINE(InitAudio_1_VoiceQoS)
{
	LIB_FUNC("U8IfNl6-Css", VoiceQoS::VoiceQoSInit);
}

} // namespace LibVoiceQoS

LIB_DEFINE(InitAudio_1)
{
	LibVoiceQoS::InitAudio_1_VoiceQoS(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
