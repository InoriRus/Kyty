#ifndef INCLUDE_KYTY_CORE_DBGASSERT_H_
#define INCLUDE_KYTY_CORE_DBGASSERT_H_

#include "Kyty/Core/Common.h"

#include <cstdlib> // IWYU pragma: keep

#ifndef KYTY_FINAL
#define ASSERT_ENABLED
#endif

namespace Kyty::Core {

#ifdef __clang__
int  dbg_exit_handler(char const* file, int line, const char* f, ...) KYTY_FORMAT_PRINTF(3, 4) __attribute__((analyzer_noreturn));
int  dbg_assert_handler(char const* expr, char const* file, int line) __attribute__((analyzer_noreturn));
int  dbg_exit_if_handler(char const* expr, char const* file, int line) __attribute__((analyzer_noreturn));
int  dbg_not_implemented_handler(char const* expr, char const* file, int line) __attribute__((analyzer_noreturn));
void dbg_exit(int status) __attribute__((analyzer_noreturn));
#else
int  dbg_exit_handler(char const* file, int line, const char* f, ...) KYTY_FORMAT_PRINTF(3, 4);
int  dbg_assert_handler(char const* expr, char const* file, int line);
int  dbg_exit_if_handler(char const* expr, char const* file, int line);
int  dbg_not_implemented_handler(char const* expr, char const* file, int line);
void dbg_exit(int status);
#endif

bool dbg_is_debugger_present();

} // namespace Kyty::Core

//#if KYTY_COMPILER == KYTY_COMPILER_MINGW
//#pragma GCC diagnostic ignored "-Warray-bounds"
//#endif

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#define ASSERT_HALT()                                                                                                                      \
	(Kyty::Core::dbg_is_debugger_present() ? (::fflush(nullptr), *(reinterpret_cast<volatile int*>(1)) = 0)                                \
	                                       : (Kyty::Core::dbg_exit(321), 1))
#else
#define ASSERT_HALT() (std::_Exit(321), 1)
#endif
//#define UNUSED(x) ((void)sizeof(x))

#ifdef ASSERT_ENABLED
#define ASSERT(x)  ((void)(!(x) && Kyty::Core::dbg_assert_handler(#x, __FILE__, __LINE__) != 0 && (ASSERT_HALT(), 1) != 0))
#define EXIT_IF(x) ((void)((x) && Kyty::Core::dbg_exit_if_handler(#x, __FILE__, __LINE__) != 0 && (ASSERT_HALT(), 1) != 0))
#else
//#define ASSERT(x) ((void)sizeof(x))
//#define EXIT_IF(x) ((void)sizeof(x))
#define ASSERT(x)                                                                                                                          \
	do                                                                                                                                     \
	{                                                                                                                                      \
		constexpr bool __emp_assert_tmp = false && (x);                                                                                    \
		(void)__emp_assert_tmp;                                                                                                            \
	} while (0)
#define EXIT_IF(x)                                                                                                                         \
	do                                                                                                                                     \
	{                                                                                                                                      \
		constexpr bool __emp_assert_tmp = false && (x);                                                                                    \
		(void)__emp_assert_tmp;                                                                                                            \
	} while (0)
#endif

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#define EXIT(f, ...)                                                                                                                       \
	{                                                                                                                                      \
		((void)(Kyty::Core::dbg_exit_handler(__FILE__, __LINE__, f, __VA_ARGS__) && (ASSERT_HALT(), 1)));                                  \
	}
#else
#define EXIT(f, s...)                                                                                                                      \
	{                                                                                                                                      \
		((void)(Kyty::Core::dbg_exit_handler(__FILE__, __LINE__, f, ##s) && (ASSERT_HALT(), 1)));                                          \
	}
#endif

#define EXIT_NOT_IMPLEMENTED(x)                                                                                                            \
	((void)((x) && Kyty::Core::dbg_not_implemented_handler(#x, __FILE__, __LINE__) != 0 && (ASSERT_HALT(), 1) != 0))
#define KYTY_NOT_IMPLEMENTED EXIT_NOT_IMPLEMENTED(true)

#endif /* INCLUDE_KYTY_CORE_DBGASSERT_H_ */
