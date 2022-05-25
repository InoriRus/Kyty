#ifndef INCLUDE_KYTY_SYS_SYSSWAPBYTEORDER_H_
#define INCLUDE_KYTY_SYS_SYSSWAPBYTEORDER_H_

#include "Kyty/Core/Common.h"

#include <type_traits>

namespace Kyty {

inline uint16_t SwapByteOrder16(uint16_t value)
{
#if KYTY_COMPILER == KYTY_COMPILER_MSVC
	return _byteswap_ushort(value);
#else
	uint16_t hi = value << 8u;
	uint16_t lo = value >> 8u;
	return hi | lo;
#endif
}

inline uint32_t SwapByteOrder32(uint32_t value)
{
#if KYTY_COMPILER == KYTY_COMPILER_CLANG
	return __builtin_bswap32(value);
#elif KYTY_COMPILER == KYTY_COMPILER_MSVC
	return _byteswap_ulong(value);
#else
	uint32_t Byte0 = value & 0x000000FFu;
	uint32_t Byte1 = value & 0x0000FF00u;
	uint32_t Byte2 = value & 0x00FF0000u;
	uint32_t Byte3 = value & 0xFF000000u;
	return (Byte0 << 24u) | (Byte1 << 8u) | (Byte2 >> 8u) | (Byte3 >> 24u);
#endif
}

inline uint64_t SwapByteOrder64(uint64_t value)
{
#if KYTY_COMPILER == KYTY_COMPILER_CLANG
	return __builtin_bswap64(value);
#elif KYTY_COMPILER == KYTY_COMPILER_MSVC
	return _byteswap_uint64(value);
#else
	uint64_t Hi = SwapByteOrder32(uint32_t(value));
	uint32_t Lo = SwapByteOrder32(uint32_t(value >> 32u));
	return (Hi << 32u) | Lo;
#endif
}

template <typename T>
inline void SwapByteOrder(T& x)
{
	if (sizeof(x) == 2)
	{
		if (std::is_signed_v<T>)
		{
			x = static_cast<std::make_signed_t<T>>(SwapByteOrder16(static_cast<std::make_unsigned_t<uint16_t>>(x)));
		} else
		{
			x = SwapByteOrder16(x);
		}
	}
	if (sizeof(x) == 4)
	{
		if (std::is_signed_v<T>)
		{
			x = static_cast<std::make_signed_t<T>>(SwapByteOrder32(static_cast<std::make_unsigned_t<uint32_t>>(x)));
		} else
		{
			x = SwapByteOrder32(x);
		}
	}
	if (sizeof(x) == 8)
	{
		if (std::is_signed_v<T>)
		{
			x = static_cast<std::make_signed_t<T>>(SwapByteOrder64(static_cast<std::make_unsigned_t<uint64_t>>(x)));
		} else
		{
			x = SwapByteOrder64(x);
		}
	}
}

template <typename T>
inline void NoSwapByteOrder(T& x)
{
}

} // namespace Kyty

#endif /* INCLUDE_KYTY_SYS_SYSSWAPBYTEORDER_H_ */
