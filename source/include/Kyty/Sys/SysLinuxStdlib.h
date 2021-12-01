#ifndef SYS_LINUX_INCLUDE_KYTY_SYSSTDLIB_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSSTDLIB_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

namespace Kyty {

inline float sys_strtof(const char* nptr, char** endptr)
{
	return strtof(nptr, endptr);
}

inline double sys_strtod(const char* nptr, char** endptr)
{
	return strtod(nptr, endptr);
}

inline int32_t sys_strtoi32(const char* nptr, char** endptr, int base)
{
	long r = strtol(nptr, endptr, base);
	if (r >= static_cast<long>(INT32_MAX))
	{
		return INT32_MAX;
	}
	if (r <= static_cast<long>(INT32_MIN))
	{
		return INT32_MIN;
	}
	return static_cast<int32_t>(r);
}

inline uint32_t sys_strtoui32(const char* nptr, char** endptr, int base)
{
	return strtoul(nptr, endptr, base);
}

inline int64_t sys_strtoi64(const char* nptr, char** endptr, int base)
{
	return strtoll(nptr, endptr, base);
}

inline uint64_t sys_strtoui64(const char* nptr, char** endptr, int base)
{
	return strtoull(nptr, endptr, base);
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSSTDLIB_H_ */
