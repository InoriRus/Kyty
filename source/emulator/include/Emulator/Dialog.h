#ifndef EMULATOR_INCLUDE_EMULATOR_DIALOG_H_
#define EMULATOR_INCLUDE_EMULATOR_DIALOG_H_

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Dialog {

namespace CommonDialog {

int KYTY_SYSV_ABI CommonDialogInitialize();

} // namespace CommonDialog

} // namespace Kyty::Libs::Dialog

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_DIALOG_H_ */
