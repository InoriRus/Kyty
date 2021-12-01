#ifndef INCLUDE_KYTY_MATH_MATHALL_H_
#define INCLUDE_KYTY_MATH_MATHALL_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Subsystems.h"

//#define KYTY_MATH_GLM

#ifdef KYTY_MATH_GLM
#include "Kyty/KytyGlmInc.h" // IWYU pragma: export
typedef glm::mat2 mat2;
typedef glm::mat3 mat3;
typedef glm::mat4 mat4;
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
#else
#include "Kyty/Math/VectorAndMatrix.h" // IWYU pragma: export
using mat2 = Kyty::Math::m::mat2;
using mat3 = Kyty::Math::m::mat3;
using mat4 = Kyty::Math::m::mat4;
using vec2 = Kyty::Math::m::vec2;
using vec3 = Kyty::Math::m::vec3;
using vec4 = Kyty::Math::m::vec4;
#endif

namespace Kyty::Math {

namespace Double {
constexpr double E          = 2.7182818284590452354;  /* e */
constexpr double LOG2E      = 1.4426950408889634074;  /* log 2e */
constexpr double LOG10E     = 0.43429448190325182765; /* log 10e */
constexpr double LN2        = 0.69314718055994530942; /* log e2 */
constexpr double LN10       = 2.30258509299404568402; /* log e10 */
constexpr double PI         = 3.14159265358979323846; /* pi */
constexpr double PI_2       = 1.57079632679489661923; /* pi/2 */
constexpr double PI_4       = 0.78539816339744830962; /* pi/4 */
constexpr double C_1_PI     = 0.31830988618379067154; /* 1/pi */
constexpr double C_2_PI     = 0.63661977236758134308; /* 2/pi */
constexpr double C_2_SQRTPI = 1.12837916709551257390; /* 2/sqrt(pi) */
constexpr double SQRT2      = 1.41421356237309504880; /* sqrt(2) */
constexpr double SQRT1_2    = 0.70710678118654752440; /* 1/sqrt(2) */
} // namespace Double

namespace Float {
constexpr float E          = 2.7182818284590452354f;  /* e */
constexpr float LOG2E      = 1.4426950408889634074f;  /* log 2e */
constexpr float LOG10E     = 0.43429448190325182765f; /* log 10e */
constexpr float LN2        = 0.69314718055994530942f; /* log e2 */
constexpr float LN10       = 2.30258509299404568402f; /* log e10 */
constexpr float PI         = 3.14159265358979323846f; /* pi */
constexpr float PI_2       = 1.57079632679489661923f; /* pi/2 */
constexpr float PI_4       = 0.78539816339744830962f; /* pi/4 */
constexpr float C_1_PI     = 0.31830988618379067154f; /* 1/pi */
constexpr float C_2_PI     = 0.63661977236758134308f; /* 2/pi */
constexpr float C_2_SQRTPI = 1.12837916709551257390f; /* 2/sqrt(pi) */
constexpr float SQRT2      = 1.41421356237309504880f; /* sqrt(2) */
constexpr float SQRT1_2    = 0.70710678118654752440f; /* 1/sqrt(2) */
} // namespace Float

KYTY_SUBSYSTEM_DEFINE(Math);

inline vec3& xyz(vec4& v) // NOLINT(google-runtime-references)
{
#ifdef KYTY_MATH_GLM
	return *(vec3*)glm::value_ptr(v);
#else
	return *reinterpret_cast<vec3*>(m::value_ptr(v));
#endif
}

inline const vec3& xyz(const vec4& v)
{
#ifdef KYTY_MATH_GLM
	return *(const vec3*)glm::value_ptr(v);
#else
	return *reinterpret_cast<const vec3*>(m::value_ptr(v));
#endif
}

inline vec2& xy(vec4& v) // NOLINT(google-runtime-references)
{
#ifdef KYTY_MATH_GLM
	return *(vec2*)glm::value_ptr(v);
#else
	return *reinterpret_cast<vec2*>(m::value_ptr(v));
#endif
}

inline vec2& xy(vec3& v) // NOLINT(google-runtime-references)
{
#ifdef KYTY_MATH_GLM
	return *(vec2*)glm::value_ptr(v);
#else
	return *reinterpret_cast<vec2*>(m::value_ptr(v));
#endif
}

inline uint32_t nod(uint32_t a, uint32_t b)
{
	while ((a != 0u) && (b != 0u))
	{
		a > b ? a %= b : b %= a;
	}
	return a + b;
}

struct Size
{
	Size() = default;
	Size(uint32_t w, uint32_t h): width(w), height(h) {}

	bool operator==(const Size& s) const { return width == s.width && height == s.height; }

	uint32_t width {0};
	uint32_t height {0};
};

struct Rect
{
	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;
};

inline mat4 mat_translate(const vec3& v)
{
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, v.x, v.y, v.z, 1.0f);
}

inline mat4 mat_translate(const vec2& v)
{
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, v.x, v.y, 0.0f, 1.0f);
}

inline mat4 mat_translate(float z)
{
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, z, 1.0f);
}

inline void mat_translate_upd(mat4& m, const vec3& v) // NOLINT(google-runtime-references)
{
	m[3][0] = v.x;
	m[3][1] = v.y;
	m[3][2] = v.z;
}

inline void mat_translate_upd(mat4& m, const vec2& v) // NOLINT(google-runtime-references)
{
	m[3][0] = v.x;
	m[3][1] = v.y;
}

inline void mat_translate_upd(mat4& m, float z) // NOLINT(google-runtime-references)
{
	m[3][2] = z;
}

inline mat4 mat_scale(const vec3& v)
{
	return mat4(v.x, 0.0f, 0.0f, 0.0f, 0.0f, v.y, 0.0f, 0.0f, 0.0f, 0.0f, v.z, 0.0f, 0.0f, 0.0f, 0.0, 1.0f);
}

inline mat4 mat_scale(const vec2& v)
{
	return mat4(v.x, 0.0f, 0.0f, 0.0f, 0.0f, v.y, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0, 1.0f);
}

inline void mat_scale_upd(mat4& m, const vec3& v) // NOLINT(google-runtime-references)
{
	m[0][0] = v.x;
	m[1][1] = v.y;
	m[2][2] = v.z;
}

inline void mat_scale_upd(mat4& m, const vec2& v) // NOLINT(google-runtime-references)
{
	m[0][0] = v.x;
	m[1][1] = v.y;
}

inline mat4 mat_rotate(float angle, const vec3& v)
{
#ifdef KYTY_MATH_GLM
	return glm::rotate(mat4(1.0f), angle, v);
#else
	return m::rotate(angle, v);
#endif
}

inline mat4 mat_rotate(float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);

	return mat4(c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0, 1.0f);
}

inline void mat_rotate_upd(mat4& m, float angle, const vec3& v) // NOLINT(google-runtime-references)
{
#ifdef KYTY_MATH_GLM
	m = glm::rotate(mat4(1.0f), angle, v);
#else
	m = m::rotate(angle, v);
#endif
}

inline void mat_rotate_upd(mat4& m, float angle) // NOLINT(google-runtime-references)
{
	float c = cosf(angle);
	float s = sinf(angle);

	m[0][0] = c;
	m[1][1] = c;
	m[1][0] = -s;
	m[0][1] = s;
}

inline vec2 vec_reciprocal(const vec2& v)
{
	EXIT_IF(v.x == 0.0f || v.y == 0.0f);

	return vec2(1.0f / v.x, 1.0f / v.y);
}

inline vec3 vec_reciprocal(const vec3& v)
{
	EXIT_IF(v.x == 0.0f || v.y == 0.0f || v.z == 0.0f);

	return vec3(1.0f / v.x, 1.0f / v.y, 1.0f / v.z);
}

inline vec4 vec_reciprocal(const vec4& v)
{
	EXIT_IF(v.x == 0.0f || v.y == 0.0f || v.z == 0.0f || v.w == 0.0f);

	return vec4(1.0f / v.x, 1.0f / v.y, 1.0f / v.z, 1.0f / v.w);
}

struct PosUV
{
	vec2 pos {};
	vec2 uv {};
};

struct RectUV
{
	PosUV p1 {};
	PosUV p2 {};
	float z {};
};

template <class T>
inline T floordiv(T a, T b)
{
	if (b < 0)
	{
		if (a < 0)
		{
			return a / b;
		};
		b = -b;
		a = -a;
	}

	return (a - (a < 0 ? b - 1 : 0)) / b;
}

inline mat4 mat_lookAt(const vec3& eye, const vec3& center, const vec3& up)
{
#ifdef KYTY_MATH_GLM
	return glm::lookAt(eye, center, up);
#else
	return m::lookAt(eye, center, up);
#endif
}

inline mat4 mat_perspective(float f, float a, float nr, float fr)
{
#ifdef KYTY_MATH_GLM
	return glm::perspective(f, a, nr, fr);
#else
	return m::perspective(f, a, nr, fr);
#endif
}

inline mat4 mat_ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
#ifdef KYTY_MATH_GLM
	return glm::ortho(left, right, bottom, top, z_near, z_far);
#else
	return m::ortho(left, right, bottom, top, z_near, z_far);
#endif
}

inline mat4 mat_inverse_transpose(const mat4& m)
{
#ifdef KYTY_MATH_GLM
	return glm::inverseTranspose(m);
#else
	return m::inverseTranspose(m);
#endif
}

inline mat4 mat_transpose(const mat4& m)
{
#ifdef KYTY_MATH_GLM
	return glm::transpose(m);
#else
	return m::transpose(m);
#endif
}

inline mat4 mat_inverse(const mat4& m)
{
#ifdef KYTY_MATH_GLM
	return glm::inverse(m);
#else
	return m::inverse(m);
#endif
}

inline mat3 mat_transpose(const mat3& m)
{
#ifdef KYTY_MATH_GLM
	return glm::transpose(m);
#else
	return m::transpose(m);
#endif
}

inline mat3 mat_inverse(const mat3& m)
{
#ifdef KYTY_MATH_GLM
	return glm::inverse(m);
#else
	return m::inverse(m);
#endif
}

inline mat4 make_mat4(const float* f)
{
#ifdef KYTY_MATH_GLM
	return glm::make_mat4(f);
#else
	return m::make_mat4(f);
#endif
}

inline float math_radians(float a)
{
#ifdef KYTY_MATH_GLM
	return glm::radians(a);
#else
	return m::radians(a);
#endif
}

inline float math_round(float a)
{
#ifdef KYTY_MATH_GLM
	return glm::round(a);
#else
	return m::round(a);
#endif
}

inline vec4 math_round(const vec4& a)
{
#ifdef KYTY_MATH_GLM
	return glm::round(a);
#else
	return m::round(a);
#endif
}

inline float math_max(float a, float b)
{
#ifdef KYTY_MATH_GLM
	return glm::max(a, b);
#else
	return m::max(a, b);
#endif
}

inline vec2 math_max(const vec2& a, const vec2& b)
{
#ifdef KYTY_MATH_GLM
	return glm::max(a, b);
#else
	return m::max(a, b);
#endif
}

inline vec3 math_max(const vec3& a, const vec3& b)
{
#ifdef KYTY_MATH_GLM
	return glm::max(a, b);
#else
	return m::max(a, b);
#endif
}

inline vec4 math_max(const vec4& a, const vec4& b)
{
#ifdef KYTY_MATH_GLM
	return glm::max(a, b);
#else
	return m::max(a, b);
#endif
}

inline float math_min(float a, float b)
{
#ifdef KYTY_MATH_GLM
	return glm::min(a, b);
#else
	return m::min(a, b);
#endif
}

inline vec2 math_min(const vec2& a, const vec2& b)
{
#ifdef KYTY_MATH_GLM
	return glm::min(a, b);
#else
	return m::min(a, b);
#endif
}

inline vec3 math_min(const vec3& a, const vec3& b)
{
#ifdef KYTY_MATH_GLM
	return glm::min(a, b);
#else
	return m::min(a, b);
#endif
}

inline vec4 math_min(const vec4& a, const vec4& b)
{
#ifdef KYTY_MATH_GLM
	return glm::min(a, b);
#else
	return m::min(a, b);
#endif
}

inline float math_clamp(float v, float a, float b)
{
#ifdef KYTY_MATH_GLM
	return glm::clamp(v, a, b);
#else
	return m::clamp(v, a, b);
#endif
}

inline vec3 math_clamp(const vec3& v, float a, float b)
{
#ifdef KYTY_MATH_GLM
	return glm::clamp(v, a, b);
#else
	return m::clamp(v, a, b);
#endif
}

inline float math_abs(float v)
{
#ifdef KYTY_MATH_GLM
	return glm::abs(v);
#else
	return m::abs(v);
#endif
}

inline vec2 math_abs(const vec2& v)
{
#ifdef KYTY_MATH_GLM
	return glm::abs(v);
#else
	return m::abs(v);
#endif
}

inline float math_dot(const vec2& x, const vec2& y)
{
#ifdef KYTY_MATH_GLM
	return glm::dot(x, y);
#else
	return m::dot(x, y);
#endif
}

inline int math_nod(int a, int b)
{
	while ((a != 0) && (b != 0))
	{
		a > b ? a %= b : b %= a;
	}
	return a + b;
}

inline int math_nod(int a, int b, int c)
{
	return static_cast<int>(nod(nod(a, b), c));
}

inline int math_nod(int* a, int n)
{
	int d = a[0];
	for (int i = 1; i < n; i++)
	{
		d = static_cast<int>(nod(d, a[i]));
	}
	return d;
}

} // namespace Kyty::Math

#endif /* INCLUDE_KYTY_MATH_MATHALL_H_ */
