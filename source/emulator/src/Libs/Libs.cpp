#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibcInternal {
LIB_DEFINE(InitLibcInternal_1);
} // namespace LibcInternal

LIB_DEFINE(InitLibC_1);
LIB_DEFINE(InitLibKernel_1);
LIB_DEFINE(InitVideoOut_1);
LIB_DEFINE(InitSysmodule_1);
LIB_DEFINE(InitDiscMap_1);
LIB_DEFINE(InitDebug_1);
LIB_DEFINE(InitGraphicsDriver_1);
LIB_DEFINE(InitSystemService_1);
LIB_DEFINE(InitUserService_1);
LIB_DEFINE(InitPad_1);

bool Init(const String& id, Loader::SymbolDatabase* s)
{
	LIB_CHECK(U"libc_1", InitLibC_1);
	LIB_CHECK(U"libc_internal_1", LibcInternal::InitLibcInternal_1);
	LIB_CHECK(U"libkernel_1", InitLibKernel_1);
	LIB_CHECK(U"libVideoOut_1", InitVideoOut_1);
	LIB_CHECK(U"libSysmodule_1", InitSysmodule_1);
	LIB_CHECK(U"libDiscMap_1", InitDiscMap_1);
	LIB_CHECK(U"libDebug_1", InitDebug_1);
	LIB_CHECK(U"libGraphicsDriver_1", InitGraphicsDriver_1);
	LIB_CHECK(U"libUserService_1", InitUserService_1);
	LIB_CHECK(U"libSystemService_1", InitSystemService_1);
	LIB_CHECK(U"libPad_1", InitPad_1);

	return false;
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
