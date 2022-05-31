#include "Emulator/Common.h"
#include "Emulator/Dialog.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

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

namespace LibSaveDataDialog {

LIB_VERSION("SaveDataDialog", 1, "SaveDataDialog", 1, 1);

namespace SaveDataDialog = Dialog::SaveDataDialog;

LIB_DEFINE(InitDialog_1_SaveDataDialog)
{
	LIB_FUNC("KK3Bdg1RWK0", SaveDataDialog::SaveDataDialogUpdateStatus);
	LIB_FUNC("YuH2FA7azqQ", SaveDataDialog::SaveDataDialogTerminate);
	LIB_FUNC("hay1CfTmLyA", SaveDataDialog::SaveDataDialogProgressBarSetValue);
}

} // namespace LibSaveDataDialog

LIB_DEFINE(InitDialog_1)
{
	LibCommonDialog::InitDialog_1_CommonDialog(s);
	LibSaveDataDialog::InitDialog_1_SaveDataDialog(s);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
