#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("UserService", 1, "UserService", 1, 1);

namespace UserService {

static KYTY_SYSV_ABI int UserServiceInitialize(const void* /*params*/)
{
	PRINT_NAME();

	return OK;
}

static KYTY_SYSV_ABI int UserServiceGetInitialUser(int* user_id)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(user_id == nullptr);

	*user_id = 1;

	return OK;
}

static KYTY_SYSV_ABI int UserServiceGetEvent(void* event)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(event == nullptr);

	return USER_SERVICE_ERROR_NO_EVENT;
}

} // namespace UserService

LIB_DEFINE(InitUserService_1)
{
	LIB_FUNC("j3YMu1MVNNo", UserService::UserServiceInitialize);
	LIB_FUNC("CdWp0oHWGr0", UserService::UserServiceGetInitialUser);
	LIB_FUNC("yH17Q6NWtVg", UserService::UserServiceGetEvent);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
