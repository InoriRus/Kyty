#ifndef INCLUDE_KYTY_CORE_MAGICENUM_H_
#define INCLUDE_KYTY_CORE_MAGICENUM_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/String8.h"

#include "magic_enum.hpp" // IWYU pragma: export

namespace Kyty::Core {

template <typename E>
inline String EnumName(E v)
{
	auto str = magic_enum::enum_name(v);
	return String::FromUtf8(str.data(), static_cast<uint32_t>(str.length()));
}

template <typename E>
inline String8 EnumName8(E v)
{
	auto str = magic_enum::enum_name(v);
	return String8(str.data(), static_cast<uint32_t>(str.length()));
}

template <typename E>
inline E EnumValue(const String& str, E default_value)
{
	auto v = magic_enum::enum_cast<E>(str.C_Str());
	if (v.has_value())
	{
		return v.value();
	}
	return default_value;
}

template <typename E>
inline E EnumValue(const String8& str, E default_value)
{
	auto v = magic_enum::enum_cast<E>(str.c_str());
	if (v.has_value())
	{
		return v.value();
	}
	return default_value;
}

#define KYTY_ENUM_RANGE(e, mx, mn)                                                                                                         \
	namespace magic_enum::customize {                                                                                                      \
	template <>                                                                                                                            \
	struct enum_range<e>                                                                                                                   \
	{                                                                                                                                      \
		static constexpr int min = (mx);                                                                                                   \
		static constexpr int max = (mn);                                                                                                   \
	};                                                                                                                                     \
	}

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_MAGICENUM_H_ */
