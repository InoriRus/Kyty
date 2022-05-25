#include "Kyty/Core/String.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Sys/SysStdio.h"
#include "Kyty/Sys/SysStdlib.h"

#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#ifdef String
#undef String
#endif
#endif

namespace Kyty::Core {

KYTY_HASH_DEFINE_CALC(String)
{
	return key->Hash();
}

KYTY_HASH_DEFINE_EQUALS(String)
{
	return *key_a == *key_b;
}

struct CharPropRange
{
	CharProperty r[256];
};

extern uint8_t       g_char_prop_p[];
extern CharPropRange g_char_prop_r[];

static size_t strlen(const char32_t* str)
{
	const char32_t* eos = str;
	while (*eos++ != 0u)
	{
	}
	return (eos - str - 1);
}

static const CharProperty& char_get_prop(uint32_t code)
{
	return g_char_prop_r[g_char_prop_p[code >> 8u]].r[code & 0xffU];
}

static const int g_fc_extra_bytes_for_utf8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

static const uint32_t g_fc_utf8_magic[6] = {0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL};

int Char::ToUtf8(char32_t src, uint8_t* dst)
{
	int rn = 0;

	uint32_t u = src;

	if (u <= 0x7f)
	{
		rn     = 1;
		dst[0] = u;
	} else if (u < 0x7ff)
	{
		rn     = 2;
		dst[1] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[0] = (u & 0x3fU) + 0xc0U;
	} else if (u < 0xffff)
	{
		rn     = 3;
		dst[2] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[1] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[0] = (u & 0x3fU) + 0xe0U;
	} else if (u < 0x1fffff)
	{
		rn     = 4;
		dst[3] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[2] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[1] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[0] = (u & 0x3fU) + 0xf0U;
	} else if (u < 0x3ffffff)
	{
		rn     = 5;
		dst[4] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[3] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[2] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[1] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[0] = (u & 0x3fU) + 0xf8U;
	} else
	{
		rn     = 6;
		dst[5] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[4] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[3] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[2] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[1] = (u & 0x3fU) + 0x80U;
		u >>= 6u;
		dst[0] = (u & 0x3fU) + 0xfcU;
	}

	return rn;
}

int Char::ToUtf16(char32_t src, char16_t* dst)
{
	int rn = 0;

	uint32_t u = src;

	if (u <= 0xffff)
	{
		rn     = 1;
		dst[0] = u;
	} else
	{
		rn = 2;
		u -= 0x010000;
		dst[1] = (u & 0x3ffU) + 0xdc00U;
		u >>= 10u;
		dst[0] = (u & 0x3ffU) + 0xd800U; // u >>= 10;
	}

	return rn;
}

int Char::ToUtf32(char32_t src, char32_t* dst)
{
	dst[0] = src;
	return 1;
}

#define KYTY_CP(a, b)                                                                                                                      \
	case a: c = b; break;

int Char::ToCp866(char32_t src, uint8_t* dst)
{
	uint32_t      u = src;
	unsigned char c = 240;
	if (u < 128)
	{
		c = u;
	} else
	{
		switch (u)
		{
			// clang-format off
			KYTY_CP(0x0410, 128) KYTY_CP(0x0411, 129) KYTY_CP(0x0412, 130) KYTY_CP(0x0413, 131)
			KYTY_CP(0x0414, 132) KYTY_CP(0x0415, 133) KYTY_CP(0x0416, 134) KYTY_CP(0x0417, 135)
			KYTY_CP(0x0418, 136) KYTY_CP(0x0419, 137) KYTY_CP(0x041A, 138) KYTY_CP(0x041B, 139)
			KYTY_CP(0x041C, 140) KYTY_CP(0x041D, 141) KYTY_CP(0x041E, 142) KYTY_CP(0x041F, 143)

			KYTY_CP(0x0420, 144) KYTY_CP(0x0421, 145) KYTY_CP(0x0422, 146) KYTY_CP(0x0423, 147)
			KYTY_CP(0x0424, 148) KYTY_CP(0x0425, 149) KYTY_CP(0x0426, 150) KYTY_CP(0x0427, 151)
			KYTY_CP(0x0428, 152) KYTY_CP(0x0429, 153) KYTY_CP(0x042A, 154) KYTY_CP(0x042B, 155)
			KYTY_CP(0x042C, 156) KYTY_CP(0x042D, 157) KYTY_CP(0x042E, 158) KYTY_CP(0x042F, 159)

			KYTY_CP(0x0430, 160) KYTY_CP(0x0431, 161) KYTY_CP(0x0432, 162) KYTY_CP(0x0433, 163)
			KYTY_CP(0x0434, 164) KYTY_CP(0x0435, 165) KYTY_CP(0x0436, 166) KYTY_CP(0x0437, 167)
			KYTY_CP(0x0438, 168) KYTY_CP(0x0439, 169) KYTY_CP(0x043A, 170) KYTY_CP(0x043B, 171)
			KYTY_CP(0x043C, 172) KYTY_CP(0x043D, 173) KYTY_CP(0x043E, 174) KYTY_CP(0x043F, 175)

			KYTY_CP(0x2591, 176) KYTY_CP(0x2592, 177) KYTY_CP(0x2593, 178) KYTY_CP(0x2502, 179)
			KYTY_CP(0x2524, 180) KYTY_CP(0x2561, 181) KYTY_CP(0x2562, 182) KYTY_CP(0x2556, 183)
			KYTY_CP(0x2555, 184) KYTY_CP(0x2563, 185) KYTY_CP(0x2551, 186) KYTY_CP(0x2557, 187)
			KYTY_CP(0x255D, 188) KYTY_CP(0x255C, 189) KYTY_CP(0x255B, 190) KYTY_CP(0x2510, 191)

			KYTY_CP(0x2514, 192) KYTY_CP(0x2534, 193) KYTY_CP(0x252C, 194) KYTY_CP(0x251C, 195)
			KYTY_CP(0x2500, 196) KYTY_CP(0x253C, 197) KYTY_CP(0x255E, 198) KYTY_CP(0x255F, 199)
			KYTY_CP(0x255A, 200) KYTY_CP(0x2554, 201) KYTY_CP(0x2569, 202) KYTY_CP(0x2566, 203)
			KYTY_CP(0x2560, 204) KYTY_CP(0x2550, 205) KYTY_CP(0x256C, 206) KYTY_CP(0x2567, 207)

			KYTY_CP(0x2568, 208) KYTY_CP(0x2564, 209) KYTY_CP(0x2565, 210) KYTY_CP(0x2559, 211)
			KYTY_CP(0x2558, 212) KYTY_CP(0x2552, 213) KYTY_CP(0x2553, 214) KYTY_CP(0x256B, 215)
			KYTY_CP(0x256A, 216) KYTY_CP(0x2518, 217) KYTY_CP(0x250C, 218) KYTY_CP(0x2588, 219)
			KYTY_CP(0x2584, 220) KYTY_CP(0x258C, 221) KYTY_CP(0x2590, 222) KYTY_CP(0x2580, 223)

			KYTY_CP(0x0440, 224) KYTY_CP(0x0441, 225) KYTY_CP(0x0442, 226) KYTY_CP(0x0443, 227)
			KYTY_CP(0x0444, 228) KYTY_CP(0x0445, 229) KYTY_CP(0x0446, 230) KYTY_CP(0x0447, 231)
			KYTY_CP(0x0448, 232) KYTY_CP(0x0449, 233) KYTY_CP(0x044A, 234) KYTY_CP(0x044B, 235)
			KYTY_CP(0x044C, 236) KYTY_CP(0x044D, 237) KYTY_CP(0x044E, 238) KYTY_CP(0x044F, 239)

			KYTY_CP(0x0401, 240) KYTY_CP(0x0451, 241) KYTY_CP(0x0404, 242) KYTY_CP(0x0454, 243)
			KYTY_CP(0x0407, 244) KYTY_CP(0x0457, 245) KYTY_CP(0x040E, 246) KYTY_CP(0x045E, 247)
			KYTY_CP(0x00B0, 248) KYTY_CP(0x2219, 249) KYTY_CP(0x00B7, 250) KYTY_CP(0x221A, 251)
			KYTY_CP(0x2116, 252) KYTY_CP(0x00A4, 253) KYTY_CP(0x25A0, 254) KYTY_CP(0x00A0, 255)

			KYTY_CP(0x2193, 25)// ?
			KYTY_CP(0x2191, 24)// ?
			KYTY_CP(0x2192, 26)// ?
			KYTY_CP(0x2190, 27)// ?

			default: break;

			// clang-format on
		}
	}

	dst[0] = c;

	return 1;
}

int Char::ToCp1251(char32_t src, uint8_t* dst)
{
	uint32_t      u = src;
	unsigned char c = 240;
	if (u < 128)
	{
		c = u;
	} else
	{
		switch (u)
		{
			// clang-format off
			KYTY_CP(0x0402, 128) KYTY_CP(0x0403, 129) KYTY_CP(0x201A, 130) KYTY_CP(0x0453, 131)
			KYTY_CP(0x201E, 132) KYTY_CP(0x2026, 133) KYTY_CP(0x2020, 134) KYTY_CP(0x2021, 135)
			KYTY_CP(0x20AC, 136) KYTY_CP(0x2030, 137) KYTY_CP(0x0409, 138) KYTY_CP(0x2039, 139)
			KYTY_CP(0x040A, 140) KYTY_CP(0x040C, 141) KYTY_CP(0x040B, 142) KYTY_CP(0x040F, 143)

			KYTY_CP(0x0452, 144) KYTY_CP(0x2018, 145) KYTY_CP(0x2019, 146) KYTY_CP(0x201C, 147)
			KYTY_CP(0x201D, 148) KYTY_CP(0x2022, 149) KYTY_CP(0x2013, 150) KYTY_CP(0x2014, 151)
			/*CP(0x0000, 152)*/ KYTY_CP(0x2122, 153) KYTY_CP(0x0459, 154) KYTY_CP(0x203A, 155)
			KYTY_CP(0x045A, 156) KYTY_CP(0x045C, 157) KYTY_CP(0x045B, 158) KYTY_CP(0x045F, 159)

			KYTY_CP(0x00A0, 160) KYTY_CP(0x040E, 161) KYTY_CP(0x045E, 162) KYTY_CP(0x0408, 163)
			KYTY_CP(0x00A4, 164) KYTY_CP(0x0490, 165) KYTY_CP(0x00A6, 166) KYTY_CP(0x00A7, 167)
			KYTY_CP(0x0401, 168) KYTY_CP(0x00A9, 169) KYTY_CP(0x0404, 170) KYTY_CP(0x00AB, 171)
			KYTY_CP(0x00AC, 172) KYTY_CP(0x00AD, 173) KYTY_CP(0x00AE, 174) KYTY_CP(0x0407, 175)

			KYTY_CP(0x00B0, 176) KYTY_CP(0x00B1, 177) KYTY_CP(0x0406, 178) KYTY_CP(0x0456, 179)
			KYTY_CP(0x0491, 180) KYTY_CP(0x00B5, 181) KYTY_CP(0x00B6, 182) KYTY_CP(0x00B7, 183)
			KYTY_CP(0x0451, 184) KYTY_CP(0x2116, 185) KYTY_CP(0x0454, 186) KYTY_CP(0x00BB, 187)
			KYTY_CP(0x0458, 188) KYTY_CP(0x0405, 189) KYTY_CP(0x0455, 190) KYTY_CP(0x0457, 191)

			KYTY_CP(0x0410, 192) KYTY_CP(0x0411, 193) KYTY_CP(0x0412, 194) KYTY_CP(0x0413, 195)
			KYTY_CP(0x0414, 196) KYTY_CP(0x0415, 197) KYTY_CP(0x0416, 198) KYTY_CP(0x0417, 199)
			KYTY_CP(0x0418, 200) KYTY_CP(0x0419, 201) KYTY_CP(0x041A, 202) KYTY_CP(0x041B, 203)
			KYTY_CP(0x041C, 204) KYTY_CP(0x041D, 205) KYTY_CP(0x041E, 206) KYTY_CP(0x041F, 207)

			KYTY_CP(0x0420, 208) KYTY_CP(0x0421, 209) KYTY_CP(0x0422, 210) KYTY_CP(0x0423, 211)
			KYTY_CP(0x0424, 212) KYTY_CP(0x0425, 213) KYTY_CP(0x0426, 214) KYTY_CP(0x0427, 215)
			KYTY_CP(0x0428, 216) KYTY_CP(0x0429, 217) KYTY_CP(0x042A, 218) KYTY_CP(0x042B, 219)
			KYTY_CP(0x042C, 220) KYTY_CP(0x042D, 221) KYTY_CP(0x042E, 222) KYTY_CP(0x042F, 223)

			KYTY_CP(0x0430, 224) KYTY_CP(0x0431, 225) KYTY_CP(0x0432, 226) KYTY_CP(0x0433, 227)
			KYTY_CP(0x0434, 228) KYTY_CP(0x0435, 229) KYTY_CP(0x0436, 230) KYTY_CP(0x0437, 231)
			KYTY_CP(0x0438, 232) KYTY_CP(0x0439, 233) KYTY_CP(0x043A, 234) KYTY_CP(0x043B, 235)
			KYTY_CP(0x043C, 236) KYTY_CP(0x043D, 237) KYTY_CP(0x043E, 238) KYTY_CP(0x043F, 239)

			KYTY_CP(0x0440, 240) KYTY_CP(0x0441, 241) KYTY_CP(0x0442, 242) KYTY_CP(0x0443, 243)
			KYTY_CP(0x0444, 244) KYTY_CP(0x0445, 245) KYTY_CP(0x0446, 246) KYTY_CP(0x0447, 247)
			KYTY_CP(0x0448, 248) KYTY_CP(0x0449, 249) KYTY_CP(0x044A, 250) KYTY_CP(0x044B, 251)
			KYTY_CP(0x044C, 252) KYTY_CP(0x044D, 253) KYTY_CP(0x044E, 254) KYTY_CP(0x044F, 255)

			KYTY_CP(0x2193, 25)// ?
			KYTY_CP(0x2191, 24)// ?
			KYTY_CP(0x2192, 26)// ?
			KYTY_CP(0x2190, 27)// ?

			default: break;
			// clang-format on
		}
	}

	dst[0] = c;

	return 1;
}

bool Char::IsDecimal(char32_t ucs4)
{
	return char_get_prop(ucs4).decimal;
}

bool Char::IsAlpha(char32_t ucs4)
{
	return char_get_prop(ucs4).alpha;
}

bool Char::IsAlphaNum(char32_t ucs4)
{
	const CharProperty& p = char_get_prop(ucs4);
	return (p.alpha || p.decimal);
}

bool Char::IsLower(char32_t ucs4)
{
	return char_get_prop(ucs4).lower;
}

bool Char::IsUpper(char32_t ucs4)
{
	return char_get_prop(ucs4).upper;
}

bool Char::IsSpace(char32_t ucs4)
{
	return char_get_prop(ucs4).space;
}

bool Char::IsHex(char32_t ucs4)
{
	return char_get_prop(ucs4).hex;
}

int Char::HexDigit(char32_t ucs4)
{
	const CharProperty& p = char_get_prop(ucs4);
	return p.hex ? p.hex_data : -1;
}

int Char::DecimalDigit(char32_t ucs4)
{
	const CharProperty& p = char_get_prop(ucs4);
	return p.decimal ? p.hex_data : -1;
}

static char32_t MakeLower(char32_t* ucs4)
{
	const CharProperty& p = char_get_prop(*ucs4);
	if (p.upper)
	{
		*ucs4 += p.case_offset;
	}
	return *ucs4;
}

static char32_t MakeUpper(char32_t* ucs4)
{
	const CharProperty& p = char_get_prop(*ucs4);
	if (p.lower)
	{
		*ucs4 += p.case_offset;
	}
	return *ucs4;
}

char32_t Char::ToLower(char32_t ucs4)
{
	MakeLower(&ucs4);
	return ucs4;
}

char32_t Char::ToUpper(char32_t ucs4)
{
	MakeUpper(&ucs4);
	return ucs4;
}

char32_t Char::ReadUtf32(const char32_t** str)
{
	EXIT_IF(str == nullptr);
	EXIT_IF(*str == nullptr);

	char32_t ch = **str;
	(*str)++;
	if (ch == 0)
	{
		return U'\0';
	}

	return ch == 0xFEFF ? ReadUtf32(str) : ch;
}

char32_t Char::ReadUtf16(const char16_t** str)
{
	EXIT_IF(str == nullptr);
	EXIT_IF(*str == nullptr);

	char16_t b = 0;

	b = **str;
	(*str)++;
	if (b == 0)
	{
		return U'\0';
	}

	uint32_t ch = 0;

	if (b >= static_cast<char16_t>(0xd800) && b <= static_cast<char16_t>(0xdbff))
	{
		ch = b & 0x3ffU;
		ch <<= 10u;
		b = **str;
		(*str)++;
		if (b == 0)
		{
			return U'\0';
		}
		ch += b & 0x3ffU;
		ch += 0x010000;
	} else
	{
		ch = b;
	}

	return ch == 0xFEFF ? ReadUtf16(str) : ch;
}

char32_t Char::ReadUtf8(const uint8_t** str)
{
	EXIT_IF(str == nullptr);
	EXIT_IF(*str == nullptr);

	uint8_t b = 0;

	b = **str;
	(*str)++;
	if (b == 0)
	{
		return U'\0';
	}

	uint32_t ch = 0;

	int extra_bytes = g_fc_extra_bytes_for_utf8[b];

	EXIT_IF(extra_bytes > 5);

	for (int i = 0; i < extra_bytes; i++)
	{
		ch += b;
		ch <<= 6u;
		b = **str;
		(*str)++;
		if (b == 0)
		{
			return U'\0';
		}
	}
	ch += b;

	ch -= g_fc_utf8_magic[extra_bytes];

	return ch == 0xFEFF ? ReadUtf8(str) : ch;
}

char32_t Char::ReadCp866(const uint8_t** str)
{
	EXIT_IF(str == nullptr);
	EXIT_IF(*str == nullptr);

	const uint32_t tbl[128] = {
	    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
	    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
	    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
	    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
	    0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040E, 0x045E, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x2116, 0x00A4, 0x25A0, 0x00A0};

	uint8_t b = 0;

	b = **str;
	(*str)++;
	if (b == 0)
	{
		return U'\0';
	}

	if (b < 128)
	{
		return b;
	}

	return tbl[b - 128];
}

char32_t Char::ReadCp1251(const uint8_t** str)
{
	EXIT_IF(str == nullptr);
	EXIT_IF(*str == nullptr);

	const uint32_t tbl[128] = {
	    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x0401, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
	    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
	    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
	    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
	    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F};

	uint8_t b = 0;

	b = **str;
	(*str)++;
	if (b == 0)
	{
		return U'\0';
	}

	if (b < 128)
	{
		return b;
	}

	return tbl[b - 128];
}

bool Char::EqualAscii(const char32_t* utf32_str, const char* ascii_str)
{
	EXIT_IF(!utf32_str || !ascii_str);

	const auto* ascii_str_ptr = reinterpret_cast<const unsigned char*>(ascii_str);

	for (;;)
	{
		EXIT_IF(*ascii_str_ptr > 127);

		if (*utf32_str != *ascii_str_ptr)
		{
			return false;
		}

		if (*utf32_str == 0 || *ascii_str_ptr == 0)
		{
			break;
		}

		utf32_str++;
		ascii_str_ptr++;
	}

	return true;
}

bool Char::EqualAsciiNoCase(const char32_t* utf32_str, const char* ascii_str)
{
	EXIT_IF(!utf32_str || !ascii_str);

	const auto* ascii_str_ptr = reinterpret_cast<const unsigned char*>(ascii_str);

	for (;;)
	{
		EXIT_IF(*ascii_str_ptr > 127);

		if (!(Char::ToLower(*utf32_str) == *ascii_str_ptr || Char::ToUpper(*utf32_str) == *ascii_str_ptr))
		{
			return false;
		}

		if (*utf32_str == 0 || *ascii_str_ptr == 0)
		{
			break;
		}

		utf32_str++;
		ascii_str_ptr++;
	}

	return true;
}

bool Char::EqualAsciiN(const char32_t* utf32_str, const char* ascii_str, int n)
{
	EXIT_IF(!utf32_str || !ascii_str);

	const auto* ascii_str_ptr = reinterpret_cast<const unsigned char*>(ascii_str);

	for (;;)
	{
		if (n <= 0)
		{
			break;
		}

		EXIT_IF(*ascii_str_ptr > 127);

		if (*utf32_str != *ascii_str_ptr)
		{
			return false;
		}

		if (*utf32_str == 0 || *ascii_str_ptr == 0)
		{
			break;
		}

		utf32_str++;
		ascii_str_ptr++;
		n--;
	}

	return true;
}

bool Char::EqualAsciiNoCaseN(const char32_t* utf32_str, const char* ascii_str, int n)
{
	EXIT_IF(!utf32_str || !ascii_str);

	const auto* ascii_str_ptr = reinterpret_cast<const unsigned char*>(ascii_str);

	for (;;)
	{
		if (n <= 0)
		{
			break;
		}

		EXIT_IF(*ascii_str_ptr > 127);

		if (!(Char::ToLower(*utf32_str) == *ascii_str_ptr || Char::ToUpper(*utf32_str) == *ascii_str_ptr))
		{
			return false;
		}

		if (*utf32_str == 0 || *ascii_str_ptr == 0)
		{
			break;
		}

		utf32_str++;
		ascii_str_ptr++;
		n--;
	}

	return true;
}

static void copy_on_write(String::DataType** data)
{
	(*data)->CopyOnWrite(data);
}

static bool compare_equal(const String::DataType* data1, const String::DataType* data2, uint32_t from1, uint32_t from2, uint32_t size)
{
	const char32_t* p1 = data1->GetData() + from1;
	const char32_t* p2 = data2->GetData() + from2;

	for (uint32_t i = 0; i < size; i++)
	{
		if (*(p1) != *(p2))
		{
			return false;
		}
		p1++;
		p2++;
	}

	return true;
}

static bool compare_equal(const String::DataType* data1, char32_t data2, uint32_t from1)
{
	const char32_t* p1 = data1->GetData() + from1;

	return (*(p1) == data2);
}

static bool compare_equal_no_case(const String::DataType* data1, const String::DataType* data2, uint32_t from1, uint32_t from2,
                                  uint32_t size)
{
	const char32_t* p1 = data1->GetData() + from1;
	const char32_t* p2 = data2->GetData() + from2;

	for (uint32_t i = 0; i < size; i++)
	{
		if ((Char::ToUpper(*p1) != Char::ToUpper(*p2)) && (Char::ToLower(*p1) != Char::ToLower(*p2)))
		{
			return false;
		}
		p1++;
		p2++;
	}

	return true;
}

static bool compare_equal_no_case(const String::DataType* data1, char32_t data2, uint32_t from1)
{
	const char32_t* p1 = data1->GetData() + from1;

	return !((Char::ToUpper(*p1) != Char::ToUpper(data2)) && (Char::ToLower(*p1) != Char::ToLower(data2)));
}

String::String(): m_data(new DataType(1, false))
{
	(*m_data)[0] = U'\0';
}

String::String(const String& src): m_data(nullptr)
{
	//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
	src.m_data->CopyPtr(&m_data, src.m_data);
}

String::String(String&& src) noexcept: m_data(src.m_data)
{
	src.m_data = nullptr;
}

String& String::operator=(String&& src) noexcept
{
	if (m_data != src.m_data)
	{
		if (m_data != nullptr)
		{
			m_data->Release();
			m_data = nullptr;
		}

		m_data     = src.m_data;
		src.m_data = nullptr;
	}
	return *this;
}

String::String(char32_t ch, uint32_t repeat): m_data(new DataType(repeat + 1, false))
{
	for (uint32_t i = 0; i < repeat; i++)
	{
		(*m_data)[i] = ch;
	}
	(*m_data)[repeat] = U'\0';
}

String::String(const char32_t* str): String(str, static_cast<uint32_t>(strlen(str))) {}

String::String(const Utf8& utf8): m_data(nullptr)
{
	String s(reinterpret_cast<const uint8_t*>(utf8.GetData()), Char::ReadUtf8);
	s.m_data->CopyPtr(&m_data, s.m_data);
}

uint32_t String::Hash() const
{
	EXIT_IF(m_data == nullptr);

	return m_data->Hash();
}

bool String::EqualAscii(const char* ascii_str) const
{
	return Char::EqualAscii(GetDataConst(), ascii_str);
}

bool String::EqualAsciiNoCase(const char* ascii_str) const
{
	return Char::EqualAsciiNoCase(GetDataConst(), ascii_str);
}

String::String(const char32_t* array, uint32_t size): m_data(new DataType(size + 1, false))
{
	std::memcpy(m_data->GetData(), array, sizeof(char32_t) * size);
	(*m_data)[size] = U'\0';
}

String::~String()
{
	if (m_data != nullptr)
	{
		//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
		m_data->Release();
		m_data = nullptr;
	}
}

uint32_t String::Size() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = m_data->Size();

	EXIT_IF(size == 0);
	//	if (size == 0)
	//	{
	//		return 0;
	//	}

	return size - 1;
}

bool String::IsEmpty() const
{
	EXIT_IF(m_data == nullptr);

	return (m_data->Size() == 0 || m_data->At(0) == 0);
}

bool String::IsInvalid() const
{
	return (m_data == nullptr);
}

void String::Clear()
{
	//	data->Lock();
	//	if (data->DecRef() == 0)
	//	{
	//		data->Unlock();
	//		delete data;
	//	} else
	//	{
	//		data->Unlock();
	//	}
	if (m_data != nullptr)
	{
		m_data->Release();
		m_data = nullptr;
	}

	m_data       = new DataType(1, false);
	(*m_data)[0] = U'\0';
}

char32_t& String::operator[](uint32_t index)
{
	EXIT_IF(m_data == nullptr);
	EXIT_IF(index >= m_data->Size());

	copy_on_write(&m_data);
	return (*m_data)[index];
}

const char32_t& String::operator[](uint32_t index) const
{
	EXIT_IF(m_data == nullptr);
	EXIT_IF(index >= m_data->Size());

	return m_data->At(index);
}

const char32_t& String::At(uint32_t index) const
{
	EXIT_IF(m_data == nullptr);
	if (index >= m_data->Size())
	{
		EXIT_IF(index >= m_data->Size());
	}

	return m_data->At(index);
}

char32_t* String::GetData()
{
	EXIT_IF(m_data == nullptr);
	copy_on_write(&m_data);
	return m_data->GetData();
}

const char32_t* String::GetData() const
{
	EXIT_IF(m_data == nullptr);
	return m_data->GetDataConst();
}

const char32_t* String::GetDataConst() const
{
	EXIT_IF(m_data == nullptr);
	return m_data->GetDataConst();
}

// const char* String::c_str() const
//{
//	return utf8_str();
//}

String::Utf8 String::utf8_str() const // NOLINT(readability-identifier-naming)
{
	EXIT_IF(m_data == nullptr);

	Utf8 s;
	char buf[6];

	for (uint32_t i = 0; i < m_data->Size(); i++)
	{
		int num = Char::ToUtf8(m_data->At(i), reinterpret_cast<uint8_t*>(buf));
		s.Add(buf, num);
	}

	return s;
}

String::Utf16 String::utf16_str() const // NOLINT(readability-identifier-naming)
{
	EXIT_IF(m_data == nullptr);

	Utf16    s;
	char16_t buf[2];

	for (uint32_t i = 0; i < m_data->Size(); i++)
	{
		int num = Char::ToUtf16(m_data->At(i), buf);
		s.Add(buf, num);
	}

	return s;
}

String::Utf32 String::utf32_str() const // NOLINT(readability-identifier-naming)
{
	EXIT_IF(m_data == nullptr);

	Utf32    s;
	char32_t buf[1];

	for (uint32_t i = 0; i < m_data->Size(); i++)
	{
		int num = Char::ToUtf32(m_data->At(i), buf);
		s.Add(buf, num);
	}

	return s;
}

String::Cp866 String::cp866_str() const // NOLINT(readability-identifier-naming)
{
	EXIT_IF(m_data == nullptr);

	Cp866 s;
	char  buf[1];

	for (uint32_t i = 0; i < m_data->Size(); i++)
	{
		int num = Char::ToCp866(m_data->At(i), reinterpret_cast<uint8_t*>(buf));
		s.Add(buf, num);
	}

	return s;
}

String::Cp1251 String::cp1251_str() const // NOLINT(readability-identifier-naming)
{
	EXIT_IF(m_data == nullptr);

	Cp1251 s;
	char   buf[1];

	for (uint32_t i = 0; i < m_data->Size(); i++)
	{
		int num = Char::ToCp1251(m_data->At(i), reinterpret_cast<uint8_t*>(buf));
		s.Add(buf, num);
	}

	return s;
}

String& String::operator=(const String& src) // NOLINT(bugprone-unhandled-self-assignment, cert-oop54-cpp)
{
	if (m_data != src.m_data)
	{
		if (m_data != nullptr)
		{
			m_data->Release();
			m_data = nullptr;
		}

		src.m_data->CopyPtr(&m_data, src.m_data);
	}
	return *this;
}

String& String::operator=(char32_t ch)
{
	(*this) = String(ch);
	return *this;
}

String& String::operator=(const char* utf8_str)
{
	(*this) = String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8);
	return *this;
}

String& String::operator=(const char16_t* utf16_str)
{
	(*this) = String(utf16_str, Char::ReadUtf16);
	return *this;
}

String& String::operator=(const char32_t* utf32_str)
{
	(*this) = String(utf32_str);
	return *this;
}

String& String::operator+=(const String& src)
{
	EXIT_IF(m_data == nullptr);

	if (this == &src)
	{
		const String src2(src); // NOLINT(performance-unnecessary-copy-initialization)
		copy_on_write(&m_data);
		m_data->RemoveAt(m_data->Size() - 1);
		m_data->Add(src2.GetData(), src2.Size() + 1);
	} else
	{
		copy_on_write(&m_data);
		m_data->RemoveAt(m_data->Size() - 1);
		m_data->Add(src.GetData(), src.Size() + 1);
	}
	return *this;
}

String& String::operator+=(char32_t ch)
{
	(*this) += String(ch);
	return *this;
}

String& String::operator+=(const char* utf8_str)
{
	(*this) += String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8);
	return *this;
}

String& String::operator+=(const char16_t* utf16_str)
{
	(*this) += String(utf16_str, Char::ReadUtf16);
	return *this;
}

String& String::operator+=(const char32_t* utf32_str)
{
	(*this) += String(utf32_str);
	return *this;
}

String operator+(const String& str1, const String& str2)
{
	String s(str1);
	s += str2;
	return s;
}

String operator+(const char* utf8_str1, const String& str2)
{
	return String(reinterpret_cast<const uint8_t*>(utf8_str1), Char::ReadUtf8) + str2;
}

String operator+(const String& str1, const char* utf8_str2)
{
	return str1 + String(reinterpret_cast<const uint8_t*>(utf8_str2), Char::ReadUtf8);
}

String operator+(const char32_t* utf32_str1, const String& str2)
{
	String str1(utf32_str1);
	return str1 + str2;
}

String operator+(const String& str1, const char32_t* utf32_str2)
{
	return str1 + String(utf32_str2);
}

String operator+(char32_t ch, const String& str2)
{
	return String(ch) + str2;
}

String operator+(const String& str1, char32_t ch)
{
	return str1 + String(ch);
}

bool String::Equal(const String& src) const
{
	EXIT_IF(m_data == nullptr);

	if (m_data == src.m_data)
	{
		return true;
	}

	uint32_t size = this->Size();

	if (size != src.Size())
	{
		return false;
	}

	return compare_equal(m_data, src.m_data, 0, 0, size);
}

bool String::Equal(char32_t ch) const
{
	return this->Equal(String(ch));
}

bool String::Equal(const char* utf8_str) const
{
	return this->Equal(String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8));
}

bool String::Equal(const char32_t* utf32_str) const
{
	return this->Equal(String(utf32_str));
}

bool String::EqualNoCase(const String& src) const
{
	EXIT_IF(m_data == nullptr);

	if (m_data == src.m_data)
	{
		return true;
	}

	uint32_t size = this->Size();

	if (size != src.Size())
	{
		return false;
	}

	return compare_equal_no_case(m_data, src.m_data, 0, 0, size);
}

bool String::EqualNoCase(char32_t ch) const
{
	return this->EqualNoCase(String(ch));
}

bool String::EqualNoCase(const char* utf8_str) const
{
	return this->EqualNoCase(String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8));
}

bool String::EqualNoCase(const char32_t* utf32_str) const
{
	return this->EqualNoCase(String(utf32_str));
}

String String::Mid(uint32_t first, uint32_t count) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (first >= size)
	{
		return {};
	}

	if (first + count > size)
	{
		count = size - first;
	}

	if (first == 0 && count == size)
	{
		return *this;
	}

	return String(m_data->GetDataConst() + first, count);
}

String String::Mid(uint32_t first) const
{
	return Mid(first, Size() - first);
}

String String::Left(uint32_t count) const
{
	return Mid(0, count);
}

String String::Right(uint32_t count) const
{
	uint32_t size = Size();
	if (count >= size)
	{
		return *this;
	}
	return Mid(size - count, count);
}

String String::ToUpper() const
{
	String   r(*this);
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		r[i] = Char::ToUpper(r.At(i));
	}
	return r;
}

String String::ToLower() const
{
	String   r(*this);
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		r[i] = Char::ToLower(r.At(i));
	}
	return r;
}

String String::TrimRight() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(size - i - 1)))
		{
			return Mid(0, size - i);
		}
	}
	return {};
}

String String::TrimLeft() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(i)))
		{
			return Mid(i, size - i);
		}
	}
	return {};
}

String String::Trim() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size     = Size();
	uint32_t left_pos = size;
	uint32_t count    = 0;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(i)))
		{
			left_pos = i;
			break;
		}
	}

	for (uint32_t i = 0; i < size - left_pos; i++)
	{
		if (!Char::IsSpace(m_data->At(size - i - 1)))
		{
			count = size - left_pos - i;
			break;
		}
	}

	return Mid(left_pos, count);
}

String String::Simplify() const
{
	EXIT_IF(m_data == nullptr);

	String   r;
	uint32_t size       = Size();
	bool     prev_space = true;

	for (uint32_t i = 0; i < size; i++)
	{
		char32_t c = m_data->At(i);
		if (Char::IsSpace(c))
		{
			if (prev_space)
			{
				continue;
			}

			prev_space = true;
		} else
		{
			prev_space = false;
		}

		r += c;
	}
	return r.TrimRight();
}

String String::ReplaceChar(char32_t old_char, char32_t new_char, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	String r(*this);

	uint32_t size = Size();
	if (cs == String::Case::Sensitive)
	{
		for (uint32_t i = 0; i < size; i++)
		{
			if (m_data->At(i) == old_char)
			{
				r[i] = new_char;
			}
		}
	} else if (cs == String::Case::Insensitive)
	{
		for (uint32_t i = 0; i < size; i++)
		{
			if (Char::ToLower(m_data->At(i)) == Char::ToLower(old_char) || Char::ToUpper(m_data->At(i)) == Char::ToUpper(old_char))
			{
				r[i] = new_char;
			}
		}
	}

	return r;
}

String String::RemoveAt(uint32_t index, uint32_t count) const
{
	EXIT_IF(m_data == nullptr);

	if (index >= Size())
	{
		return *this;
	}
	String r(*this);
	copy_on_write(&r.m_data);
	r.m_data->RemoveAt(index, count);
	if (r.m_data->Size() == 0 || r.At(r.m_data->Size() - 1) != 0)
	{
		r.m_data->Add(U'\0');
	}
	return r;
}

String String::RemoveChar(char32_t ch, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	String r(*this);
	bool   cow = false;
	for (uint32_t i = 0; i < r.Size();)
	{
		if ((cs == String::Case::Sensitive && r.At(i) == ch) ||
		    (cs == String::Case::Insensitive &&
		     (Char::ToLower(r.At(i)) == Char::ToLower(ch) || Char::ToUpper(r.At(i)) == Char::ToUpper(ch))))
		{
			if (!cow)
			{
				copy_on_write(&r.m_data);
				cow = true;
			}

			r.m_data->RemoveAt(i);
		} else
		{
			i++;
		}
	}
	return r;
}

String String::InsertAt(uint32_t index, const String& str) const
{
	return Mid(0, index) + str + Mid(index, Size() - index);
}

uint32_t String::FindIndex(const String& str, uint32_t from, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (from >= size)
	{
		return STRING_INVALID_INDEX;
	}

	uint32_t str_size = str.Size();

	if (str_size == 0)
	{
		return from;
	}

	if (cs == String::Case::Sensitive)
	{
		for (uint32_t i = from; i + str_size <= size; i++)
		{
			//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
			if (compare_equal(m_data, str.m_data, i, 0, str_size))
			{
				return i;
			}
		}
	} else if (cs == String::Case::Insensitive)
	{
		for (uint32_t i = from; i + str_size <= size; i++)
		{
			if (compare_equal_no_case(m_data, str.m_data, i, 0, str_size))
			{
				return i;
			}
		}
	}

	return STRING_INVALID_INDEX;
}

uint32_t String::FindLastIndex(const String& str, uint32_t from, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (size == 0)
	{
		return STRING_INVALID_INDEX;
	}

	if (from >= size)
	{
		from = size - 1;
	}

	uint32_t str_size = str.Size();

	if (str_size == 0)
	{
		return from;
	}

	if (from + str_size > size)
	{
		from = size - str_size;
	}

	if (cs == String::Case::Sensitive)
	{
		for (uint32_t i = from; i <= from; i--)
		{
			if (compare_equal(m_data, str.m_data, i, 0, str_size))
			{
				return i;
			}
		}
	} else if (cs == String::Case::Insensitive)
	{
		for (uint32_t i = from; i <= from; i--)
		{
			if (compare_equal_no_case(m_data, str.m_data, i, 0, str_size))
			{
				return i;
			}
		}
	}

	return STRING_INVALID_INDEX;
}

bool String::IndexValid(uint32_t index) const
{
	return index < Size();
}

uint32_t String::FindIndex(char32_t chr, uint32_t from, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (from >= size)
	{
		return STRING_INVALID_INDEX;
	}

	if (cs == String::Case::Sensitive)
	{
		for (uint32_t i = from; i + 1 <= size; i++)
		{
			if (compare_equal(m_data, chr, i))
			{
				return i;
			}
		}
	} else if (cs == String::Case::Insensitive)
	{
		for (uint32_t i = from; i + 1 <= size; i++)
		{
			if (compare_equal_no_case(m_data, chr, i))
			{
				return i;
			}
		}
	}

	return STRING_INVALID_INDEX;
}

uint32_t String::FindLastIndex(char32_t chr, uint32_t from, Case cs) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (size == 0)
	{
		return STRING_INVALID_INDEX;
	}

	if (from >= size)
	{
		from = size - 1;
	}

	if (from + 1 > size)
	{
		from = size - 1;
	}

	if (cs == String::Case::Sensitive)
	{
		for (uint32_t i = from; i <= from; i--)
		{
			if (compare_equal(m_data, chr, i))
			{
				return i;
			}
		}
	} else if (cs == String::Case::Insensitive)
	{
		for (uint32_t i = from; i <= from; i--)
		{
			if (compare_equal_no_case(m_data, chr, i))
			{
				return i;
			}
		}
	}

	return STRING_INVALID_INDEX;
}

bool String::Printf(const char* format, va_list args)
{
	uint32_t len = sys_vscprintf(format, args);

	if (len == 0)
	{
		return false;
	}

	char buffer[1024];
	bool use_buffer = (len + 1 <= 1024);

	char* d = (use_buffer ? buffer : new char[len + 1]);
	memset(d, 0, len + 1);

	len = sys_vsnprintf(d, len, format, args);

	*this = d;

	if (!use_buffer)
	{
		DeleteArray(d);
	}

	return !(len == 0);
}

bool String::Printf(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	bool r = Printf(format, args);

	va_end(args);

	return r;
}

String String::FromPrintf(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	String                str;
	[[maybe_unused]] bool r = str.Printf(format, args);

	va_end(args);

	EXIT_IF(!r);

	return str;
}

bool String::ContainsStr(const String& str, Case cs) const
{
	if (str.Size() == 0)
	{
		return true;
	}
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(str, 0, cs) != STRING_INVALID_INDEX;
}

bool String::ContainsAnyStr(const StringList& list, Case cs) const
{
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (ContainsStr(list.At(i), cs))
		{
			return true;
		}
	}
	return false;
}

bool String::ContainsAllStr(const StringList& list, Case cs) const
{
	bool     ret  = true;
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		ret = ret && ContainsStr(list.At(i), cs);
	}
	return ret;
}

bool String::ContainsChar(char32_t chr, Case cs) const
{
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(chr, 0, cs) != STRING_INVALID_INDEX;
}

bool String::ContainsAnyChar(const String& list, Case cs) const
{
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (ContainsChar(list.At(i), cs))
		{
			return true;
		}
	}
	return false;
}

bool String::ContainsAllChar(const String& list, Case cs) const
{
	bool     ret  = true;
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		ret = ret && ContainsChar(list.At(i), cs);
	}
	return ret;
}

bool String::EndsWith(const String& str, Case cs) const
{
	uint32_t str_size = str.Size();
	if (str_size == 0)
	{
		return true;
	}
	uint32_t size = Size();
	if (size == 0)
	{
		return false;
	}
	return FindLastIndex(str, size - 1, cs) == (size - str_size);
}

bool String::StartsWith(const String& str, Case cs) const
{
	if (str.Size() == 0)
	{
		return true;
	}
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(str, 0, cs) == 0;
}

bool String::EndsWith(char32_t chr, Case cs) const
{
	uint32_t size = Size();
	if (size == 0)
	{
		return false;
	}
	return FindLastIndex(chr, size - 1, cs) == (size - 1);
}

bool String::StartsWith(char32_t chr, Case cs) const
{
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(chr, 0, cs) == 0;
}

StringList String::Split(const String& sep, SplitType type, Case cs) const
{
	StringList list;
	uint32_t   start    = 0;
	uint32_t   extra    = (sep.IsEmpty() ? 1 : 0);
	uint32_t   end      = 0;
	uint32_t   sep_size = sep.Size();

	while ((end = FindIndex(sep, start + extra, cs)) != STRING_INVALID_INDEX)
	{
		if (start != end || type == String::SplitType::WithEmptyParts)
		{
			list.Add(Mid(start, end - start));
		}

		start = end + sep_size;
	}

	if (start != Size() || type == String::SplitType::WithEmptyParts)
	{
		list.Add(Mid(start, Size() - start));
	}
	return list;
}

StringList String::Split(char32_t sep, SplitType type, Case cs) const
{
	StringList list;
	uint32_t   start = 0;
	uint32_t   end   = 0;

	while ((end = FindIndex(sep, start, cs)) != STRING_INVALID_INDEX)
	{
		if (start != end || type == String::SplitType::WithEmptyParts)
		{
			list.Add(Mid(start, end - start));
		}

		start = end + 1;
	}

	if (start != Size() || type == String::SplitType::WithEmptyParts)
	{
		list.Add(Mid(start, Size() - start));
	}
	return list;
}

String String::ReplaceStr(const String& old_str, const String& new_str, Case cs) const
{
	String   str;
	uint32_t start    = 0;
	uint32_t extra    = (old_str.IsEmpty() ? 1 : 0);
	uint32_t end      = 0;
	uint32_t sep_size = old_str.Size();

	while ((end = FindIndex(old_str, start + extra, cs)) != STRING_INVALID_INDEX)
	{
		if (start != end)
		{
			str += Mid(start, end - start);
		}

		str += new_str;

		start = end + sep_size;
	}

	if (start != Size())
	{
		str += Mid(start, Size() - start);
	}
	return str;
}

String String::RemoveStr(const String& str, Case cs) const
{
	String   ret_str;
	uint32_t start    = 0;
	uint32_t extra    = (str.IsEmpty() ? 1 : 0);
	uint32_t end      = 0;
	uint32_t sep_size = str.Size();

	while ((end = FindIndex(str, start + extra, cs)) != STRING_INVALID_INDEX)
	{
		if (start != end)
		{
			ret_str += Mid(start, end - start);
		}

		start = end + sep_size;
	}

	if (start != Size())
	{
		ret_str += Mid(start, Size() - start);
	}
	return ret_str;
}

uint32_t String::ToUint32(int base) const
{
	return sys_strtoui32(utf8_str().GetData(), nullptr, base);
}

uint64_t String::ToUint64(int base) const
{
	return sys_strtoui64(utf8_str().GetData(), nullptr, base);
}

int32_t String::ToInt32(int base) const
{
	return sys_strtoi32(utf8_str().GetData(), nullptr, base);
}

int64_t String::ToInt64(int base) const
{
	return sys_strtoi64(utf8_str().GetData(), nullptr, base);
}

double String::ToDouble() const
{
	return sys_strtod(utf8_str().GetData(), nullptr);
}

float String::ToFloat() const
{
	return sys_strtof(utf8_str().GetData(), nullptr);
}

bool StringList::Contains(const String& str, String::Case cs) const
{
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (At(i).ContainsStr(str, cs))
		{
			return true;
		}
	}
	return false;
}

String StringList::Concat(const String& str) const
{
	String   r;
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		r += (i > 0 ? str : String(U"")) + At(i);
	}
	return r;
}

String StringList::Concat(char32_t chr) const
{
	String   r;
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (i > 0)
		{
			r += chr;
		}
		r += At(i);
	}
	return r;
}

String String::DirectoryWithoutFilename() const
{
	uint32_t index = FindLastIndex(U"/");

	if (index == STRING_INVALID_INDEX)
	{
		return {};
	}

	return Left(index + 1);
}

String String::FilenameWithoutDirectory() const
{
	uint32_t index = FindLastIndex(U"/");

	if (index == STRING_INVALID_INDEX)
	{
		return *this;
	}

	return Mid(index + 1);
}

String String::FilenameWithoutExtension() const
{
	uint32_t index = FindLastIndex(U".");

	if (index == STRING_INVALID_INDEX)
	{
		return *this;
	}

	return Left(index);
}

String String::ExtensionWithoutFilename() const
{
	uint32_t index = FindLastIndex(U".");

	if (index == STRING_INVALID_INDEX)
	{
		return {};
	}

	return Mid(index);
}

String String::FixFilenameSlash() const
{
	auto str = ReplaceChar(U'\\', U'/');
	return str;
}

String String::FixDirectorySlash() const
{
	auto str = ReplaceChar(U'\\', U'/');
	if (!str.EndsWith(U'/'))
	{
		str += U'/';
	}
	return str;
}

String String::RemoveLast(uint32_t num) const
{
	uint32_t size = Size();

	if (num >= size)
	{
		return {};
	}

	return Left(size - num);
}

String String::RemoveFirst(uint32_t num) const
{
	uint32_t size = Size();

	if (num >= size)
	{
		return {};
	}

	return Right(size - num);
}

bool StringList::Equal(const StringList& str) const
{
	uint32_t size = Size();

	if (size != str.Size())
	{
		return false;
	}

	for (uint32_t i = 0; i < size; i++)
	{
		if (!At(i).Equal(str.At(i)))
		{
			return false;
		}
	}
	return true;
}

bool StringList::EqualNoCase(const StringList& str) const
{
	uint32_t size = Size();

	if (size != str.Size())
	{
		return false;
	}

	for (uint32_t i = 0; i < size; i++)
	{
		if (!At(i).EqualNoCase(str.At(i)))
		{
			return false;
		}
	}
	return true;
}

ByteBuffer String::HexToBin() const
{
	EXIT_IF(m_data == nullptr);

	ByteBuffer r;

	uint8_t p = 0;

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if ((i % 2) == 0)
		{
			p = Char::HexDigit(m_data->At(i)) * 16;
		} else
		{
			p += Char::HexDigit(m_data->At(i));
			r.Add(static_cast<Byte>(p));
			p = 0;
		}
	}

	return r;
}

String String::HexFromBin(const ByteBuffer& bin)
{
	String r;

	FOR (i, bin)
	{
		r += String::FromPrintf("%02" PRIX8, std::to_integer<uint8_t>(bin.At(i)));
	}

	return r;
}

String String::SafeLua() const
{
	return ReplaceStr(U"\\", U"\\\\").ReplaceStr(U"\'", U"\\'");
}

String String::SafeCsv() const
{
	bool add_space = false;
	if (StartsWith(U'+') || StartsWith(U'=') || StartsWith(U'-'))
	{
		add_space = true;
	}
	if (ContainsChar(U'\"') || ContainsChar(U';') || ContainsChar(U'+') || ContainsChar(U'=') || ContainsChar(U'-'))
	{
		return U"\"" + String(add_space ? U" " : U"") + ReplaceStr(U"\"", U"\"\"") + U"\"";
	}
	return *this;
}

bool String::IsAlpha() const
{
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsAlpha(At(i)))
		{
			return false;
		}
	}
	return true;
}

String String::SortChars() const
{
	EXIT_IF(m_data == nullptr);

	if (IsEmpty())
	{
		return {};
	}
	auto             size = m_data->Size() - 1;
	Vector<char32_t> d(size);
	std::memcpy(d.GetData(), m_data->GetDataConst(), size * sizeof(char32_t));
	d.Sort();
	return String(d.GetDataConst(), size);
}

} // namespace Kyty::Core
