#ifndef INCLUDE_KYTY_SCRIPTS_LUACPP_H_
#define INCLUDE_KYTY_SCRIPTS_LUACPP_H_

#include "Kyty/Core/Common.h"

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
//#define LUA_BUILD_AS_DLL
//#define LUA_USE_WINDOWS
#else
#define LUA_USE_LINUX
#endif

// IWYU pragma: begin_exports
extern "C" {
#include "lua.h"
//
#include "lualib.h"
//
#include "lauxlib.h"
//
}
// IWYU pragma: end_exports

#endif /* INCLUDE_KYTY_SCRIPTS_LUACPP_H_ */
