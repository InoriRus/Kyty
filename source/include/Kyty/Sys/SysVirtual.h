#ifndef INCLUDE_KYTY_SYS_SYSVIRTUAL_H_
#define INCLUDE_KYTY_SYS_SYSVIRTUAL_H_

#include "Kyty/Core/Common.h"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "Kyty/Sys/Windows/SysWindowsVirtual.h" // IWYU pragma: export
#elif defined(__linux__)
#include "Kyty/Sys/Linux/SysLinuxVirtual.h"     // IWYU pragma: export
#elif defined(__APPLE__)
#include "Kyty/Sys/MacOS/SysMacOSVirtual.h"     // IWYU pragma: export
#else
#   error "Unknown compiler"
#endif

#endif /* INCLUDE_KYTY_SYS_SYSVIRTUAL_H_ */
