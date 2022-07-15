#include "Emulator/Common.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibGen4 {

LIB_VERSION("VideoOut", 1, "VideoOut", 0, 0);

LIB_DEFINE(InitVideoOut_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("Up36PTk687E", VideoOut::VideoOutOpen);
	LIB_FUNC("uquVH4-Du78", VideoOut::VideoOutClose);
	LIB_FUNC("6kPnj51T62Y", VideoOut::VideoOutGetResolutionStatus);
	LIB_FUNC("i6-sR91Wt-4", VideoOut::VideoOutSetBufferAttribute);
	LIB_FUNC("CBiu4mCE1DA", VideoOut::VideoOutSetFlipRate);
	LIB_FUNC("HXzjK9yI30k", VideoOut::VideoOutAddFlipEvent);
	LIB_FUNC("Xru92wHJRmg", VideoOut::VideoOutAddVblankEvent);
	LIB_FUNC("w3BY+tAEiQY", VideoOut::VideoOutRegisterBuffers);
	LIB_FUNC("U46NwOiJpys", VideoOut::VideoOutSubmitFlip);
	LIB_FUNC("SbU3dwp80lQ", VideoOut::VideoOutGetFlipStatus);
	LIB_FUNC("1FZBKy8HeNU", VideoOut::VideoOutGetVblankStatus);
	LIB_FUNC("MTxxrOCeSig", VideoOut::VideoOutSetWindowModeMargins);
}

} // namespace LibGen4

namespace LibGen5 {

LIB_VERSION("VideoOut", 1, "VideoOut", 1, 1);

LIB_DEFINE(InitVideoOut_1)
{
	PRINT_NAME_ENABLE(true);

	LIB_FUNC("Up36PTk687E", VideoOut::VideoOutOpen);
	LIB_FUNC("PjS5uASwcV8", VideoOut::VideoOutSetBufferAttribute2);
	LIB_FUNC("rKBUtgRrtbk", VideoOut::VideoOutRegisterBuffers2);
}

} // namespace LibGen5

LIB_DEFINE(InitVideoOut_1)
{
	LibGen4::InitVideoOut_1(s);
	LibGen5::InitVideoOut_1(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
