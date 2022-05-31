#include "Emulator/Emulator.h"

#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

namespace Kyty::Emulator {

#ifdef KYTY_EMU_ENABLED
void kyty_reg();
#endif

KYTY_SUBSYSTEM_INIT(Emulator)
{
#ifdef KYTY_EMU_ENABLED
	kyty_reg();
#endif
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Emulator) {}

KYTY_SUBSYSTEM_DESTROY(Emulator) {}

} // namespace Kyty::Emulator
