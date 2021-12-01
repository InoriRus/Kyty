#ifndef SYS_LINUX_INCLUDE_KYTY_SYSSTDIO_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSSTDIO_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

//#include <stdio.h>

namespace Kyty {

inline uint32_t sys_vscprintf(const char* format, va_list argptr)
{
	int     len;
	va_list argcopy;
	va_copy(argcopy, argptr);
	len = vsnprintf(0, 0, format, argcopy);
	va_end(argcopy);
	return len < 0 ? 0 : len;
}

inline uint32_t sys_vsnprintf(char* Dest, size_t Count, const char* Format, va_list Args)
{
	int len = vsnprintf(Dest, Count + 1, Format, Args);
	return len < 0 ? 0 : len;
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSSTDIO_H_ */
