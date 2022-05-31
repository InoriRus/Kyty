#ifndef EMULATOR_INCLUDE_EMULATOR_COMMON_H_
#define EMULATOR_INCLUDE_EMULATOR_COMMON_H_

#include "Kyty/Core/Common.h"

#if (KYTY_COMPILER == KYTY_COMPILER_CLANG || KYTY_COMPILER == KYTY_COMPILER_MINGW) && KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS &&            \
    KYTY_BITNESS == 64 && KYTY_ENDIAN == KYTY_ENDIAN_LITTLE && KYTY_ABI == KYTY_ABI_X86_64 && KYTY_PROJECT == KYTY_PROJECT_EMULATOR
#define KYTY_EMU_ENABLED
#endif

#ifdef KYTY_EMU_ENABLED

#include "Emulator/Log.h" // IWYU pragma: export

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KYTY_MS_ABI __attribute__((ms_abi))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KYTY_SYSV_ABI __attribute__((sysv_abi))

#endif

#endif /* EMULATOR_INCLUDE_EMULATOR_COMMON_H_ */
