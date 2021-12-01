#ifndef EMULATOR_INCLUDE_EMULATOR_PROFILER_H_
#define EMULATOR_INCLUDE_EMULATOR_PROFILER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

#define BUILD_WITH_EASY_PROFILER
#define EASY_PROFILER_STATIC

#include <easy/profiler.h> // IWYU pragma: export

//
#include "easy/details/profiler_aux.h"    // IWYU pragma: export
#include "easy/details/profiler_colors.h" // IWYU pragma: export

#include <cwchar> // IWYU pragma: export

// IWYU pragma: no_include "easy/profiler.h"

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#define KYTY_PROFILER_BLOCK(f, ...) EASY_BLOCK(f, __VA_ARGS__);
#else
#define KYTY_PROFILER_BLOCK(f, s...) EASY_BLOCK(f, ##s);
#endif

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#define KYTY_PROFILER_FUNCTION(f, ...) EASY_FUNCTION(f, __VA_ARGS__);
#else
#define KYTY_PROFILER_FUNCTION(f, s...) EASY_FUNCTION(f, ##s);
#endif

#define KYTY_PROFILER_END_BLOCK EASY_END_BLOCK

#define KYTY_PROFILER_THREAD(f) EASY_THREAD(f)

namespace Kyty::Profiler {

KYTY_SUBSYSTEM_DEFINE(Profiler);

} // namespace Kyty::Profiler

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_PROFILER_H_ */
