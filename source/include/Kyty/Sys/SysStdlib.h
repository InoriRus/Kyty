#ifndef INCLUDE_KYTY_SYS_SYSSTDLIB_H_
#define INCLUDE_KYTY_SYS_SYSSTDLIB_H_

#include "Kyty/Core/Common.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "Kyty/Sys/Windows/SysWindowsStdlib.h" // IWYU pragma: export
#elif defined(__linux__)
#include "Kyty/Sys/Linux/SysLinuxStdlib.h"     // IWYU pragma: export
#elif defined(__APPLE__)
#include "Kyty/Sys/MacOS/SysMacOSStdlib.h"     // IWYU pragma: export
#else
#   error "Unknown compiler"
#endif

#endif /* INCLUDE_KYTY_SYS_SYSSTDLIB_H_ */
