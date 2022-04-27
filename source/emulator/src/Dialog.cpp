#include "Emulator/Dialog.h"

#include "Kyty/Core/String.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

// NOLINTNEXTLINE(modernize-concat-nested-namespaces)
namespace Kyty::Libs::Dialog {

namespace CommonDialog {

LIB_NAME("CommonDialog", "CommonDialog");

int KYTY_SYSV_ABI CommonDialogInitialize()
{
	PRINT_NAME();

	return OK;
}

} // namespace CommonDialog

namespace SaveDataDialog {

LIB_NAME("SaveDataDialog", "SaveDataDialog");

int KYTY_SYSV_ABI SaveDataDialogUpdateStatus()
{
	PRINT_NAME();

	return 0;
}

int KYTY_SYSV_ABI SaveDataDialogTerminate()
{
	PRINT_NAME();

	return 0;
}

int KYTY_SYSV_ABI SaveDataDialogProgressBarSetValue(int target, uint32_t rate)
{
	PRINT_NAME();

	printf("\t target = %d\n", target);
	printf("\t rate   = %u\n", rate);

	return OK;
}

} // namespace SaveDataDialog

} // namespace Kyty::Libs::Dialog

#endif // KYTY_EMU_ENABLED
