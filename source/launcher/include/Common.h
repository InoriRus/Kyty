#ifndef LAUNCHER_INCLUDE_COMMON_H_
#define LAUNCHER_INCLUDE_COMMON_H_

// IWYU pragma: begin_exports
#include <QArgument>
#include <QObject>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
// IWYU pragma: end_exports

#define KYTY_QT_CLASS_NO_COPY(name)                                                                                                        \
public:                                                                                                                                    \
	name(const name&) = delete;                /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name& operator=(const name&) = delete;     /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name(name&&) noexcept        = delete;     /* NOLINT(bugprone-macro-parentheses) */                                                    \
	name& operator=(name&&) noexcept = delete; /* NOLINT(bugprone-macro-parentheses) */

#define KYTY_QT_CLASS_DEFAULT_COPY(name)                                                                                                   \
public:                                                                                                                                    \
	name(const name&) = default;                /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name& operator=(const name&) = default;     /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name(name&&) noexcept        = default;     /* NOLINT(bugprone-macro-parentheses) */                                                   \
	name& operator=(name&&) noexcept = default; /* NOLINT(bugprone-macro-parentheses) */

#endif /* LAUNCHER_INCLUDE_COMMON_H_ */
