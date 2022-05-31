#include "Kyty/Core/Timer.h"

#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"
#include "Emulator/Loader/Timer.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader::Timer {

static Core::Timer g_timer;

KYTY_SUBSYSTEM_INIT(Timer)
{
	Start();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Timer) {}

KYTY_SUBSYSTEM_DESTROY(Timer) {}

void Start()
{
	g_timer.Start();
}

double GetTimeMs()
{
	return g_timer.GetTimeMs();
}

Core::Time GetTime()
{
	return Core::Time(static_cast<int>(GetTimeMs()));
}

uint64_t GetCounter()
{
	return g_timer.GetTicks();
}

uint64_t GetFrequency()
{
	return g_timer.GetFrequency();
}

} // namespace Kyty::Loader::Timer

#endif // KYTY_EMU_ENABLED
