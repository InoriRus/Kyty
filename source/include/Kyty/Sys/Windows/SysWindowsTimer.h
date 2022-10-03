#ifndef SYS_WIN32_INCLUDE_KYTY_SYSTIMER_H_
#define SYS_WIN32_INCLUDE_KYTY_SYSTIMER_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#include <windows.h>

namespace Kyty {

struct SysTimeStruct
{
	uint16_t Year;         // NOLINT(readability-identifier-naming)
	uint16_t Month;        // NOLINT(readability-identifier-naming)
	uint16_t Day;          // NOLINT(readability-identifier-naming)
	uint16_t Hour;         // NOLINT(readability-identifier-naming)
	uint16_t Minute;       // NOLINT(readability-identifier-naming)
	uint16_t Second;       // NOLINT(readability-identifier-naming)
	uint16_t Milliseconds; // NOLINT(readability-identifier-naming)
	bool     is_invalid;   // NOLINT(readability-identifier-naming)
};

struct SysFileTimeStruct
{
	FILETIME time;
	bool     is_invalid;
};

// NOLINTNEXTLINE(google-runtime-references)
inline void sys_file_to_system_time_utc(const SysFileTimeStruct& f, SysTimeStruct& t)
{
	SYSTEMTIME s;

	if (f.is_invalid || (FileTimeToSystemTime(&f.time, &s) == 0))
	{
		t.is_invalid = true;
		return;
	}

	t.is_invalid   = false;
	t.Year         = s.wYear;
	t.Month        = s.wMonth;
	t.Day          = s.wDay;
	t.Hour         = s.wHour;
	t.Minute       = s.wMinute;
	t.Second       = (s.wSecond == 60 ? 59 : s.wSecond);
	t.Milliseconds = s.wMilliseconds;
}

// NOLINTNEXTLINE(google-runtime-references)
inline void sys_time_t_to_system(time_t t, SysTimeStruct& s)
{
	SysFileTimeStruct ft {};
	LONGLONG          ll   = Int32x32To64(t, 10000000) + 116444736000000000;
	ft.time.dwLowDateTime  = static_cast<DWORD>(ll);
	ft.time.dwHighDateTime = static_cast<DWORD>(static_cast<uint64_t>(ll) >> 32u);
	ft.is_invalid          = false;
	sys_file_to_system_time_utc(ft, s);
}

// NOLINTNEXTLINE(google-runtime-references)
inline void sys_system_to_file_time_utc(const SysTimeStruct& f, SysFileTimeStruct& t)
{
	SYSTEMTIME s;

	s.wYear         = f.Year;
	s.wMonth        = f.Month;
	s.wDay          = f.Day;
	s.wHour         = f.Hour;
	s.wMinute       = f.Minute;
	s.wSecond       = f.Second;
	s.wMilliseconds = f.Milliseconds;

	t.is_invalid = (f.is_invalid || (SystemTimeToFileTime(&s, &t.time) == 0));
}

// Retrieves the current local date and time
// NOLINTNEXTLINE(google-runtime-references)
inline void sys_get_system_time(SysTimeStruct& t)
{
	SYSTEMTIME s;
	GetLocalTime(&s);

	t.is_invalid   = false;
	t.Year         = s.wYear;
	t.Month        = s.wMonth;
	t.Day          = s.wDay;
	t.Hour         = s.wHour;
	t.Minute       = s.wMinute;
	t.Second       = (s.wSecond == 60 ? 59 : s.wSecond);
	t.Milliseconds = s.wMilliseconds;
}

// Retrieves the current system date and time in Coordinated Universal Time (UTC).
// NOLINTNEXTLINE(google-runtime-references)
inline void sys_get_system_time_utc(SysTimeStruct& t)
{
	SYSTEMTIME s;
	GetSystemTime(&s);

	t.is_invalid   = false;
	t.Year         = s.wYear;
	t.Month        = s.wMonth;
	t.Day          = s.wDay;
	t.Hour         = s.wHour;
	t.Minute       = s.wMinute;
	t.Second       = (s.wSecond == 60 ? 59 : s.wSecond);
	t.Milliseconds = s.wMilliseconds;
}

inline void sys_query_performance_frequency(uint64_t* freq)
{
	LARGE_INTEGER f;
	QueryPerformanceFrequency(&f);
	*freq = f.QuadPart;
}

inline void sys_query_performance_counter(uint64_t* counter)
{
	LARGE_INTEGER c;
	QueryPerformanceCounter(&c);
	*counter = c.QuadPart;
}

} // namespace Kyty

#endif

#endif /* SYS_WIN32_INCLUDE_KYTY_SYSTIMER_H_ */
