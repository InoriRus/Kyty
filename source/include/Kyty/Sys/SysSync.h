#ifndef INCLUDE_KYTY_SYS_SYSSYNC_H_
#define INCLUDE_KYTY_SYS_SYSSYNC_H_

#include "Kyty/Core/Common.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "Kyty/Sys/Windows/SysWindowsSync.h" // IWYU pragma: export
#elif defined(__linux__)
#include "Kyty/Sys/Linux/SysLinuxSync.h"     // IWYU pragma: export
#elif defined(__APPLE__)
#include "Kyty/Sys/MacOS/SysMacOSSync.h"     // IWYU pragma: export
#else
#   error "Unknown compiler"
#endif

#endif /* INCLUDE_KYTY_SYS_SYSSYNC_H_ */
