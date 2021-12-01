#include "Emulator/Common.h"
#include "Emulator/Graphics/VideoOut.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

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
	LIB_FUNC("w3BY+tAEiQY", VideoOut::VideoOutRegisterBuffers);
	LIB_FUNC("U46NwOiJpys", VideoOut::VideoOutSubmitFlip);
	LIB_FUNC("SbU3dwp80lQ", VideoOut::VideoOutGetFlipStatus);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
