#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibcInternal {
LIB_DEFINE(InitLibcInternal_1);
} // namespace LibcInternal

LIB_DEFINE(InitAppContent_1);
LIB_DEFINE(InitAudio_1);
LIB_DEFINE(InitDebug_1);
LIB_DEFINE(InitDialog_1);
LIB_DEFINE(InitDiscMap_1);
LIB_DEFINE(InitGraphicsDriver_1);
LIB_DEFINE(InitLibKernel_1);
LIB_DEFINE(InitNet_1);
LIB_DEFINE(InitPad_1);
LIB_DEFINE(InitPlayGo_1);
LIB_DEFINE(InitSaveData_1);
LIB_DEFINE(InitSysmodule_1);
LIB_DEFINE(InitSystemService_1);
LIB_DEFINE(InitUserService_1);
LIB_DEFINE(InitVideoOut_1);

bool Init(const String& id, Loader::SymbolDatabase* s)
{
	LIB_CHECK(U"libAudio_1", InitAudio_1);
	LIB_CHECK(U"libAppContent_1", InitAppContent_1);
	LIB_CHECK(U"libc_internal_1", LibcInternal::InitLibcInternal_1);
	LIB_CHECK(U"libDebug_1", InitDebug_1);
	LIB_CHECK(U"libDialog_1", InitDialog_1);
	LIB_CHECK(U"libDiscMap_1", InitDiscMap_1);
	LIB_CHECK(U"libGraphicsDriver_1", InitGraphicsDriver_1);
	LIB_CHECK(U"libkernel_1", InitLibKernel_1);
	LIB_CHECK(U"libNet_1", InitNet_1);
	LIB_CHECK(U"libPad_1", InitPad_1);
	LIB_CHECK(U"libPlayGo_1", InitPlayGo_1);
	LIB_CHECK(U"libSaveData_1", InitSaveData_1);
	LIB_CHECK(U"libSysmodule_1", InitSysmodule_1);
	LIB_CHECK(U"libSystemService_1", InitSystemService_1);
	LIB_CHECK(U"libUserService_1", InitUserService_1);
	LIB_CHECK(U"libVideoOut_1", InitVideoOut_1);

	return false;
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
