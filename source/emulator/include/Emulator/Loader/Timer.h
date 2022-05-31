#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_TIMER_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_TIMER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader::Timer {

KYTY_SUBSYSTEM_DEFINE(Timer);

void       Start();
double     GetTimeMs();
Core::Time GetTime();
uint64_t   GetCounter();
uint64_t   GetFrequency();

} // namespace Kyty::Loader::Timer

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_TIMER_H_ */
