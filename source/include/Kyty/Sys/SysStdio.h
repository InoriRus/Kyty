#ifndef INCLUDE_KYTY_SYS_SYSSTDIO_H_
#define INCLUDE_KYTY_SYS_SYSSTDIO_H_

#include "Kyty/Core/Common.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "Kyty/Sys/Windows/SysWindowsStdio.h" //
#elif defined(__APPLE__)
#include "Kyty/Sys/MacOS/SysMacOSStdio.h"     //
#elif defined(__linux__)
#include "Kyty/Sys/Linux/SysLinuxStdio.h"     //
#else
#   error "Unknown compiler"
#endif

#endif /* INCLUDE_KYTY_SYS_SYSSTDIO_H_ */
