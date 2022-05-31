#include "Emulator/Graphics/Color.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

static void rgb_to_lab(const Color& c, ColorLab* l)
{
	double var_r = c.R();
	double var_g = c.G();
	double var_b = c.B();

	if (var_r > 0.04045)
	{
		var_r = pow(((var_r + 0.055) / 1.055), 2.4);
	} else
	{
		var_r = var_r / 12.92;
	}
	if (var_g > 0.04045)
	{
		var_g = pow(((var_g + 0.055) / 1.055), 2.4);
	} else
	{
		var_g = var_g / 12.92;
	}
	if (var_b > 0.04045)
	{
		var_b = pow(((var_b + 0.055) / 1.055), 2.4);
	} else
	{
		var_b = var_b / 12.92;
	}

	var_r = var_r * 100.0;
	var_g = var_g * 100.0;
	var_b = var_b * 100.0;

	// Observer. = 2°, Illuminant = D65
	double x = var_r * 0.4124 + var_g * 0.3576 + var_b * 0.1805;
	double y = var_r * 0.2126 + var_g * 0.7152 + var_b * 0.0722;
	double z = var_r * 0.0193 + var_g * 0.1192 + var_b * 0.9505;

	double ref_x = 95.047;
	double ref_y = 100.000;
	double ref_z = 108.883;

	double var_x = x / ref_x;
	double var_y = y / ref_y;
	double var_z = z / ref_z;

	if (var_x > 0.008856)
	{
		var_x = pow(var_x, (1.0 / 3.0));
	} else
	{
		var_x = (7.787 * var_x) + (16.0 / 116.0);
	}
	if (var_y > 0.008856)
	{
		var_y = pow(var_y, (1.0 / 3.0));
	} else
	{
		var_y = (7.787 * var_y) + (16.0 / 116.0);
	}
	if (var_z > 0.008856)
	{
		var_z = pow(var_z, (1.0 / 3.0));
	} else
	{
		var_z = (7.787 * var_z) + (16.0 / 116.0);
	}

	l->L = static_cast<float>((116.0 * var_y) - 16.0);
	l->A = static_cast<float>(500.0 * (var_x - var_y));
	l->B = static_cast<float>(200.0 * (var_y - var_z));
}

static void lab_to_rgb(const ColorLab& l, Color* c)
{
	double var_y = (l.L + 16.0) / 116.0;
	double var_x = l.A / 500.0 + var_y;
	double var_z = var_y - l.B / 200.0;

	if (pow(var_y, 3) > 0.008856)
	{
		var_y = pow(var_y, 3);
	} else
	{
		var_y = (var_y - 16.0 / 116.0) / 7.787;
	}
	if (pow(var_x, 3) > 0.008856)
	{
		var_x = pow(var_x, 3);
	} else
	{
		var_x = (var_x - 16.0 / 116.0) / 7.787;
	}
	if (pow(var_z, 3) > 0.008856)
	{
		var_z = pow(var_z, 3);
	} else
	{
		var_z = (var_z - 16.0 / 116.0) / 7.787;
	}

	double ref_x = 95.047;
	double ref_y = 100.000;
	double ref_z = 108.883;

	// Observer. = 2°, Illuminant = D65
	double x = ref_x * var_x;
	double y = ref_y * var_y;
	double z = ref_z * var_z;

	var_x = x / 100.0;
	var_y = y / 100.0;
	var_z = z / 100.0;

	double var_r = var_x * 3.2406 + var_y * -1.5372 + var_z * -0.4986;
	double var_g = var_x * -0.9689 + var_y * 1.8758 + var_z * 0.0415;
	double var_b = var_x * 0.0557 + var_y * -0.2040 + var_z * 1.0570;

	if (var_r > 0.0031308)
	{
		var_r = 1.055 * (pow(var_r, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_r = 12.92 * var_r;
	}
	if (var_g > 0.0031308)
	{
		var_g = 1.055 * (pow(var_g, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_g = 12.92 * var_g;
	}
	if (var_b > 0.0031308)
	{
		var_b = 1.055 * (pow(var_b, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_b = 12.92 * var_b;
	}

	c->R() = static_cast<float>(var_r);
	c->G() = static_cast<float>(var_g);
	c->B() = static_cast<float>(var_b);
}

static void xyz_to_rgb(const ColorXyz& l, Color* c)
{
	double x = l.X;
	double y = l.Y;
	double z = l.Z;

	double var_x = x / 100.0;
	double var_y = y / 100.0;
	double var_z = z / 100.0;

	double var_r = var_x * 3.2406 + var_y * -1.5372 + var_z * -0.4986;
	double var_g = var_x * -0.9689 + var_y * 1.8758 + var_z * 0.0415;
	double var_b = var_x * 0.0557 + var_y * -0.2040 + var_z * 1.0570;

	if (var_r > 0.0031308)
	{
		var_r = 1.055 * (pow(var_r, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_r = 12.92 * var_r;
	}
	if (var_g > 0.0031308)
	{
		var_g = 1.055 * (pow(var_g, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_g = 12.92 * var_g;
	}
	if (var_b > 0.0031308)
	{
		var_b = 1.055 * (pow(var_b, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_b = 12.92 * var_b;
	}

	c->R() = static_cast<float>(var_r);
	c->G() = static_cast<float>(var_g);
	c->B() = static_cast<float>(var_b);
}

static void xyz_to_adobe_rgb(const ColorXyz& l, Color* c)
{
	double x_a = l.X * 1.6;
	double y_a = l.Y * 1.6;
	double z_a = l.Z * 1.6;

	double x_w = 152.07;
	double y_w = 160.00;
	double z_w = 174.25;

	double x_k = 0.5282;
	double y_k = 0.5557;
	double z_k = 0.6052;

	double x = ((x_a - x_k) / (x_w - x_k)) * (x_w / y_w);
	double y = ((y_a - y_k) / (y_w - y_k));
	double z = ((z_a - z_k) / (z_w - z_k)) * (z_w / y_w);

	double r = x * 2.04159 + y * -0.56501 + z * -0.34473;
	double g = x * -0.96924 + y * 1.87597 + z * 0.04156;
	double b = x * 0.01344 + y * -0.11836 + z * 1.01517;

	r = pow(r, (1.0 / 2.19921875));
	g = pow(g, (1.0 / 2.19921875));
	b = pow(b, (1.0 / 2.19921875));

	c->R() = static_cast<float>(r);
	c->G() = static_cast<float>(g);
	c->B() = static_cast<float>(b);
}

static void adobe_rgb_to_xyz(const Color& c, ColorXyz* l)
{
	double x_w = 152.07;
	double y_w = 160.00;
	double z_w = 174.25;

	double x_k = 0.5282;
	double y_k = 0.5557;
	double z_k = 0.6052;

	double r = c.R();
	double g = c.G();
	double b = c.B();

	r = pow(r, (2.19921875));
	g = pow(g, (2.19921875));
	b = pow(b, (2.19921875));

	double x = r * 0.57667 + g * 0.18556 + b * 0.18823;
	double y = r * 0.29734 + g * 0.62736 + b * 0.07529;
	double z = r * 0.02703 + g * 0.07069 + b * 0.99134;

	double x_a = x * (x_w - x_k) * (y_w / x_w) + x_k;
	double y_a = y * (y_w - y_k) * (y_w / y_w) + y_k;
	double z_a = z * (z_w - z_k) * (y_w / z_w) + z_k;

	l->X = static_cast<float>(x_a / 1.6);
	l->Y = static_cast<float>(y_a / 1.6);
	l->Z = static_cast<float>(z_a / 1.6);
}

static void linear_to_rgb(const Color& l, Color* c)
{
	double var_r = l.R();
	double var_g = l.G();
	double var_b = l.B();

	if (var_r > 0.0031308)
	{
		var_r = 1.055 * (pow(var_r, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_r = 12.92 * var_r;
	}
	if (var_g > 0.0031308)
	{
		var_g = 1.055 * (pow(var_g, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_g = 12.92 * var_g;
	}
	if (var_b > 0.0031308)
	{
		var_b = 1.055 * (pow(var_b, (1.0 / 2.4))) - 0.055;
	} else
	{
		var_b = 12.92 * var_b;
	}

	c->R() = static_cast<float>(var_r); // if (c.r < 0.0) c.r = 0.0; if (c.r > 1.0) c.r = 1.0;
	c->G() = static_cast<float>(var_g); // if (c.g < 0.0) c.g = 0.0; if (c.g > 1.0) c.g = 1.0;
	c->B() = static_cast<float>(var_b); // if (c.b < 0.0) c.b = 0.0; if (c.b > 1.0) c.b = 1.0;
}

static void rgb_to_xyz(const Color& c, ColorXyz* l)
{
	double var_r = c.R();
	double var_g = c.G();
	double var_b = c.B();

	if (var_r > 0.04045)
	{
		var_r = pow(((var_r + 0.055) / 1.055), 2.4);
	} else
	{
		var_r = var_r / 12.92;
	}
	if (var_g > 0.04045)
	{
		var_g = pow(((var_g + 0.055) / 1.055), 2.4);
	} else
	{
		var_g = var_g / 12.92;
	}
	if (var_b > 0.04045)
	{
		var_b = pow(((var_b + 0.055) / 1.055), 2.4);
	} else
	{
		var_b = var_b / 12.92;
	}

	var_r = var_r * 100.0;
	var_g = var_g * 100.0;
	var_b = var_b * 100.0;

	// Observer. = 2°, Illuminant = D65
	double x = var_r * 0.4124 + var_g * 0.3576 + var_b * 0.1805;
	double y = var_r * 0.2126 + var_g * 0.7152 + var_b * 0.0722;
	double z = var_r * 0.0193 + var_g * 0.1192 + var_b * 0.9505;

	l->X = static_cast<float>(x);
	l->Y = static_cast<float>(y);
	l->Z = static_cast<float>(z);
}

static void rgb_to_linear(const Color& c, Color* l)
{
	double var_r = c.R();
	double var_g = c.G();
	double var_b = c.B();

	if (var_r > 0.04045)
	{
		var_r = pow(((var_r + 0.055) / 1.055), 2.4);
	} else
	{
		var_r = var_r / 12.92;
	}
	if (var_g > 0.04045)
	{
		var_g = pow(((var_g + 0.055) / 1.055), 2.4);
	} else
	{
		var_g = var_g / 12.92;
	}
	if (var_b > 0.04045)
	{
		var_b = pow(((var_b + 0.055) / 1.055), 2.4);
	} else
	{
		var_b = var_b / 12.92;
	}

	l->R() = static_cast<float>(var_r);
	l->G() = static_cast<float>(var_g);
	l->B() = static_cast<float>(var_b);
}

static float Max(float x, float y)
{
	if (x < y)
	{
		return y;
	}
	return x;
}

static float Min(float x, float y)
{
	if (x < y)
	{
		return x;
	}
	return y;
}

void rgb_to_hsv(const Color& c, ColorHsv* l)
{
	float r = c.R();
	float g = c.G();
	float b = c.B();

	float m_x = Max(r, Max(g, b));
	float mn  = Min(r, Min(g, b));

	if (m_x == mn)
	{
		l->H = 0.0f;
	} else if (m_x == r)
	{
		if (g >= b)
		{
			l->H = 60.0f * (g - b) / (m_x - mn);
		} else
		{
			l->H = 60.0f * (g - b) / (m_x - mn) + 360;
		}
	} else if (m_x == g)
	{
		l->H = 60.0f * (b - r) / (m_x - mn) + 120;
	} else
	{
		l->H = 60.0f * (r - g) / (m_x - mn) + 240;
	}

	l->S = (m_x == 0.0f) ? 0.0f : 1.0f - mn / m_x;

	l->V = m_x;
}

void hsv_to_rgb(const ColorHsv& l, Color* c)
{
	float h  = fmodf((l.H >= 0.0 ? l.H : l.H + 360.0f) / 60.0f, 6.0f);
	float cc = l.V * l.S;
	float x  = cc * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
	float r  = 0;
	float g  = 0;
	float b  = 0;
	if (0.0f <= h && h <= 1.0f)
	{
		r = cc;
		g = x;
	} else if (1.0f <= h && h <= 2.0f)
	{
		r = x;
		g = cc;
	} else if (2.0f <= h && h <= 3.0f)
	{
		g = cc;
		b = x;
	} else if (3.0f <= h && h <= 4.0f)
	{
		g = x;
		b = cc;
	} else if (4.0f <= h && h <= 5.0f)
	{
		r = x;
		b = cc;
	} else if (5.0f <= h && h <= 6.0f)
	{
		r = cc;
		b = x;
	}
	float m = l.V - cc;
	c->R()  = r + m;
	c->G()  = g + m;
	c->B()  = b + m;
}

void rgb_to_hcl(float r, float g, float b, float* h, float* c, float* l)
{
	float m_x = Max(r, Max(g, b));
	float mn  = Min(r, Min(g, b));

	float cc = m_x - mn;

	if (cc == 0)
	{
		*h = -1;
	} else if (m_x == r)
	{
		if (g >= b)
		{
			*h = ((g - b) / cc);
		} else
		{
			*h = ((g - b) / cc) + 6;
		}
	} else if (m_x == g)
	{
		*h = ((b - r) / cc) + 2;
	} else
	{
		*h = ((r - g) / cc) + 4;
	}

	if (*h > 0)
	{
		*h *= 60;
	}

	*c = cc;
	*l = static_cast<float>(0.3 * r + 0.59 * g + 0.11 * b);
}

bool Color::InGamut() const
{
	return !(R() < 0.0f || R() > 1.0f || G() < 0.0f || G() > 1.0f || B() < 0.0f || B() > 1.0f || A() < 0.0f || A() > 1.0f);
}

ColorXyz Color::ToXyz() const
{
	ColorXyz r {};
	rgb_to_xyz(*this, &r);
	return r;
}

ColorLab Color::ToLab() const
{
	ColorLab r;
	rgb_to_lab(*this, &r);
	return r;
}

Color Color::FromLab(const ColorLab& l)
{
	Color r {};
	lab_to_rgb(l, &r);
	r.A() = 1.0f;
	return r;
}

Color Color::FromXyz(const ColorXyz& l)
{
	Color r {};
	xyz_to_rgb(l, &r);
	r.A() = 1.0f;
	return r;
}

vec3 HUEToRGB(float h)
{
	h = fmodf(h, 1.0f);
	if (h < 0.0f)
	{
		h += 1.0f;
	}
	float r = fabsf(h * 6.0f - 3.0f) - 1.0f;
	float g = 2 - fabsf(h * 6.0f - 2.0f);
	float b = 2 - fabsf(h * 6.0f - 4.0f);
	return Math::math_clamp(vec3(r, g, b), 0.0f, 1.0f);
}

// All values are all in range [0..1]
Color Color::FromHsl(float h, float s, float l, float a)
{
	vec3  r_g_b = HUEToRGB(h);
	float cc    = (1.0f - fabsf(2.0f * l - 1.0f)) * s;
	vec3  rgb   = (r_g_b - 0.5f) * cc + l;
	return {rgb.r, rgb.g, rgb.b, a};
}

void Color::ToHcl(float* h, float* c, float* l) const
{
	rgb_to_hcl(Red(), Green(), Blue(), h, c, l);
}

ColorHsv Color::ToHsv() const
{
	ColorHsv r;
	rgb_to_hsv(*this, &r);
	return r;
}

Color Color::FromHsv(const ColorHsv& l)
{
	Color r {};
	hsv_to_rgb(l, &r);
	r.A() = 1.0f;
	return r;
}

void Color::Clip()
{
	if (R() < 0.0f)
	{
		R() = 0.0f;
	}
	if (R() > 1.0f)
	{
		R() = 1.0f;
	}
	if (G() < 0.0f)
	{
		G() = 0.0f;
	}
	if (G() > 1.0f)
	{
		G() = 1.0f;
	}
	if (B() < 0.0f)
	{
		B() = 0.0f;
	}
	if (B() > 1.0f)
	{
		B() = 1.0f;
	}
	if (A() < 0.0f)
	{
		A() = 0.0f;
	}
	if (A() > 1.0f)
	{
		A() = 1.0f;
	}
}

Color Color::ToLinear() const
{
	Color r {};
	rgb_to_linear(*this, &r);
	r.A() = A();
	return r;
}

Color Color::FromLinear(const Color& l)
{
	Color r {};
	linear_to_rgb(l, &r);
	r.A() = l.A();
	return r;
}

Color Color::ToAdobeRGB() const
{
	Color    r {};
	ColorXyz xyz {};
	rgb_to_xyz(*this, &xyz);
	xyz_to_adobe_rgb(xyz, &r);
	r.A() = A();
	return r;
}

Color Color::FromAdobeRGB(const Color& c)
{
	Color    r {};
	ColorXyz xyz {};
	adobe_rgb_to_xyz(c, &xyz);
	xyz_to_rgb(xyz, &r);
	r.A() = c.A();
	return r;
}

void ColorLab::Clip()
{
	if (L < COLOR_LAB_L_MIN)
	{
		L = COLOR_LAB_L_MIN;
	}
	if (L > COLOR_LAB_L_MAX)
	{
		L = COLOR_LAB_L_MAX;
	}

	if (A < COLOR_LAB_A_MIN)
	{
		A = COLOR_LAB_A_MIN;
	}
	if (A > COLOR_LAB_A_MAX)
	{
		A = COLOR_LAB_A_MAX;
	}

	if (B < COLOR_LAB_B_MIN)
	{
		B = COLOR_LAB_B_MIN;
	}
	if (B > COLOR_LAB_B_MAX)
	{
		B = COLOR_LAB_B_MAX;
	}
}

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED
