#include "Emulator/Profiler.h"

#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Config.h"

#include <easy/profiler.h>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Profiler {

void Close()
{
	auto dir = Config::GetProfilerDirection();
	if (dir == Config::ProfilerDirection::File || dir == Config::ProfilerDirection::FileAndNetwork)
	{
		profiler::dumpBlocksToFile(Config::GetProfilerOutputFile().C_Str());
	}
}

KYTY_SUBSYSTEM_INIT(Profiler)
{
	switch (Config::GetProfilerDirection())
	{
		case Config::ProfilerDirection::File: EASY_PROFILER_ENABLE; break;
		case Config::ProfilerDirection::Network: profiler::startListen(); break;
		case Config::ProfilerDirection::FileAndNetwork:
			EASY_PROFILER_ENABLE;
			profiler::startListen();
			break;
		case Config::ProfilerDirection::None:
		default: break;
	}
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Profiler)
{
	Close();
}

KYTY_SUBSYSTEM_DESTROY(Profiler)
{
	Close();
}

} // namespace Kyty::Profiler

#endif // KYTY_EMU_ENABLED
