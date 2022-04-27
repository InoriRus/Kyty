#ifndef EMULATOR_INCLUDE_EMULATOR_DIALOG_H_
#define EMULATOR_INCLUDE_EMULATOR_DIALOG_H_

#include "Kyty/Core/Common.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Dialog {

namespace CommonDialog {

int KYTY_SYSV_ABI CommonDialogInitialize();

} // namespace CommonDialog

namespace SaveDataDialog {

int KYTY_SYSV_ABI SaveDataDialogUpdateStatus();
int KYTY_SYSV_ABI SaveDataDialogTerminate();
int KYTY_SYSV_ABI SaveDataDialogProgressBarSetValue(int target, uint32_t rate);

} // namespace SaveDataDialog

} // namespace Kyty::Libs::Dialog

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_DIALOG_H_ */
