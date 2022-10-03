#include "Kyty/Core/DbgAssert.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Debug.h"
#include "Kyty/Core/Subsystems.h"

#include <cstdarg>

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#include <windows.h> // IWYU pragma: keep
// IWYU pragma: no_include <debugapi.h>
#endif

namespace Kyty::Core {

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS && KYTY_BUILD == KYTY_BUILD_DEBUG && KYTY_COMPILER == KYTY_COMPILER_CLANG
constexpr int PRINT_STACK_FROM = 4;
#else
constexpr int PRINT_STACK_FROM = 2;
#endif

#if KYTY_PLATFORM == KYTY_PLATFORM_LINUX
int IsDebuggerPresent()
{
	bool  dbg = false;
	FILE* f   = fopen("/proc/self/status", "r");

	if (f != nullptr)
	{
		char str[1024];

		while (feof(f) == 0)
		{
			str[1023] = '\0';
			int pid   = 0;

			[[maybe_unused]] auto* result = fgets(str, 1023, f);
			if (sscanf(str, "TracerPid: %d", &pid) == 1) // NOLINT
			{
				dbg = (pid != 0);
				break;
			}
		}

		[[maybe_unused]] auto result = fclose(f);
	}

	return (dbg ? 1 : 0);
}
#endif

void dbg_print_stack()
{
	DebugStack s;
	DebugStack::Trace(&s);

	printf("--- Stack Trace ---\n");
	s.Print(PRINT_STACK_FROM);
}

int dbg_assert_handler(char const* expr, char const* file, int line)
{
	dbg_print_stack();
	KYTY_LOGE("--- Fatal Error ---\n");
	KYTY_LOGE("Assertion (%s) failed in %s:%d\n", expr, file, line);
	SubsystemsListSingleton::Instance()->ShutdownAll();
	return 1;
}

int dbg_exit_if_handler(char const* expr, char const* file, int line)
{
	dbg_print_stack();
	KYTY_LOGE("--- Fatal Error ---\n");
	KYTY_LOGE("Error: condition (%s) is true in %s:%d\n", expr, file, line);
	SubsystemsListSingleton::Instance()->ShutdownAll();
	return 1;
}

int dbg_not_implemented_handler(char const* expr, char const* file, int line)
{
	dbg_print_stack();
	KYTY_LOGE("--- Fatal Error ---\n");
	KYTY_LOGE("Not implemented (%s) in %s:%d\n", expr, file, line);
	SubsystemsListSingleton::Instance()->ShutdownAll();
	return 1;
}

int dbg_exit_handler(char const* file, int line, const char* f, ...)
{
	va_list args {};
	va_start(args, f);

	dbg_print_stack();

	KYTY_LOGE("--- Error ---\n");
	vprintf(f, args);
	KYTY_LOGE(" in %s:%d\n", file, line);

	SubsystemsListSingleton::Instance()->ShutdownAll();

	va_end(args);

	return 1;
}

void dbg_exit(int status)
{
	std::_Exit(status);
}

bool dbg_is_debugger_present()
{
#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS || KYTY_PLATFORM == KYTY_PLATFORM_LINUX
	return !(IsDebuggerPresent() == 0);
#endif
	return false;
}

} // namespace Kyty::Core
