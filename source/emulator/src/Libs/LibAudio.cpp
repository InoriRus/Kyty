#include "Emulator/Audio.h"
#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibAudioOut {

LIB_VERSION("AudioOut", 1, "AudioOut", 1, 1);

namespace AudioOut = Audio::AudioOut;

LIB_DEFINE(InitAudio_1_AudioOut)
{
	LIB_FUNC("JfEPXVxhFqA", AudioOut::AudioOutInit);
	LIB_FUNC("ekNvsT22rsY", AudioOut::AudioOutOpen);
	LIB_FUNC("b+uAV89IlxE", AudioOut::AudioOutSetVolume);
	LIB_FUNC("w3PdaSTSwGE", AudioOut::AudioOutOutputs);
}

} // namespace LibAudioOut

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
	LibAudioOut::InitAudio_1_AudioOut(s);
	LibVoiceQoS::InitAudio_1_VoiceQoS(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
