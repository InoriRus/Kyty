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

} // namespace Kyty::Libs::Dialog

#endif // KYTY_EMU_ENABLED
