#include "Emulator/Emulator.h"

#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Emulator {

void kyty_reg();

KYTY_SUBSYSTEM_INIT(Emulator)
{
	kyty_reg();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Emulator) {}

KYTY_SUBSYSTEM_DESTROY(Emulator) {}

} // namespace Kyty::Emulator

#endif // KYTY_EMU_ENABLED
