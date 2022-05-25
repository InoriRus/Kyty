#ifndef INCLUDE_KYTY_CORE_COMMON_H_
#define INCLUDE_KYTY_CORE_COMMON_H_

#include "kyty_config.h" // IWYU pragma: export

#if KYTY_COMPILER != KYTY_COMPILER_MSVC
#if __cplusplus < 201703L
#undef __cplusplus
#define __cplusplus 201703L
#endif
#endif

#if (KYTY_COMPILER == KYTY_COMPILER_MINGW)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage,cert-dcl51-cpp,cert-dcl37-c,bugprone-reserved-identifier)
#define __USE_MINGW_ANSI_STDIO 1
#endif

// IWYU pragma: begin_exports
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
// IWYU pragma: end_exports

#define KYTY_FORCE_LINK_THIS(x) int force_link_##x = 0;
#define KYTY_FORCE_LINK_THAT(x)                                                                                                            \
	void force_link_function_##x()                                                                                                         \
	{                                                                                                                                      \
		extern int force_link_##x;                                                                                                         \
		force_link_##x = 1;                                                                                                                \
	}

#define KYTY_CLASS_NO_COPY(name)                                                                                                           \
public:                                                                                                                                    \
	name(const name&)                = delete; /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name& operator=(const name&)     = delete; /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name(name&&) noexcept            = delete; /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name& operator=(name&&) noexcept = delete; /* NOLINT(bugprone-macro-parentheses) */

#define KYTY_CLASS_DEFAULT_COPY(name)                                                                                                      \
public:                                                                                                                                    \
	name(const name&)                = default; /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name& operator=(const name&)     = default; /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name(name&&) noexcept            = default; /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name& operator=(name&&) noexcept = default; /* NOLINT(bugprone-macro-parentheses) */

#if (KYTY_COMPILER == KYTY_COMPILER_MINGW || KYTY_COMPILER == KYTY_COMPILER_GCC)
#define KYTY_FORMAT_PRINTF(a, b) __attribute__((format(gnu_printf, a, b)))
#elif KYTY_COMPILER == KYTY_COMPILER_CLANG
#define KYTY_FORMAT_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define KYTY_FORMAT_PRINTF(a, b)
#endif

#if KYTY_PLATFORM == KYTY_PLATFORM_ANDROID
#include <android/log.h> // IWYU pragma: export
#define KYTY_LOG_TAG   "KYTY_Tag"
#define KYTY_LOGI(...) __android_log_print(ANDROID_LOG_INFO, KYTY_LOG_TAG, __VA_ARGS__)
#define KYTY_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, KYTY_LOG_TAG, __VA_ARGS__)
#else
#define KYTY_LOGI printf
#define KYTY_LOGE printf
#endif

#endif /* INCLUDE_KYTY_CORE_COMMON_H_ */
