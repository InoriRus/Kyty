#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#include <cstdio>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("UserService", 1, "UserService", 1, 1);

namespace UserService {

struct UserServiceLoginUserIdList
{
	int user_id[4];
};

enum UserServiceEventType
{
	UserServiceEventTypeLogin,
	UserServiceEventTypeLogout
};

struct SceUserServiceEvent
{
	UserServiceEventType event_type;
	int                  user_id;
};

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

static KYTY_SYSV_ABI int UserServiceGetEvent(SceUserServiceEvent* event)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(event == nullptr);

	static bool logged_in = false;

	if (!logged_in)
	{
		logged_in         = true;
		event->event_type = UserServiceEventTypeLogin;
		event->user_id    = 1;
		return OK;
	}

	return USER_SERVICE_ERROR_NO_EVENT;
}

static KYTY_SYSV_ABI int UserServiceGetLoginUserIdList(UserServiceLoginUserIdList* user_id_list)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(user_id_list == nullptr);

	user_id_list->user_id[0] = 1;
	user_id_list->user_id[1] = -1;
	user_id_list->user_id[2] = -1;
	user_id_list->user_id[3] = -1;

	return OK;
}

static KYTY_SYSV_ABI int UserServiceGetUserName(int user_id, char* name, size_t size)
{
	EXIT_NOT_IMPLEMENTED(user_id != 1);
	EXIT_NOT_IMPLEMENTED(size < 5);

	int s = snprintf(name, size, "%s", "Kyty");

	EXIT_NOT_IMPLEMENTED(static_cast<size_t>(s) >= size);

	return OK;
}

} // namespace UserService

LIB_DEFINE(InitUserService_1)
{
	LIB_FUNC("j3YMu1MVNNo", UserService::UserServiceInitialize);
	LIB_FUNC("CdWp0oHWGr0", UserService::UserServiceGetInitialUser);
	LIB_FUNC("yH17Q6NWtVg", UserService::UserServiceGetEvent);
	LIB_FUNC("fPhymKNvK-A", UserService::UserServiceGetLoginUserIdList);
	LIB_FUNC("1xxcMiGu2fo", UserService::UserServiceGetUserName);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
