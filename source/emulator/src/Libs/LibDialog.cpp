#include "Emulator/Common.h"
#include "Emulator/Dialog.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

namespace LibCommonDialog {

LIB_VERSION("CommonDialog", 1, "CommonDialog", 1, 1);

namespace CommonDialog = Dialog::CommonDialog;

LIB_DEFINE(InitDialog_1_CommonDialog)
{
	LIB_FUNC("uoUpLGNkygk", CommonDialog::CommonDialogInitialize);
}

} // namespace LibCommonDialog

LIB_DEFINE(InitDialog_1)
{
	LibCommonDialog::InitDialog_1_CommonDialog(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
