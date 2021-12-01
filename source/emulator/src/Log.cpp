#include "Emulator/Log.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Common.h"
#include "Emulator/Config.h"

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#include <windows.h>
// IWYU pragma: no_include <handleapi.h>
// IWYU pragma: no_include <minwindef.h>
// IWYU pragma: no_include <processenv.h>
#endif

#ifdef KYTY_EMU_ENABLED

namespace Kyty {

namespace Log {

static bool         g_log_initialized = false;
static Core::Mutex* g_mutex           = nullptr;
static Direction    g_dir             = Direction::Console;
static Core::File*  g_file            = nullptr;
static bool         g_colored_printf  = false;

static bool EnableVTMode()
{
#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	// Set output mode to handle virtual terminal sequences
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	if (h == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dw_mode = 0;
	if (GetConsoleMode(h, &dw_mode) == 0)
	{
		return false;
	}

	dw_mode |= static_cast<DWORD>(ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	return (SetConsoleMode(h, dw_mode) != 0);
#endif
	return true;
}

bool IsColoredPrintf()
{
	return g_colored_printf;
}

String RemoveColors(const String& str)
{
	uint32_t start = 0;
	String   ret;
	for (;;)
	{
		auto index = str.FindIndex(U'\x1b', start);
		if (!str.IndexValid(index))
		{
			ret += str.Mid(start);
			break;
		}
		ret += str.Mid(start, index - start);
		index = str.FindIndex(U'm', index);
		if (!str.IndexValid(index))
		{
			break;
		}
		start = index + 1;
	}
	return ret;
}

static void Close()
{
	if (g_log_initialized)
	{
		g_mutex->Lock();
		if (g_dir == Direction::File && g_file != nullptr)
		{
			g_file->Flush();
			g_file->Close();
			delete g_file;
			g_file = nullptr;
		}
		g_mutex->Unlock();
	}
}

KYTY_SUBSYSTEM_INIT(Log)
{
	if (!g_log_initialized)
	{
		g_mutex           = new Core::Mutex;
		g_log_initialized = true;
	}

	auto dir = Config::GetPrintfDirection();
	SetDirection(dir);
	if (dir == Log::Direction::File)
	{
		SetOutputFile(Config::GetPrintfOutputFile());
	}
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Log)
{
	Close();
}

KYTY_SUBSYSTEM_DESTROY(Log)
{
	Close();
}

void SetDirection(Direction dir)
{
	EXIT_IF(!Log::g_log_initialized);
	EXIT_IF(!Core::Thread::IsMainThread());

	if (dir == Direction::Console)
	{
		g_colored_printf = EnableVTMode();

		if (!g_colored_printf)
		{
			::printf("Colored printf is not supported\n");
		}
	} else
	{
		g_colored_printf = false;
	}

	g_dir = dir;
}

void SetOutputFile(const String& file_name, Core::File::Encoding enc)
{
	EXIT_IF(!Log::g_log_initialized);
	EXIT_IF(!Core::Thread::IsMainThread());
	EXIT_IF(Log::g_dir != Log::Direction::File);
	EXIT_IF(Log::g_file != nullptr);

	g_file = new Core::File;
	g_file->Create(file_name);

	if (g_file->IsInvalid())
	{
		::printf("Can't create log file: %s\n", file_name.C_Str());
		delete g_file;
		g_file = nullptr;
	} else
	{
		g_file->SetEncoding(enc);
		g_file->WriteBOM();
	}
}

} // namespace Log

void printf(const char* format, ...)
{
	EXIT_IF(!Log::g_log_initialized);

	if (Log::g_dir == Log::Direction::Silent)
	{
		return;
	}

	EXIT_IF(Log::g_mutex == nullptr);

	Log::g_mutex->Lock();
	{
		va_list args {};
		va_start(args, format);
		String s;
		s.Printf(format, args);
		va_end(args);

		if (!Log::g_colored_printf)
		{
			s = Log::RemoveColors(s);
		}

		if (Log::g_dir == Log::Direction::Console)
		{
			::printf("%s", s.C_Str());
		} else if (Log::g_dir == Log::Direction::File && Log::g_file != nullptr)
		{
			Log::g_file->Write(s);
		}
	}
	Log::g_mutex->Unlock();
}

} // namespace Kyty

#endif // KYTY_EMU_ENABLED
