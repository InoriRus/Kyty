#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("UserService", 1, "UserService", 1, 1);

namespace UserService {

static KYTY_SYSV_ABI int UserServiceInitialize(const void* /*params*/)
{
	PRINT_NAME();

	return 0;
}

static KYTY_SYSV_ABI int UserServiceGetInitialUser(int* user_id)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(user_id == nullptr);

	*user_id = 1;

	return 0;
}

} // namespace UserService

LIB_DEFINE(InitUserService_1)
{
	LIB_FUNC("j3YMu1MVNNo", UserService::UserServiceInitialize);
	LIB_FUNC("CdWp0oHWGr0", UserService::UserServiceGetInitialUser);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
