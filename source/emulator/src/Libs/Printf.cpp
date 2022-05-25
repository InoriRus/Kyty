//
// Original algorithm is from:
// 	https://github.com/mpaland/printf
// 	Marco Paland (info@paland.com)
// 	2014-2019, PALANDesign Hannover, Germany
//  licensed under The MIT License (MIT)

#include "Emulator/Libs/Printf.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/VaContext.h"

#include <cfloat>
#include <cmath>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

constexpr uint32_t FLAGS_ZEROPAD   = (1U << 0U);
constexpr uint32_t FLAGS_LEFT      = (1U << 1U);
constexpr uint32_t FLAGS_PLUS      = (1U << 2U);
constexpr uint32_t FLAGS_SPACE     = (1U << 3U);
constexpr uint32_t FLAGS_HASH      = (1U << 4U);
constexpr uint32_t FLAGS_UPPERCASE = (1U << 5U);
constexpr uint32_t FLAGS_CHAR      = (1U << 6U);
constexpr uint32_t FLAGS_SHORT     = (1U << 7U);
constexpr uint32_t FLAGS_LONG      = (1U << 8U);
constexpr uint32_t FLAGS_LONG_LONG = (1U << 9U);
constexpr uint32_t FLAGS_PRECISION = (1U << 10U);
constexpr uint32_t FLAGS_ADAPT_EXP = (1U << 11U);

constexpr size_t   PRINTF_NTOA_BUFFER_SIZE        = 32U;
constexpr size_t   PRINTF_FTOA_BUFFER_SIZE        = 32U;
constexpr double   PRINTF_MAX_FLOAT               = 1e9;
constexpr uint32_t PRINTF_DEFAULT_FLOAT_PRECISION = 6U;

using out_fct_type = void (*)(char character, Vector<char>* buffer, size_t idx, size_t /*maxlen*/);

// internal null output
static inline void _out_null(char character, Vector<char>* buffer, size_t /*idx*/, size_t /*maxlen*/)
{
	buffer->Add(character);
}

static inline bool _is_digit(char ch)
{
	return (ch >= '0') && (ch <= '9');
}

static unsigned int _atoi(const char** str)
{
	unsigned int i = 0U;
	while (_is_digit(**str))
	{
		i = i * 10U + static_cast<unsigned int>(*((*str)++) - '0');
	}
	return i;
}

static size_t _out_rev(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, const char* buf, size_t len, unsigned int width,
                       unsigned int flags)
{
	const size_t start_idx = idx;

	// pad spaces up to given width
	if ((flags & FLAGS_LEFT) == 0 && (flags & FLAGS_ZEROPAD) == 0)
	{
		for (size_t i = len; i < width; i++)
		{
			out(' ', buffer, idx++, maxlen);
		}
	}

	// reverse string
	while (len != 0u)
	{
		out(buf[--len], buffer, idx++, maxlen);
	}

	// append pad spaces up to given width
	if ((flags & FLAGS_LEFT) != 0u)
	{
		while (idx - start_idx < width)
		{
			out(' ', buffer, idx++, maxlen);
		}
	}

	return idx;
}

// internal itoa format
static size_t _ntoa_format(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, char* buf, size_t len, bool negative,
                           unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
	// pad leading zeros
	if ((flags & FLAGS_LEFT) == 0u)
	{
		if ((width != 0u) && ((flags & FLAGS_ZEROPAD) != 0u) && (negative || ((flags & (FLAGS_PLUS | FLAGS_SPACE)) != 0u)))
		{
			width--;
		}
		while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = '0';
		}
		while (((flags & FLAGS_ZEROPAD) != 0u) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = '0';
		}
	}

	// handle hash
	if ((flags & FLAGS_HASH) != 0u)
	{
		if (((flags & FLAGS_PRECISION) == 0u) && (len != 0u) && ((len == prec) || (len == width)))
		{
			len--;
			if ((len != 0u) && (base == 16U))
			{
				len--;
			}
		}
		if ((base == 16U) && ((flags & FLAGS_UPPERCASE) == 0u) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'x';
		} else if ((base == 16U) && ((flags & FLAGS_UPPERCASE) != 0u) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'X';
		} else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE))
		{
			buf[len++] = 'b';
		}
		if (len < PRINTF_NTOA_BUFFER_SIZE)
		{
			buf[len++] = '0';
		}
	}

	if (len < PRINTF_NTOA_BUFFER_SIZE)
	{
		if (negative)
		{
			buf[len++] = '-';
		} else if ((flags & FLAGS_PLUS) != 0u)
		{
			buf[len++] = '+'; // ignore the space if the '+' exists
		} else if ((flags & FLAGS_SPACE) != 0u)
		{
			buf[len++] = ' ';
		}
	}

	return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

static size_t _ntoa_long_long(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, uint64_t value, bool negative,
                              uint64_t base, unsigned int prec, unsigned int width, unsigned int flags)
{
	char   buf[PRINTF_NTOA_BUFFER_SIZE];
	size_t len = 0U;

	// no hash for 0 values
	if (value == 0u)
	{
		flags &= ~FLAGS_HASH;
	}

	// write if precision != 0 and value is != 0
	if (((flags & FLAGS_PRECISION) == 0u) || (value != 0u))
	{
		do
		{
			const char digit = static_cast<char>(value % base);
			// NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
			buf[len++] = digit < 10 ? '0' + digit : ((flags & FLAGS_UPPERCASE) != 0u ? 'A' : 'a') + digit - 10;
			value /= base;
		} while ((value != 0u) && (len < PRINTF_NTOA_BUFFER_SIZE));
	}

	return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, static_cast<unsigned int>(base), prec, width, flags);
}

// internal itoa for 'long' type
static size_t _ntoa_long(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, uint32_t value, bool negative, uint32_t base,
                         unsigned int prec, unsigned int width, unsigned int flags)
{
	char   buf[PRINTF_NTOA_BUFFER_SIZE];
	size_t len = 0U;

	// no hash for 0 values
	if (value == 0u)
	{
		flags &= ~FLAGS_HASH;
	}

	// write if precision != 0 and value is != 0
	if (((flags & FLAGS_PRECISION) == 0u) || (value != 0u))
	{
		do
		{
			char digit = static_cast<char>(value % base);
			// NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
			buf[len++] = digit < 10 ? '0' + digit : ((flags & FLAGS_UPPERCASE) != 0u ? 'A' : 'a') + digit - 10;
			value /= base;
		} while ((value != 0u) && (len < PRINTF_NTOA_BUFFER_SIZE));
	}

	return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, static_cast<unsigned int>(base), prec, width, flags);
}

static size_t _etoa(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width,
                    unsigned int flags);

// internal ftoa for fixed decimal floating point
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static size_t _ftoa(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width,
                    unsigned int flags)
{
	char   buf[PRINTF_FTOA_BUFFER_SIZE];
	size_t len  = 0U;
	double diff = 0.0;

	// powers of 10
	static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

	// test for special values
	if (value != value)
	{
		return _out_rev(out, buffer, idx, maxlen, "nan", 3, width, flags);
	}
	if (value < -DBL_MAX)
	{
		return _out_rev(out, buffer, idx, maxlen, "fni-", 4, width, flags);
	}
	if (value > DBL_MAX)
	{
		return _out_rev(out, buffer, idx, maxlen, (flags & FLAGS_PLUS) != 0u ? "fni+" : "fni", (flags & FLAGS_PLUS) != 0u ? 4U : 3U, width,
		                flags);
	}

	// test for very large values
	// standard printf behavior is to print EVERY whole number digit -- which could be 100s of characters overflowing your buffers == bad
	if ((value > PRINTF_MAX_FLOAT) || (value < -PRINTF_MAX_FLOAT))
	{
		return _etoa(out, buffer, idx, maxlen, value, prec, width, flags);
	}

	// test for negative
	bool negative = false;
	if (value < 0)
	{
		negative = true;
		value    = 0 - value;
	}

	// set default precision, if not set explicitly
	if ((flags & FLAGS_PRECISION) == 0u)
	{
		prec = PRINTF_DEFAULT_FLOAT_PRECISION;
	}
	// limit precision to 9, cause a prec >= 10 can lead to overflow errors
	while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U))
	{
		buf[len++] = '0';
		prec--;
	}

	int    whole = static_cast<int>(value);
	double tmp   = (value - whole) * pow10[prec];
	auto   frac  = static_cast<uint32_t>(tmp);
	diff         = tmp - frac;

	if (diff > 0.5)
	{
		++frac;
		// handle rollover, e.g. case 0.99 with prec 1 is 1.0
		if (frac >= pow10[prec])
		{
			frac = 0;
			++whole;
		}
	} else if (diff < 0.5)
	{
	} else if ((frac == 0U) || ((frac & 1U) != 0u))
	{
		// if halfway, round up if odd OR if last digit is 0
		++frac;
	}

	if (prec == 0U)
	{
		diff = value - static_cast<double>(whole);
		if ((!(diff < 0.5) || (diff > 0.5)) && ((static_cast<uint32_t>(whole) & 1u) != 0))
		{
			// exactly 0.5 and ODD, then round up
			// 1.5 -> 2, but 2.5 -> 2
			++whole;
		}
	} else
	{
		unsigned int count = prec;
		// now do fractional part, as an unsigned number
		while (len < PRINTF_FTOA_BUFFER_SIZE)
		{
			--count;
			buf[len++] = static_cast<char>(48U + (frac % 10U));
			if ((frac /= 10U) == 0u)
			{
				break;
			}
		}
		// add extra 0s
		while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U))
		{
			buf[len++] = '0';
		}
		if (len < PRINTF_FTOA_BUFFER_SIZE)
		{
			// add decimal
			buf[len++] = '.';
		}
	}

	// do whole part, number is reversed
	while (len < PRINTF_FTOA_BUFFER_SIZE)
	{
		buf[len++] = static_cast<char>(48 + (whole % 10));
		if ((whole /= 10) == 0)
		{
			break;
		}
	}

	// pad leading zeros
	if (((flags & FLAGS_LEFT) == 0u) && ((flags & FLAGS_ZEROPAD) != 0u))
	{
		if ((width != 0u) && (negative || ((flags & (FLAGS_PLUS | FLAGS_SPACE)) != 0u)))
		{
			width--;
		}
		while ((len < width) && (len < PRINTF_FTOA_BUFFER_SIZE))
		{
			buf[len++] = '0';
		}
	}

	if (len < PRINTF_FTOA_BUFFER_SIZE)
	{
		if (negative)
		{
			buf[len++] = '-';
		} else if ((flags & FLAGS_PLUS) != 0u)
		{
			buf[len++] = '+'; // ignore the space if the '+' exists
		} else if ((flags & FLAGS_SPACE) != 0u)
		{
			buf[len++] = ' ';
		}
	}

	return _out_rev(out, buffer, idx, maxlen, buf, len, width, flags);
}

// internal ftoa variant for exponential floating-point type, contributed by Martijn Jasperse <m.jasperse@gmail.com>
static size_t _etoa(out_fct_type out, Vector<char>* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width,
                    unsigned int flags)
{
	// check for NaN and special values
	if ((value != value) || (value > DBL_MAX) || (value < -DBL_MAX))
	{
		return _ftoa(out, buffer, idx, maxlen, value, prec, width, flags);
	}

	// determine the sign
	const bool negative = value < 0;
	if (negative)
	{
		value = -value;
	}

	// default precision
	if ((flags & FLAGS_PRECISION) == 0u)
	{
		prec = PRINTF_DEFAULT_FLOAT_PRECISION;
	}

	// determine the decimal exponent
	// based on the algorithm by David Gay (https://www.ampl.com/netlib/fp/dtoa.c)
	union
	{
		uint64_t U;
		double   F;
	} conv {};

	conv.F   = value;
	int exp2 = static_cast<int>((conv.U >> 52U) & 0x07FFU) - 1023; // effectively log2
	conv.U   = (conv.U & ((1ULL << 52U) - 1U)) | (1023ULL << 52U); // drop the exponent so conv.F is now in [1,2)
	// now approximate log10 from the log2 integer part and an expansion of ln around 1.5
	int expval = static_cast<int>(0.1760912590558 + exp2 * 0.301029995663981 + (conv.F - 1.5) * 0.289529654602168);
	// now we want to compute 10^expval but we want to be sure it won't overflow
	// exp2            = static_cast<int>(expval * 3.321928094887362 + 0.5);
	exp2            = lround(expval * 3.321928094887362);
	const double z  = expval * 2.302585092994046 - exp2 * 0.6931471805599453;
	const double z2 = z * z;
	conv.U          = static_cast<uint64_t>(exp2 + 1023) << 52U;
	// compute exp(z) using continued fractions, see https://en.wikipedia.org/wiki/Exponential_function#Continued_fractions_for_ex
	conv.F *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
	// correct for rounding errors
	if (value < conv.F)
	{
		expval--;
		conv.F /= 10;
	}

	// the exponent format is "%+03d" and largest value is "307", so set aside 4-5 characters
	unsigned int minwidth = ((expval < 100) && (expval > -100)) ? 4U : 5U;

	// in "%g" mode, "prec" is the number of *significant figures* not decimals
	if ((flags & FLAGS_ADAPT_EXP) != 0u)
	{
		// do we want to fall-back to "%f" mode?
		if ((value >= 1e-4) && (value < 1e6))
		{
			if (static_cast<int>(prec) > expval)
			{
				prec = static_cast<unsigned>(static_cast<int>(prec) - expval - 1);
			} else
			{
				prec = 0;
			}
			flags |= FLAGS_PRECISION; // make sure _ftoa respects precision
			// no characters in exponent
			minwidth = 0U;
			expval   = 0;
		} else
		{
			// we use one sigfig for the whole part
			if ((prec > 0) && ((flags & FLAGS_PRECISION) != 0u))
			{
				--prec;
			}
		}
	}

	// will everything fit?
	unsigned int fwidth = width;
	if (width > minwidth)
	{
		// we didn't fall-back so subtract the characters required for the exponent
		fwidth -= minwidth;
	} else
	{
		// not enough characters, so go back to default sizing
		fwidth = 0U;
	}
	if (((flags & FLAGS_LEFT) != 0u) && (minwidth != 0u))
	{
		// if we're padding on the right, DON'T pad the floating part
		fwidth = 0U;
	}

	// rescale the float value
	if (expval != 0)
	{
		value /= conv.F;
	}

	// output the floating part
	const size_t start_idx = idx;
	idx                    = _ftoa(out, buffer, idx, maxlen, negative ? -value : value, prec, fwidth, flags & ~FLAGS_ADAPT_EXP);

	// output the exponent part
	if (minwidth != 0u)
	{
		// output the exponential symbol
		out((flags & FLAGS_UPPERCASE) != 0u ? 'E' : 'e', buffer, idx++, maxlen);
		// output the exponent value
		idx = _ntoa_long(out, buffer, idx, maxlen, (expval < 0) ? -expval : expval, expval < 0, 10, 0, minwidth - 1,
		                 FLAGS_ZEROPAD | FLAGS_PLUS);
		// might need to right-pad spaces
		if ((flags & FLAGS_LEFT) != 0u)
		{
			while (idx - start_idx < width)
			{
				out(' ', buffer, idx++, maxlen);
			}
		}
	}
	return idx;
}

static inline unsigned int _strnlen_s(const char* str, size_t maxsize)
{
	const char* s = nullptr;
	for (s = str; (*s != 0) && ((maxsize--) != 0u); ++s)
	{
		;
	}
	return static_cast<unsigned int>(s - str);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static int kyty_printf_internal(bool sn, char* sn_s, size_t sn_n, const char* format, VaList* va_list)
{
	Vector<char> buffer;

	uint32_t flags     = 0;
	uint32_t width     = 0;
	uint32_t precision = 0;
	uint32_t n         = 0;
	size_t   idx       = 0U;
	auto     maxlen    = static_cast<size_t>(-1);

	// use null output function
	auto out = _out_null;

	while (*format != 0)
	{
		// format specifier?  %[flags][width][.precision][length]
		if (*format != '%')
		{
			// no
			out(*format, &buffer, idx++, maxlen);
			format++;
			continue;
		}

		// yes, evaluate it
		format++;

		// evaluate flags
		flags = 0U;
		do
		{
			switch (*format)
			{
				case '0':
					flags |= FLAGS_ZEROPAD;
					format++;
					n = 1U;
					break;
				case '-':
					flags |= FLAGS_LEFT;
					format++;
					n = 1U;
					break;
				case '+':
					flags |= FLAGS_PLUS;
					format++;
					n = 1U;
					break;
				case ' ':
					flags |= FLAGS_SPACE;
					format++;
					n = 1U;
					break;
				case '#':
					flags |= FLAGS_HASH;
					format++;
					n = 1U;
					break;
				default: n = 0U; break;
			}
		} while (n != 0u);

		// evaluate width field
		width = 0U;
		if (_is_digit(*format))
		{
			width = _atoi(&format);
		} else if (*format == '*')
		{
			// const int w = va_arg(va, int);
			const int w = VaArg_int(va_list);
			if (w < 0)
			{
				flags |= FLAGS_LEFT; // reverse padding
				width = static_cast<unsigned int>(-w);
			} else
			{
				width = static_cast<unsigned int>(w);
			}
			format++;
		}

		// evaluate precision field
		precision = 0U;
		if (*format == '.')
		{
			flags |= FLAGS_PRECISION;
			format++;
			if (_is_digit(*format))
			{
				precision = _atoi(&format);
			} else if (*format == '*')
			{
				// const int prec = (int)va_arg(va, int);
				const int prec = VaArg_int(va_list);
				precision      = prec > 0 ? static_cast<unsigned int>(prec) : 0U;
				format++;
			}
		}

		// evaluate length field
		switch (*format)
		{
			case 'l':
				flags |= FLAGS_LONG;
				format++;
				if (*format == 'l')
				{
					flags |= FLAGS_LONG_LONG;
					format++;
				}
				break;
			case 'h':
				flags |= FLAGS_SHORT;
				format++;
				if (*format == 'h')
				{
					flags |= FLAGS_CHAR;
					format++;
				}
				break;
			case 't':
				flags |= (sizeof(ptrdiff_t) == sizeof(int32_t) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			case 'j':
				flags |= (sizeof(intmax_t) == sizeof(int32_t) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			case 'z':
				flags |= (sizeof(size_t) == sizeof(int32_t) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			default: break;
		}

		// evaluate specifier
		switch (*format)
		{
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
			case 'b':
			{
				// set the base
				unsigned int base = 0;
				if (*format == 'x' || *format == 'X')
				{
					base = 16U;
				} else if (*format == 'o')
				{
					base = 8U;
				} else if (*format == 'b')
				{
					base = 2U;
				} else
				{
					base = 10U;
					flags &= ~FLAGS_HASH; // no hash for dec format
				}
				// uppercase
				if (*format == 'X')
				{
					flags |= FLAGS_UPPERCASE;
				}

				// no plus or space flag for u, x, X, o, b
				if ((*format != 'i') && (*format != 'd'))
				{
					flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
				}

				// ignore '0' flag when precision is given
				if ((flags & FLAGS_PRECISION) != 0u)
				{
					flags &= ~FLAGS_ZEROPAD;
				}

				// convert the integer
				if ((*format == 'i') || (*format == 'd'))
				{
					// signed
					if ((flags & FLAGS_LONG_LONG) != 0u || (flags & FLAGS_LONG) != 0u)
					{
						// const long long value = va_arg(va, long long);
						auto value = VaArg_long_long(va_list);
						idx = _ntoa_long_long(out, &buffer, idx, maxlen, static_cast<uint64_t>(value > 0 ? value : 0 - value), value < 0,
						                      base, precision, width, flags);
					} else if ((flags & FLAGS_LONG) != 0u)
					{
						// const long value = va_arg(va, long);
						auto value = VaArg_long(va_list);
						idx = _ntoa_long(out, &buffer, idx, maxlen, static_cast<uint32_t>(value > 0 ? value : 0 - value), value < 0, base,
						                 precision, width, flags);
					} else
					{
						// const int value = (flags & FLAGS_CHAR)    ? (char)va_arg(va, int)
						//                 : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int)
						//                                       : va_arg(va, int);
						int value = (flags & FLAGS_CHAR) != 0u    ? static_cast<char>(VaArg_int(va_list))
						            : (flags & FLAGS_SHORT) != 0u ? static_cast<int16_t>(VaArg_int(va_list))
						                                          : VaArg_int(va_list);
						idx = _ntoa_long(out, &buffer, idx, maxlen, static_cast<unsigned int>(value > 0 ? value : 0 - value), value < 0,
						                 base, precision, width, flags);
					}
				} else
				{
					// unsigned
					if ((flags & FLAGS_LONG_LONG) != 0u || (flags & FLAGS_LONG) != 0u)
					{
						idx = _ntoa_long_long(out, &buffer, idx, maxlen, static_cast<uint64_t>(VaArg_long_long(va_list)), false, base,
						                      precision, width, flags);
					} else if ((flags & FLAGS_LONG) != 0u)
					{
						idx = _ntoa_long(out, &buffer, idx, maxlen, static_cast<uint32_t>(VaArg_long(va_list)), false, base, precision,
						                 width, flags);
					} else
					{
						const unsigned int value = (flags & FLAGS_CHAR) != 0u    ? static_cast<unsigned char>(VaArg_int(va_list))
						                           : (flags & FLAGS_SHORT) != 0u ? static_cast<uint16_t>(VaArg_int(va_list))
						                                                         : static_cast<unsigned int>(VaArg_int(va_list));
						idx                      = _ntoa_long(out, &buffer, idx, maxlen, value, false, base, precision, width, flags);
					}
				}
				format++;
				break;
			}
			case 'f':
			case 'F':
				if (*format == 'F')
				{
					flags |= FLAGS_UPPERCASE;
				}
				idx = _ftoa(out, &buffer, idx, maxlen, VaArg_double(va_list), precision, width, flags);
				format++;
				break;
			case 'e':
			case 'E':
			case 'g':
			case 'G':
				if ((*format == 'g') || (*format == 'G'))
				{
					flags |= FLAGS_ADAPT_EXP;
				}
				if ((*format == 'E') || (*format == 'G'))
				{
					flags |= FLAGS_UPPERCASE;
				}
				idx = _etoa(out, &buffer, idx, maxlen, VaArg_double(va_list), precision, width, flags);
				format++;
				break;
			case 'c':
			{
				unsigned int l = 1U;
				// pre padding
				if ((flags & FLAGS_LEFT) == 0u)
				{
					while (l++ < width)
					{
						out(' ', &buffer, idx++, maxlen);
					}
				}
				// char output
				out(static_cast<char>(VaArg_int(va_list)), &buffer, idx++, maxlen);
				// post padding
				if ((flags & FLAGS_LEFT) != 0u)
				{
					while (l++ < width)
					{
						out(' ', &buffer, idx++, maxlen);
					}
				}
				format++;
				break;
			}

			case 's':
			{
				// const char*  p = va_arg(va, char*);
				const char*  p = VaArg_ptr<const char>(va_list);
				unsigned int l = _strnlen_s(p, precision != 0u ? precision : static_cast<size_t>(-1));
				// pre padding
				if ((flags & FLAGS_PRECISION) != 0u)
				{
					l = (l < precision ? l : precision);
				}
				if ((flags & FLAGS_LEFT) == 0u)
				{
					while (l++ < width)
					{
						out(' ', &buffer, idx++, maxlen);
					}
				}
				// string output
				while ((*p != 0) && (((flags & FLAGS_PRECISION) == 0u) || ((precision--) != 0u)))
				{
					out(*(p++), &buffer, idx++, maxlen);
				}
				// post padding
				if ((flags & FLAGS_LEFT) != 0u)
				{
					while (l++ < width)
					{
						out(' ', &buffer, idx++, maxlen);
					}
				}
				format++;
				break;
			}

			case 'p':
			{
				width = sizeof(void*) * 2U;
				flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
				const bool is_ll = sizeof(uintptr_t) == sizeof(int64_t);
				if (is_ll)
				{
					idx = _ntoa_long_long(out, &buffer, idx, maxlen, reinterpret_cast<uintptr_t>(VaArg_ptr<void>(va_list)), false, 16U,
					                      precision, width, flags);
				} else
				{
					idx =
					    _ntoa_long(out, &buffer, idx, maxlen, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(VaArg_ptr<void>(va_list))),
					               false, 16U, precision, width, flags);
				}
				format++;
				break;
			}

			case '%':
				out('%', &buffer, idx++, maxlen);
				format++;
				break;

			default:
				out(*format, &buffer, idx++, maxlen);
				format++;
				break;
		}
	}

	// termination
	out(static_cast<char>(0), &buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

	if (sn)
	{
		int s = snprintf(sn_s, sn_n, "%s", buffer.GetDataConst());
		EXIT_NOT_IMPLEMENTED(static_cast<size_t>(s) >= sn_n);
	} else
	{
		printf(FG_BRIGHT_MAGENTA "%s" DEFAULT, buffer.GetDataConst());
	}

	// return written chars without terminating \0
	return static_cast<int>(idx);
}

static int kyty_vprintf(const char* format, VaList* va_list)
{
	return kyty_printf_internal(false, nullptr, 0, format, va_list);
}

static int kyty_printf_ctx(VaContext* ctx)
{
	const char* format = VaArg_ptr<const char>(&ctx->va_list);

	return kyty_printf_internal(false, nullptr, 0, format, &ctx->va_list);
}

static int kyty_snprintf_ctx(VaContext* ctx)
{
	char*       s      = VaArg_ptr<char>(&ctx->va_list);
	size_t      n      = VaArg_size_t(&ctx->va_list);
	const char* format = VaArg_ptr<const char>(&ctx->va_list);

	return kyty_printf_internal(true, s, n, format, &ctx->va_list);
}

static int KYTY_SYSV_ABI kyty_printf_std(VA_ARGS)
{
	VA_CONTEXT(ctx); // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)

	return kyty_printf_ctx(&ctx);
}

libc_printf_std_func_t GetPrintfStdFunc()
{
	return reinterpret_cast<libc_printf_std_func_t>(kyty_printf_std);
}

libc_printf_ctx_func_t GetPrintfCtxFunc()
{
	return kyty_printf_ctx;
}

libc_snprintf_ctx_func_t GetSnrintfCtxFunc()
{
	return kyty_snprintf_ctx;
}

libc_vprintf_func_t GetVprintfFunc()
{
	return kyty_vprintf;
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
