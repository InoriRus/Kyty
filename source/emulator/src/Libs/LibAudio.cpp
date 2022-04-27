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

namespace LibAudioIn {

LIB_VERSION("AudioIn", 1, "AudioIn", 1, 1);

namespace AudioIn = Audio::AudioIn;

LIB_DEFINE(InitAudio_1_AudioIn)
{
	LIB_FUNC("5NE8Sjc7VC8", AudioIn::AudioInOpen);
	LIB_FUNC("LozEOU8+anM", AudioIn::AudioInInput);
}

} // namespace LibAudioIn

namespace LibVoiceQoS {

LIB_VERSION("VoiceQoS", 1, "VoiceQoS", 0, 0);

namespace VoiceQoS = Audio::VoiceQoS;

LIB_DEFINE(InitAudio_1_VoiceQoS)
{
	LIB_FUNC("U8IfNl6-Css", VoiceQoS::VoiceQoSInit);
}

} // namespace LibVoiceQoS

namespace LibAjm {

LIB_VERSION("Ajm", 1, "Ajm", 1, 1);

namespace Ajm = Audio::Ajm;

LIB_DEFINE(InitAudio_1_Ajm)
{
	LIB_FUNC("dl+4eHSzUu4", Ajm::AjmInitialize);
}

} // namespace LibAjm

LIB_DEFINE(InitAudio_1)
{
	LibAudioOut::InitAudio_1_AudioOut(s);
	LibAudioIn::InitAudio_1_AudioIn(s);
	LibVoiceQoS::InitAudio_1_VoiceQoS(s);
	LibAjm::InitAudio_1_Ajm(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
