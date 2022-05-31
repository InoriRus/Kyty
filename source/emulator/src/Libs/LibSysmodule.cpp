#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("Sysmodule", 1, "Sysmodule", 1, 1);

namespace Sysmodule {

static KYTY_SYSV_ABI int SysmoduleLoadModule(uint16_t id)
{
	PRINT_NAME();

	printf("\t id = %d\n", static_cast<int>(id));

	return 0;
}

static KYTY_SYSV_ABI int SysmoduleUnloadModule(uint16_t id)
{
	PRINT_NAME();

	printf("\t id = %d\n", static_cast<int>(id));

	return 0;
}

static KYTY_SYSV_ABI int SysmoduleLoadModuleInternalWithArg(uint16_t id, int arg1, int arg2, int arg3, int* ret)
{
	PRINT_NAME();

	printf("\t id = %d\n", static_cast<int>(id));

	EXIT_IF(arg1 != 0);
	EXIT_IF(arg2 != 0);
	EXIT_IF(arg3 != 0);
	EXIT_IF(ret == nullptr);

	*ret = 0;

	return 0;
}

static KYTY_SYSV_ABI int SysmoduleIsLoaded(uint16_t id)
{
	PRINT_NAME();

	printf("\t id = %d\n", static_cast<int>(id));

	return 0;
}

} // namespace Sysmodule

LIB_DEFINE(InitSysmodule_1)
{
	LIB_FUNC("eR2bZFAAU0Q", Sysmodule::SysmoduleUnloadModule);
	LIB_FUNC("hHrGoGoNf+s", Sysmodule::SysmoduleLoadModuleInternalWithArg);
	LIB_FUNC("g8cM39EUZ6o", Sysmodule::SysmoduleLoadModule);
	LIB_FUNC("fMP5NHUOaMk", Sysmodule::SysmoduleIsLoaded);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
