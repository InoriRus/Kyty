#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_COLOR_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_COLOR_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Math/MathAll.h"

#include "Emulator/Common.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

using rgba32_t = uint32_t;

struct ColorHsv
{
	explicit ColorHsv(float h = 0.0f, float s = 0.0f, float v = 0.0f): H(h), S(s), V(v) {}

	float H; /*NOLINT(readability-identifier-naming)*/ // [0..360]
	float S; /*NOLINT(readability-identifier-naming)*/ // [0..1]
	float V; /*NOLINT(readability-identifier-naming)*/ // [0..1]
};

constexpr float COLOR_LAB_L_MIN = (0.0f);
constexpr float COLOR_LAB_L_MAX = (100.0f);
constexpr float COLOR_LAB_A_MIN = (-128.0f);
constexpr float COLOR_LAB_A_MAX = (+127.0f);
constexpr float COLOR_LAB_B_MIN = (-128.0f);
constexpr float COLOR_LAB_B_MAX = (+127.0f);

using lab64_t = uint64_t;

constexpr lab64_t COLOR_LAB_L_BITS = 16u;
constexpr lab64_t COLOR_LAB_A_BITS = 16u;
constexpr lab64_t COLOR_LAB_B_BITS = 16u;
constexpr lab64_t COLOR_LAB_L_MASK = ((static_cast<lab64_t>(1) << (COLOR_LAB_L_BITS)) - static_cast<lab64_t>(1));
constexpr lab64_t COLOR_LAB_A_MASK = ((static_cast<lab64_t>(1) << (COLOR_LAB_A_BITS)) - static_cast<lab64_t>(1));
constexpr lab64_t COLOR_LAB_B_MASK = ((static_cast<lab64_t>(1) << (COLOR_LAB_B_BITS)) - static_cast<lab64_t>(1));

struct ColorLab
{
	ColorLab() = default;

	ColorLab(float l, float a, float b): L(l), A(a), B(b) {}

	float L = 0; /*NOLINT(readability-identifier-naming)*/ // [COLOR_LAB_L_MIN..COLOR_LAB_L_MAX]
	float A = 0; /*NOLINT(readability-identifier-naming)*/ // [COLOR_LAB_A_MIN..COLOR_LAB_A_MAX]
	float B = 0; /*NOLINT(readability-identifier-naming)*/ // [COLOR_LAB_B_MIN..COLOR_LAB_B_MAX]

	explicit ColorLab(lab64_t lab32)
	    : ColorLab((static_cast<float>((lab32 >> (0u)) & COLOR_LAB_L_MASK) / static_cast<float>(COLOR_LAB_L_MASK)) *
	                       (COLOR_LAB_L_MAX - COLOR_LAB_L_MIN) +
	                   COLOR_LAB_L_MIN,
	               (static_cast<float>((lab32 >> (COLOR_LAB_L_BITS)) & COLOR_LAB_A_MASK) / static_cast<float>(COLOR_LAB_A_MASK)) *
	                       (COLOR_LAB_A_MAX - COLOR_LAB_A_MIN) +
	                   COLOR_LAB_A_MIN,
	               (static_cast<float>((lab32 >> (COLOR_LAB_L_BITS + COLOR_LAB_A_BITS)) & COLOR_LAB_B_MASK) /
	                static_cast<float>(COLOR_LAB_B_MASK)) *
	                       (COLOR_LAB_B_MAX - COLOR_LAB_B_MIN) +
	                   COLOR_LAB_B_MIN)
	{
	}

	[[nodiscard]] lab64_t ToLab64() const
	{
		auto l = static_cast<int64_t>(
		    Math::math_round(static_cast<float>(COLOR_LAB_L_MASK) * ((L - COLOR_LAB_L_MIN) / (COLOR_LAB_L_MAX - COLOR_LAB_L_MIN))));
		if (l < 0)
		{
			l = 0;
		}
		if (l > static_cast<int64_t>(COLOR_LAB_L_MASK))
		{
			l = static_cast<int64_t>(COLOR_LAB_L_MASK);
		}
		auto a = static_cast<int64_t>(
		    Math::math_round(static_cast<float>(COLOR_LAB_A_MASK) * ((A - COLOR_LAB_A_MIN) / (COLOR_LAB_A_MAX - COLOR_LAB_A_MIN))));
		if (a < 0)
		{
			a = 0;
		}
		if (a > static_cast<int64_t>(COLOR_LAB_A_MASK))
		{
			a = static_cast<int64_t>(COLOR_LAB_A_MASK);
		}
		auto b = static_cast<int64_t>(
		    Math::math_round(static_cast<float>(COLOR_LAB_B_MASK) * ((B - COLOR_LAB_B_MIN) / (COLOR_LAB_B_MAX - COLOR_LAB_B_MIN))));
		if (b < 0)
		{
			b = 0;
		}
		if (b > static_cast<int64_t>(COLOR_LAB_B_MASK))
		{
			b = static_cast<int64_t>(COLOR_LAB_B_MASK);
		}

		return (static_cast<lab64_t>(l) << 0u) | (static_cast<lab64_t>(a) << (COLOR_LAB_L_BITS)) |
		       (static_cast<lab64_t>(b) << (COLOR_LAB_L_BITS + COLOR_LAB_A_BITS));
	}

	[[nodiscard]] float Distance(const ColorLab& other) const
	{
		float d1 = L - other.L;
		float d2 = A - other.A;
		float d3 = B - other.B;
		return sqrtf(d1 * d1 + d2 * d2 + d3 * d3);
	}

	bool operator==(const ColorLab& c) const { return (L == c.L && A == c.A && B == c.B); }

	bool operator!=(const ColorLab& c) const { return (L != c.L || A != c.A || B != c.B); }

	void Clip();
};

struct ColorXyz
{
	float X; /*NOLINT(readability-identifier-naming)*/
	float Y; /*NOLINT(readability-identifier-naming)*/
	float Z; /*NOLINT(readability-identifier-naming)*/

	[[nodiscard]] float Distance(const ColorXyz& other) const
	{
		float d1 = X - other.X;
		float d2 = Y - other.Y;
		float d3 = Z - other.Z;
		return sqrtf(d1 * d1 + d2 * d2 + d3 * d3);
	}
};

inline rgba32_t Rgb(int r, int g, int b, int a = 255)
{
	return (static_cast<uint32_t>(r) << 0u) | (static_cast<uint32_t>(g) << 8u) | (static_cast<uint32_t>(b) << 16u) |
	       (static_cast<uint32_t>(a) << 24u);
}

inline int RgbToRed(rgba32_t c)
{
	return static_cast<int>(c & 0xffU);
}

inline int RgbToGreen(rgba32_t c)
{
	return static_cast<int>((c >> 8u) & 0xffU);
}

inline int RgbToBlue(rgba32_t c)
{
	return static_cast<int>((c >> 16u) & 0xffU);
}

inline int RgbToAlpha(rgba32_t c)
{
	return static_cast<int>((c >> 24u) & 0xffU);
}

inline float RgbDistance(rgba32_t a, rgba32_t b)
{
	int dr = RgbToRed(a) - RgbToRed(b);
	int dg = RgbToGreen(a) - RgbToGreen(b);
	int db = RgbToBlue(a) - RgbToBlue(b);
	return sqrtf(static_cast<float>(dr * dr + dg * dg + db * db));
}

class Color final
{
public:
	Color()  = default;
	~Color() = default;
	Color(float r, float g, float b, float a = 1.0f): m_v(r, g, b, a) {}
	explicit Color(const vec4& av): m_v(av) {}

	KYTY_CLASS_DEFAULT_COPY(Color);

	static Color FromRgb(int r, int g, int b, int a = 255)
	{
		return {static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f,
		        static_cast<float>(a) / 255.0f};
	}

	explicit Color(rgba32_t rgba32)
	    : m_v(static_cast<float>((rgba32 >> 0u) & 0xffU) / 255.0f, static_cast<float>((rgba32 >> 8u) & 0xffU) / 255.0f,
	          static_cast<float>((rgba32 >> 16u) & 0xffU) / 255.0f, static_cast<float>((rgba32 >> 24u) & 0xffU) / 255.0f)
	{
	}

	[[nodiscard]] rgba32_t ToRgba32() const
	{
		return (static_cast<uint32_t>(Red()) << 0u) | (static_cast<uint32_t>(Green()) << 8u) | (static_cast<uint32_t>(Blue()) << 16u) |
		       (static_cast<uint32_t>(Alpha()) << 24u);
	}

	[[nodiscard]] ColorXyz ToXyz() const;
	[[nodiscard]] ColorLab ToLab() const;
	[[nodiscard]] Color    ToLinear() const;
	[[nodiscard]] Color    ToAdobeRGB() const;
	void                   ToHcl(float* h, float* c, float* l) const;
	[[nodiscard]] ColorHsv ToHsv() const;

	[[nodiscard]] Color AlphaBlendOver(const Color& bg) const
	{
		return {m_v.r * m_v.a + bg.m_v.r * (1.0f - m_v.a), m_v.g * m_v.a + bg.m_v.g * (1.0f - m_v.a),
		        m_v.b * m_v.a + bg.m_v.b * (1.0f - m_v.a), 1.0f};
	}

	static Color FromLab(const ColorLab& l);
	static Color FromXyz(const ColorXyz& l);
	static Color FromLinear(const Color& l);
	static Color FromAdobeRGB(const Color& c);
	static Color FromHsl(float h, float s, float l, float a = 1.0f);
	static Color FromHsv(const ColorHsv& l);

	static Color Black() { return FromRgb(0, 0, 0); }
	static Color White() { return FromRgb(255, 255, 255); }

	void               Clip();
	[[nodiscard]] bool InGamut() const;

	[[nodiscard]] float Distance(const Color& other) const
	{
		float d1 = m_v.x - other.m_v.x;
		float d2 = m_v.y - other.m_v.y;
		float d3 = m_v.z - other.m_v.z;
		return sqrtf(d1 * d1 + d2 * d2 + d3 * d3);
	}

	[[nodiscard]] uint8_t Red() const
	{
		int t = static_cast<int>(Math::math_round(m_v.r * 255.0f));
		if (t < 0)
		{
			t = 0;
		}
		if (t > 255)
		{
			t = 255;
		}
		return t;
	}

	[[nodiscard]] uint8_t Green() const
	{
		int t = static_cast<int>(Math::math_round(m_v.g * 255.0f));
		if (t < 0)
		{
			t = 0;
		}
		if (t > 255)
		{
			t = 255;
		}
		return t;
	}

	[[nodiscard]] uint8_t Blue() const
	{
		int t = static_cast<int>(Math::math_round(m_v.b * 255.0f));
		if (t < 0)
		{
			t = 0;
		}
		if (t > 255)
		{
			t = 255;
		}
		return t;
	}

	[[nodiscard]] uint8_t Alpha() const
	{
		int t = static_cast<int>(Math::math_round(m_v.a * 255.0f));
		if (t < 0)
		{
			t = 0;
		}
		if (t > 255)
		{
			t = 255;
		}
		return t;
	}

	[[nodiscard]] bool IsBlack() const { return (m_v.r == 0.0f && m_v.g == 0.0f && m_v.b == 0.0f); }

	[[nodiscard]] bool IsWhite() const { return (m_v.r == 1.0f && m_v.g == 1.0f && m_v.b == 1.0f); }

	[[nodiscard]] vec3 Rgb() const { return m_v.Rgb(); }
	[[nodiscard]] vec4 Rgba() const { return m_v; }

	[[nodiscard]] const float& R() const { return m_v.r; }
	[[nodiscard]] const float& G() const { return m_v.g; }
	[[nodiscard]] const float& B() const { return m_v.b; }
	[[nodiscard]] const float& A() const { return m_v.a; }
	float&                     R() { return m_v.r; }
	float&                     G() { return m_v.g; }
	float&                     B() { return m_v.b; }
	float&                     A() { return m_v.a; }

	friend bool operator==(const Color& v1, const Color& v2) { return v1.m_v == v2.m_v; }
	friend bool operator!=(const Color& v1, const Color& v2) { return v1.m_v != v2.m_v; }

private:
	vec4 m_v;
};

} // namespace Kyty::Libs::Graphics

using Color = Kyty::Libs::Graphics::Color;

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_COLOR_H_ */
