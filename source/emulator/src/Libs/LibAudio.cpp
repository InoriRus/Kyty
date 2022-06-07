#include "Emulator/Audio.h"
#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

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
	LIB_FUNC("QOQtbeDqsT4", AudioOut::AudioOutOutput);
	LIB_FUNC("s1--uE9mBFw", AudioOut::AudioOutClose);
	LIB_FUNC("GrQ9s4IrNaQ", AudioOut::AudioOutGetPortState);
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
	LIB_FUNC("Q3dyFuwGn64", Ajm::AjmModuleRegister);
}

} // namespace LibAjm

namespace LibAvPlayer {

LIB_VERSION("AvPlayer", 1, "AvPlayer", 1, 0);

namespace AvPlayer = Audio::AvPlayer;

LIB_DEFINE(InitAudio_1_AvPlayer)
{
	LIB_FUNC("aS66RI0gGgo", AvPlayer::AvPlayerInit);
	LIB_FUNC("KMcEa+rHsIo", AvPlayer::AvPlayerAddSource);
	LIB_FUNC("OVths0xGfho", AvPlayer::AvPlayerSetLooping);
	LIB_FUNC("JdksQu8pNdQ", AvPlayer::AvPlayerGetVideoDataEx);
	LIB_FUNC("Wnp1OVcrZgk", AvPlayer::AvPlayerGetAudioData);
	LIB_FUNC("UbQoYawOsfY", AvPlayer::AvPlayerIsActive);
	LIB_FUNC("NkJwDzKmIlw", AvPlayer::AvPlayerClose);
}

} // namespace LibAvPlayer

namespace LibAudio3d {

LIB_VERSION("Audio3d", 1, "Audio3d", 1, 1);

namespace Audio3d = Audio::Audio3d;

LIB_DEFINE(InitAudio_1_Audio3d)
{
	LIB_FUNC("UmCvjSmuZIw", Audio3d::Audio3dInitialize);
	LIB_FUNC("Im+jOoa5WAI", Audio3d::Audio3dGetDefaultOpenParameters);
	LIB_FUNC("XeDDK0xJWQA", Audio3d::Audio3dPortOpen);
	LIB_FUNC("Yq9bfUQ0uJg", Audio3d::Audio3dPortSetAttribute);
	LIB_FUNC("YaaDbDwKpFM", Audio3d::Audio3dPortGetQueueLevel);
	LIB_FUNC("lw0qrdSjZt8", Audio3d::Audio3dPortAdvance);
	LIB_FUNC("VEVhZ9qd4ZY", Audio3d::Audio3dPortPush);
}

} // namespace LibAudio3d

namespace LibNgs2 {

LIB_VERSION("Ngs2", 1, "Ngs2", 1, 1);

namespace Ngs2 = Audio::Ngs2;

LIB_DEFINE(InitAudio_1_Ngs2)
{
	LIB_FUNC("mPYgU4oYpuY", Ngs2::Ngs2SystemCreateWithAllocator);
	LIB_FUNC("U546k6orxQo", Ngs2::Ngs2RackCreateWithAllocator);
	LIB_FUNC("cLV4aiT9JpA", Ngs2::Ngs2RackCreate);
	LIB_FUNC("0eFLVCfWVds", Ngs2::Ngs2RackQueryBufferSize);
	LIB_FUNC("MwmHz8pAdAo", Ngs2::Ngs2RackGetVoiceHandle);
	LIB_FUNC("uu94irFOGpA", Ngs2::Ngs2VoiceControl);
	LIB_FUNC("-TOuuAQ-buE", Ngs2::Ngs2VoiceGetState);
	LIB_FUNC("i0VnXM-C9fc", Ngs2::Ngs2SystemRender);
}

} // namespace LibNgs2

LIB_DEFINE(InitAudio_1)
{
	LibAudioOut::InitAudio_1_AudioOut(s);
	LibAudioIn::InitAudio_1_AudioIn(s);
	LibVoiceQoS::InitAudio_1_VoiceQoS(s);
	LibAjm::InitAudio_1_Ajm(s);
	LibAvPlayer::InitAudio_1_AvPlayer(s);
	LibAudio3d::InitAudio_1_Audio3d(s);
	LibNgs2::InitAudio_1_Ngs2(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
