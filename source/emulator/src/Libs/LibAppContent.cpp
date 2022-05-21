#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("AppContent", 1, "AppContentUtil", 1, 1);

namespace AppContent {

struct AppContentInitParam
{
	char reserved[32];
};

struct AppContentBootParam
{
	char     reserved1[4];
	uint32_t attr;
	char     reserved2[32];
};

struct NpUnifiedEntitlementLabel
{
	char data[17];
	char padding[3];
};

struct AppContentAddcontInfo
{
	NpUnifiedEntitlementLabel entitlement_label;
	uint32_t                  status;
};

int KYTY_SYSV_ABI AppContentInitialize(const AppContentInitParam* init_param, AppContentBootParam* boot_param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(init_param == nullptr);
	EXIT_NOT_IMPLEMENTED(boot_param == nullptr);

	boot_param->attr = 0;

	return OK;
}

int KYTY_SYSV_ABI AppContentGetAddcontInfoList(uint32_t service_label, AppContentAddcontInfo* /*list*/, uint32_t list_num,
                                               uint32_t* hit_num)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(hit_num == nullptr);

	printf("\t service_label = %u\n", service_label);
	printf("\t list_num      = %u\n", list_num);

	*hit_num = 0;

	return OK;
}

} // namespace AppContent

LIB_DEFINE(InitAppContent_1)
{
	LIB_FUNC("R9lA82OraNs", AppContent::AppContentInitialize);
	LIB_FUNC("xnd8BJzAxmk", AppContent::AppContentGetAddcontInfoList);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
