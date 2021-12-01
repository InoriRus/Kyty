#ifndef INCLUDE_KYTY_MATH_VECTORANDMATRIX_H_
#define INCLUDE_KYTY_MATH_VECTORANDMATRIX_H_

// IWYU pragma: private

#include "Kyty/Core/Common.h"

#include <cmath> // IWYU pragma: export

namespace Kyty::Math::m {

class vec3;
class vec4;

// NOLINTNEXTLINE(readability-identifier-naming)
class vec2 final
{
public:
	vec2() noexcept              = default;
	~vec2()                      = default;
	vec2(const vec2& v) noexcept = default;
	vec2(vec2&& v) noexcept      = default;
	explicit vec2(const vec3& v) noexcept;
	explicit vec2(const vec4& v) noexcept;
	explicit vec2(float s) noexcept;
	explicit vec2(float s1, float s2) noexcept;

	float&       operator[](int i);
	float const& operator[](int i) const;

	vec2& operator=(const vec2& v) = default;
	vec2& operator=(vec2&& v) noexcept = default;

	vec2& operator+=(float s);
	vec2& operator+=(const vec2& v);
	vec2& operator-=(float s);
	vec2& operator-=(const vec2& v);
	vec2& operator*=(float s);
	vec2& operator*=(const vec2& v);
	vec2& operator/=(float s);
	vec2& operator/=(const vec2& v);

	vec2 operator-() const;

	friend vec2 operator+(const vec2& v, float s);
	friend vec2 operator+(float s, const vec2& v);
	friend vec2 operator+(const vec2& v1, const vec2& v2);
	friend vec2 operator-(const vec2& v, float s);
	friend vec2 operator-(float s, const vec2& v);
	friend vec2 operator-(const vec2& v1, const vec2& v2);
	friend vec2 operator*(const vec2& v, float s);
	friend vec2 operator*(float s, const vec2& v);
	friend vec2 operator*(const vec2& v1, const vec2& v2);
	friend vec2 operator/(const vec2& v, float s);
	friend vec2 operator/(float s, const vec2& v);
	friend vec2 operator/(const vec2& v1, const vec2& v2);

	friend bool operator==(const vec2& v1, const vec2& v2);
	friend bool operator!=(const vec2& v1, const vec2& v2);

	union
	{
		struct
		{
			union
			{
				float x, r;
			};
			union
			{
				float y, g;
			};
		};

		float data[2];
	};
};

// NOLINTNEXTLINE(readability-identifier-naming)
class vec3 final
{
public:
	vec3() noexcept              = default;
	vec3(const vec3& v) noexcept = default;
	vec3(vec3&& v) noexcept      = default;
	~vec3()                      = default;
	explicit vec3(const vec4& v) noexcept;
	explicit vec3(float s) noexcept;
	explicit vec3(float s1, float s2, float s3) noexcept;
	explicit vec3(const vec2& v, float s3) noexcept;

	float&       operator[](int i);
	float const& operator[](int i) const;

	[[nodiscard]] vec2 Xy() const { return vec2(x, y); }
	[[nodiscard]] vec2 Rg() const { return vec2(r, g); }

	vec3& operator=(const vec3& v) = default;
	vec3& operator=(vec3&& v) noexcept = default;

	vec3& operator+=(float s);
	vec3& operator+=(const vec3& v);
	vec3& operator-=(float s);
	vec3& operator-=(const vec3& v);
	vec3& operator*=(float s);
	vec3& operator*=(const vec3& v);
	vec3& operator/=(float s);
	vec3& operator/=(const vec3& v);

	vec3 operator-() const;

	friend vec3 operator+(const vec3& v, float s);
	friend vec3 operator+(float s, const vec3& v);
	friend vec3 operator+(const vec3& v1, const vec3& v2);
	friend vec3 operator-(const vec3& v, float s);
	friend vec3 operator-(float s, const vec3& v);
	friend vec3 operator-(const vec3& v1, const vec3& v2);
	friend vec3 operator*(const vec3& v, float s);
	friend vec3 operator*(float s, const vec3& v);
	friend vec3 operator*(const vec3& v1, const vec3& v2);
	friend vec3 operator/(const vec3& v, float s);
	friend vec3 operator/(float s, const vec3& v);
	friend vec3 operator/(const vec3& v1, const vec3& v2);

	friend bool operator==(const vec3& v1, const vec3& v2);
	friend bool operator!=(const vec3& v1, const vec3& v2);

	union
	{
		struct
		{
			union
			{
				float x, r;
			};
			union
			{
				float y, g;
			};
			union
			{
				float z, b;
			};
		};

		float data[3];
	};
};

// NOLINTNEXTLINE(readability-identifier-naming)
class vec4 final
{
public:
	vec4() noexcept              = default;
	vec4(const vec4& v) noexcept = default;
	vec4(vec4&& v) noexcept      = default;
	~vec4()                      = default;
	explicit vec4(float s) noexcept;
	explicit vec4(float s1, float s2, float s3, float s4) noexcept;
	explicit vec4(const vec3& v, float s4) noexcept;

	float&       operator[](int i);
	float const& operator[](int i) const;

	[[nodiscard]] vec2 Xy() const { return vec2(x, y); }
	[[nodiscard]] vec2 Rg() const { return vec2(r, g); }
	[[nodiscard]] vec3 Xyz() const { return vec3(x, y, z); }
	[[nodiscard]] vec3 Rgb() const { return vec3(r, g, b); }

	vec4& operator=(const vec4& v) = default;
	vec4& operator=(vec4&& v) noexcept = default;

	vec4& operator+=(float s);
	vec4& operator+=(const vec4& v);
	vec4& operator-=(float s);
	vec4& operator-=(const vec4& v);
	vec4& operator*=(float s);
	vec4& operator*=(const vec4& v);
	vec4& operator/=(float s);
	vec4& operator/=(const vec4& v);

	vec4 operator-() const;

	friend vec4 operator+(const vec4& v, float s);
	friend vec4 operator+(float s, const vec4& v);
	friend vec4 operator+(const vec4& v1, const vec4& v2);
	friend vec4 operator-(const vec4& v, float s);
	friend vec4 operator-(float s, const vec4& v);
	friend vec4 operator-(const vec4& v1, const vec4& v2);
	friend vec4 operator*(const vec4& v, float s);
	friend vec4 operator*(float s, const vec4& v);
	friend vec4 operator*(const vec4& v1, const vec4& v2);
	friend vec4 operator/(const vec4& v, float s);
	friend vec4 operator/(float s, const vec4& v);
	friend vec4 operator/(const vec4& v1, const vec4& v2);

	friend bool operator==(const vec4& v1, const vec4& v2);
	friend bool operator!=(const vec4& v1, const vec4& v2);

	union
	{
		struct
		{
			union
			{
				float x;
				float r;
			};
			union
			{
				float y, g;
			};
			union
			{
				float z, b;
			};
			union
			{
				float w, a;
			};
		};

		float data[4];
	};
};

class mat3;
class mat4;

// NOLINTNEXTLINE(readability-identifier-naming)
class mat2 final
{
public:
	mat2() noexcept              = default;
	mat2(const mat2& m) noexcept = default;
	mat2(mat2&& m) noexcept      = default;
	~mat2()                      = default;
	explicit mat2(const mat3& m) noexcept;
	explicit mat2(const mat4& m) noexcept;
	explicit mat2(float x) noexcept;
	explicit mat2(float x1, float y1, float x2, float y2) noexcept;
	explicit mat2(const vec2& v1, const vec2& v2) noexcept;

	vec2&       operator[](int i);
	const vec2& operator[](int i) const;

	mat2& operator=(const mat2& m) = default;
	mat2& operator=(mat2&& m) noexcept = default;

	mat2& operator+=(float s);
	mat2& operator+=(const mat2& m);
	mat2& operator-=(float s);
	mat2& operator-=(const mat2& m);
	mat2& operator*=(float s);
	mat2& operator*=(const mat2& m);
	mat2& operator/=(float s);

	friend mat2 operator+(const mat2& m, float s);
	friend mat2 operator+(float s, const mat2& m);
	friend mat2 operator+(const mat2& m1, const mat2& m2);
	friend mat2 operator-(const mat2& m, float s);
	friend mat2 operator-(float s, const mat2& m);
	friend mat2 operator-(const mat2& m1, const mat2& m2);
	friend mat2 operator*(const mat2& m, float s);
	friend mat2 operator*(float s, const mat2& m);
	friend mat2 operator*(const mat2& m1, const mat2& m2);
	friend vec2 operator*(const mat2& m, const vec2& v);
	friend vec2 operator*(const vec2& v, const mat2& m);
	friend mat2 operator/(const mat2& m, float s);
	friend mat2 operator/(float s, const mat2& m);

	friend bool operator==(const mat2& m1, const mat2& m2);
	friend bool operator!=(const mat2& m1, const mat2& m2);

	vec2 data[2];
};

// NOLINTNEXTLINE(readability-identifier-naming)
class mat3 final
{
public:
	mat3() noexcept              = default;
	~mat3()                      = default;
	mat3(const mat3& m) noexcept = default;
	mat3(mat3&& m) noexcept      = default;
	explicit mat3(const mat4& m) noexcept;
	explicit mat3(float x) noexcept;
	explicit mat3(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3) noexcept;
	explicit mat3(const vec3& v1, const vec3& v2, const vec3& v3) noexcept;

	vec3&       operator[](int i);
	const vec3& operator[](int i) const;

	mat3& operator=(const mat3& m) = default;
	mat3& operator=(mat3&& m) noexcept = default;

	mat3& operator+=(float s);
	mat3& operator+=(const mat3& m);
	mat3& operator-=(float s);
	mat3& operator-=(const mat3& m);
	mat3& operator*=(float s);
	mat3& operator*=(const mat3& m);
	mat3& operator/=(float s);

	friend mat3 operator+(const mat3& m, float s);
	friend mat3 operator+(float s, const mat3& m);
	friend mat3 operator+(const mat3& m1, const mat3& m2);
	friend mat3 operator-(const mat3& m, float s);
	friend mat3 operator-(float s, const mat3& m);
	friend mat3 operator-(const mat3& m1, const mat3& m2);
	friend mat3 operator*(const mat3& m, float s);
	friend mat3 operator*(float s, const mat3& m);
	friend mat3 operator*(const mat3& m1, const mat3& m2);
	friend vec3 operator*(const mat3& m, const vec3& v);
	friend vec3 operator*(const vec3& v, const mat3& m);
	friend mat3 operator/(const mat3& m, float s);
	friend mat3 operator/(float s, const mat3& m);

	friend bool operator==(const mat3& m1, const mat3& m2);
	friend bool operator!=(const mat3& m1, const mat3& m2);

	vec3 data[3];
};

// NOLINTNEXTLINE(readability-identifier-naming)
class mat4 final
{
public:
	mat4() noexcept              = default;
	~mat4()                      = default;
	mat4(const mat4& m) noexcept = default;
	mat4(mat4&& m) noexcept      = default;
	explicit mat4(float x) noexcept;
	explicit mat4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2, float x3, float y3, float z3, float w3,
	              float x4, float y4, float z4, float w4) noexcept;
	explicit mat4(const vec4& v1, const vec4& v2, const vec4& v3, const vec4& v4) noexcept;

	vec4&       operator[](int i);
	const vec4& operator[](int i) const;

	mat4& operator=(const mat4& m) = default;
	mat4& operator=(mat4&& m) noexcept = default;

	mat4& operator+=(float s);
	mat4& operator+=(const mat4& m);
	mat4& operator-=(float s);
	mat4& operator-=(const mat4& m);
	mat4& operator*=(float s);
	mat4& operator*=(const mat4& m);
	mat4& operator/=(float s);

	friend mat4 operator+(const mat4& m, float s);
	friend mat4 operator+(float s, const mat4& m);
	friend mat4 operator+(const mat4& m1, const mat4& m2);
	friend mat4 operator-(const mat4& m, float s);
	friend mat4 operator-(float s, const mat4& m);
	friend mat4 operator-(const mat4& m1, const mat4& m2);
	friend mat4 operator*(const mat4& m, float s);
	friend mat4 operator*(float s, const mat4& m);
	friend mat4 operator*(const mat4& m1, const mat4& m2);
	friend vec4 operator*(const mat4& m, const vec4& v);
	friend vec4 operator*(const vec4& v, const mat4& m);
	friend mat4 operator/(const mat4& m, float s);
	friend mat4 operator/(float s, const mat4& m);

	friend bool operator==(const mat4& m1, const mat4& m2);
	friend bool operator!=(const mat4& m1, const mat4& m2);

	vec4 data[4];
};

inline float* value_ptr(vec2& v) // NOLINT(google-runtime-references)
{
	return &v.x;
}
inline float* value_ptr(vec3& v) // NOLINT(google-runtime-references)
{
	return &v.x;
}
inline float* value_ptr(vec4& v) // NOLINT(google-runtime-references)
{
	return &v.x;
}
inline float* value_ptr(mat2& v) // NOLINT(google-runtime-references)
{
	return &v.data[0].x;
}
inline float* value_ptr(mat3& v) // NOLINT(google-runtime-references)
{
	return &v.data[0].x;
}
inline float* value_ptr(mat4& v) // NOLINT(google-runtime-references)
{
	return &v.data[0].x;
}

inline const float* value_ptr(const vec2& v)
{
	return &v.x;
}
inline const float* value_ptr(const vec3& v)
{
	return &v.x;
}
inline const float* value_ptr(const vec4& v)
{
	return &v.x;
}
inline const float* value_ptr(const mat2& v)
{
	return &v.data[0].x;
}
inline const float* value_ptr(const mat3& v)
{
	return &v.data[0].x;
}
inline const float* value_ptr(const mat4& v)
{
	return &v.data[0].x;
}

inline float round(float n)
{
	if (n < 0.0f)
	{
		return ceilf(n - 0.5f);
	}
	return floorf(n + 0.5f);
}

inline vec2 round(const vec2& v)
{
	return vec2(round(v.x), round(v.y));
}
inline vec3 round(const vec3& v)
{
	return vec3(round(v.x), round(v.y), round(v.z));
}
inline vec4 round(const vec4& v)
{
	return vec4(round(v.x), round(v.y), round(v.z), round(v.w));
}

inline float abs(float n)
{
	return fabsf(n);
}
inline vec2 abs(const vec2& v)
{
	return vec2(abs(v.x), abs(v.y));
}
inline vec3 abs(const vec3& v)
{
	return vec3(abs(v.x), abs(v.y), abs(v.z));
}
inline vec4 abs(const vec4& v)
{
	return vec4(abs(v.x), abs(v.y), abs(v.z), abs(v.w));
}

inline vec2 normalize(const vec2& v)
{
	return v / sqrtf(v.x * v.x + v.y * v.y);
}
inline vec3 normalize(const vec3& v)
{
	return v / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}
inline vec4 normalize(const vec4& v)
{
	return v / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline float dot(const vec2& v1, const vec2& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}
inline float dot(const vec3& v1, const vec3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
inline float dot(const vec4& v1, const vec4& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline vec3 cross(const vec3& x, const vec3& y)
{
	return vec3(x.y * y.z - y.y * x.z, x.z * y.x - y.z * x.x, x.x * y.y - y.x * x.y);
}

inline mat4 rotate(float a, const vec3& v)
{
	float c  = cosf(a);
	float s  = sinf(a);
	float t  = 1.0f - c;
	vec3  u  = normalize(v);
	float tx = t * u.x;
	float ty = t * u.y;
	float tz = t * u.z;
	float sx = s * u.x;
	float sy = s * u.y;
	float sz = s * u.z;

	float m1 = c + u.x * tx;
	float m2 = u.y * tx - sz;
	float m3 = u.z * tx + sy;
	float m4 = u.y * tx + sz;
	float m5 = c + u.y * ty;
	float m6 = u.y * tz - sx;
	float m7 = u.z * tx - sy;
	float m8 = u.z * ty + sx;
	float m9 = c + u.z * tz;

	return mat4(m1, m4, m7, 0, m2, m5, m8, 0, m3, m6, m9, 0, 0, 0, 0, 1);
}

inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up)
{
	vec3 f(normalize(center - eye));
	vec3 s(normalize(cross(f, up)));
	vec3 u(cross(s, f));

	mat4 r {};

	r.data[0].x = s.x;
	r.data[0].y = u.x;
	r.data[0].z = -f.x;
	r.data[0].w = 0.0f;

	r.data[1].x = s.y;
	r.data[1].y = u.y;
	r.data[1].z = -f.y;
	r.data[1].w = 0.0f;

	r.data[2].x = s.z;
	r.data[2].y = u.z;
	r.data[2].z = -f.z;
	r.data[2].w = 0.0f;

	r.data[3].x = -dot(s, eye);
	r.data[3].y = -dot(u, eye);
	r.data[3].z = dot(f, eye);
	r.data[3].w = 1.0f;

	return r;
}

inline mat4 perspective(float fovy, float aspect, float z_near, float z_far)
{
	float t = tanf(fovy / 2.0f);

	mat4 r {};

	r.data[0].x = 1.0f / (aspect * t);
	r.data[0].y = 0.0f;
	r.data[0].z = 0.0f;
	r.data[0].w = 0.0f;

	r.data[1].x = 0.0f;
	r.data[1].y = 1.0f / (t);
	r.data[1].z = 0.0f;
	r.data[1].w = 0.0f;

	r.data[2].x = 0.0f;
	r.data[2].y = 0.0f;
	r.data[2].z = -(z_far + z_near) / (z_far - z_near);
	r.data[2].w = -1.0f;

	r.data[3].x = 0.0f;
	r.data[3].y = 0.0f;
	r.data[3].z = -(2.0f * z_far * z_near) / (z_far - z_near);
	r.data[3].w = 0.0f;

	return r;
}

inline mat4 ortho(float left, float right, float bottom, float top, float z_near, float z_far)
{
	mat4 r {};

	r.data[0].x = 2.0f / (right - left);
	r.data[0].y = 0.0f;
	r.data[0].z = 0.0f;
	r.data[0].w = 0.0f;

	r.data[1].x = 0.0f;
	r.data[1].y = 2.0f / (top - bottom);
	r.data[1].z = 0.0f;
	r.data[1].w = 0.0f;

	r.data[2].x = 0.0f;
	r.data[2].y = 0.0f;
	r.data[2].z = -2.0f / (z_far - z_near);
	r.data[2].w = 0.0f;

	r.data[3].x = -(right + left) / (right - left);
	r.data[3].y = -(top + bottom) / (top - bottom);
	r.data[3].z = -(z_far + z_near) / (z_far - z_near);
	r.data[3].w = 1.0f;

	return r;
}

inline float determinant(const mat2& m)
{
	return m.data[0].x * m.data[1].y - m.data[0].y * m.data[1].x;
}

inline float determinant(const mat3& m)
{
	return m.data[0].x * determinant(mat2(m.data[1].y, m.data[1].z, m.data[2].y, m.data[2].z)) -
	       m.data[0].y * determinant(mat2(m.data[1].x, m.data[1].z, m.data[2].x, m.data[2].z)) +
	       m.data[0].z * determinant(mat2(m.data[1].x, m.data[1].y, m.data[2].x, m.data[2].y));
}

inline float determinant(const mat4& m)
{
	return m.data[0].x * determinant(mat3(m.data[1].y, m.data[1].z, m.data[1].w, m.data[2].y, m.data[2].z, m.data[2].w, m.data[3].y,
	                                      m.data[3].z, m.data[3].w)) -
	       m.data[0].y * determinant(mat3(m.data[1].x, m.data[1].z, m.data[1].w, m.data[2].x, m.data[2].z, m.data[2].w, m.data[3].x,
	                                      m.data[3].z, m.data[3].w)) +
	       m.data[0].z * determinant(mat3(m.data[1].x, m.data[1].y, m.data[1].w, m.data[2].x, m.data[2].y, m.data[2].w, m.data[3].x,
	                                      m.data[3].y, m.data[3].w)) -
	       m.data[0].w * determinant(mat3(m.data[1].x, m.data[1].y, m.data[1].z, m.data[2].x, m.data[2].y, m.data[2].z, m.data[3].x,
	                                      m.data[3].y, m.data[3].z));
}

inline mat2 inverseTranspose(const mat2& m)
{
	float x1 = m.data[1].y;
	float y1 = m.data[1].x;
	float x2 = m.data[0].y;
	float y2 = m.data[0].x;

	return mat2(x1, -y1, -x2, y2) / determinant(m);
}

inline mat3 inverseTranspose(const mat3& m)
{
	float x1 = determinant(mat2(m.data[1].y, m.data[1].z, m.data[2].y, m.data[2].z));

	float y1 = determinant(mat2(m.data[1].x, m.data[1].z, m.data[2].x, m.data[2].z));

	float z1 = determinant(mat2(m.data[1].x, m.data[1].y, m.data[2].x, m.data[2].y));

	float x2 = determinant(mat2(m.data[0].y, m.data[0].z, m.data[2].y, m.data[2].z));

	float y2 = determinant(mat2(m.data[0].x, m.data[0].z, m.data[2].x, m.data[2].z));

	float z2 = determinant(mat2(m.data[0].x, m.data[0].y, m.data[2].x, m.data[2].y));

	float x3 = determinant(mat2(m.data[0].y, m.data[0].z, m.data[1].y, m.data[1].z));

	float y3 = determinant(mat2(m.data[0].x, m.data[0].z, m.data[1].x, m.data[1].z));

	float z3 = determinant(mat2(m.data[0].x, m.data[0].y, m.data[1].x, m.data[1].y));

	return mat3(x1, -y1, z1, -x2, y2, -z2, x3, -y3, z3) / determinant(m);
}

inline mat4 inverseTranspose(const mat4& m)
{
	float x1 = determinant(
	    mat3(m.data[1].y, m.data[1].z, m.data[1].w, m.data[2].y, m.data[2].z, m.data[2].w, m.data[3].y, m.data[3].z, m.data[3].w));

	float y1 = determinant(
	    mat3(m.data[1].x, m.data[1].z, m.data[1].w, m.data[2].x, m.data[2].z, m.data[2].w, m.data[3].x, m.data[3].z, m.data[3].w));

	float z1 = determinant(
	    mat3(m.data[1].x, m.data[1].y, m.data[1].w, m.data[2].x, m.data[2].y, m.data[2].w, m.data[3].x, m.data[3].y, m.data[3].w));

	float w1 = determinant(
	    mat3(m.data[1].x, m.data[1].y, m.data[1].z, m.data[2].x, m.data[2].y, m.data[2].z, m.data[3].x, m.data[3].y, m.data[3].z));

	float x2 = determinant(
	    mat3(m.data[0].y, m.data[0].z, m.data[0].w, m.data[2].y, m.data[2].z, m.data[2].w, m.data[3].y, m.data[3].z, m.data[3].w));

	float y2 = determinant(
	    mat3(m.data[0].x, m.data[0].z, m.data[0].w, m.data[2].x, m.data[2].z, m.data[2].w, m.data[3].x, m.data[3].z, m.data[3].w));

	float z2 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].w, m.data[2].x, m.data[2].y, m.data[2].w, m.data[3].x, m.data[3].y, m.data[3].w));

	float w2 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].z, m.data[2].x, m.data[2].y, m.data[2].z, m.data[3].x, m.data[3].y, m.data[3].z));

	float x3 = determinant(
	    mat3(m.data[0].y, m.data[0].z, m.data[0].w, m.data[1].y, m.data[1].z, m.data[1].w, m.data[3].y, m.data[3].z, m.data[3].w));

	float y3 = determinant(
	    mat3(m.data[0].x, m.data[0].z, m.data[0].w, m.data[1].x, m.data[1].z, m.data[1].w, m.data[3].x, m.data[3].z, m.data[3].w));

	float z3 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].w, m.data[1].x, m.data[1].y, m.data[1].w, m.data[3].x, m.data[3].y, m.data[3].w));

	float w3 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].z, m.data[1].x, m.data[1].y, m.data[1].z, m.data[3].x, m.data[3].y, m.data[3].z));

	float x4 = determinant(
	    mat3(m.data[0].y, m.data[0].z, m.data[0].w, m.data[1].y, m.data[1].z, m.data[1].w, m.data[2].y, m.data[2].z, m.data[2].w));

	float y4 = determinant(
	    mat3(m.data[0].x, m.data[0].z, m.data[0].w, m.data[1].x, m.data[1].z, m.data[1].w, m.data[2].x, m.data[2].z, m.data[2].w));

	float z4 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].w, m.data[1].x, m.data[1].y, m.data[1].w, m.data[2].x, m.data[2].y, m.data[2].w));

	float w4 = determinant(
	    mat3(m.data[0].x, m.data[0].y, m.data[0].z, m.data[1].x, m.data[1].y, m.data[1].z, m.data[2].x, m.data[2].y, m.data[2].z));

	return mat4(x1, -y1, z1, -w1, -x2, y2, -z2, w2, x3, -y3, z3, -w3, -x4, y4, -z4, w4) / determinant(m);
}

inline mat2 transpose(const mat2& m)
{
	return mat2(m.data[0].x, m.data[1].x, m.data[0].y, m.data[1].y);
}

inline mat3 transpose(const mat3& m)
{
	return mat3(m.data[0].x, m.data[1].x, m.data[2].x, m.data[0].y, m.data[1].y, m.data[2].y, m.data[0].z, m.data[1].z, m.data[2].z);
}

inline mat4 transpose(const mat4& m)
{
	return mat4(m.data[0].x, m.data[1].x, m.data[2].x, m.data[3].x, m.data[0].y, m.data[1].y, m.data[2].y, m.data[3].y, m.data[0].z,
	            m.data[1].z, m.data[2].z, m.data[3].z, m.data[0].w, m.data[1].w, m.data[2].w, m.data[3].w);
}

inline mat2 inverse(const mat2& m)
{
	return transpose(inverseTranspose(m));
}

inline mat3 inverse(const mat3& m)
{
	return transpose(inverseTranspose(m));
}

inline mat4 inverse(const mat4& m)
{
	return transpose(inverseTranspose(m));
}

inline mat2 make_mat2(const float* m)
{
	return mat2(m[0], m[1], m[2], m[3]);
}

inline mat3 make_mat3(const float* m)
{
	return mat3(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
}

inline mat4 make_mat4(const float* m)
{
	return mat4(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
}

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

inline float max(float a, float b)
{
	return a > b ? a : b;
}
inline vec2 max(const vec2& a, const vec2& b)
{
	return vec2(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y);
}
inline vec3 max(const vec3& a, const vec3& b)
{
	return vec3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
}
inline vec4 max(const vec4& a, const vec4& b)
{
	return vec4(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, a.w > b.w ? a.w : b.w);
}
inline vec2 max(const vec2& a, float b)
{
	return max(a, vec2(b));
}
inline vec3 max(const vec3& a, float b)
{
	return max(a, vec3(b));
}
inline vec4 max(const vec4& a, float b)
{
	return max(a, vec4(b));
}

inline float min(float a, float b)
{
	return a < b ? a : b;
}
inline vec2 min(const vec2& a, const vec2& b)
{
	return vec2(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y);
}
inline vec3 min(const vec3& a, const vec3& b)
{
	return vec3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
}
inline vec4 min(const vec4& a, const vec4& b)
{
	return vec4(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, a.w < b.w ? a.w : b.w);
}
inline vec2 min(const vec2& a, float b)
{
	return min(a, vec2(b));
}
inline vec3 min(const vec3& a, float b)
{
	return min(a, vec3(b));
}
inline vec4 min(const vec4& a, float b)
{
	return min(a, vec4(b));
}

inline float clamp(float x, float min_val, float max_val)
{
	return min(max(x, min_val), max_val);
}
inline vec2 clamp(const vec2& x, float min_val, float max_val)
{
	return min(max(x, min_val), max_val);
}
inline vec3 clamp(const vec3& x, float min_val, float max_val)
{
	return min(max(x, min_val), max_val);
}
inline vec4 clamp(const vec4& x, float min_val, float max_val)
{
	return min(max(x, min_val), max_val);
}

inline float radians(float degrees)
{
	return degrees * (0.01745329251994329576923690768489f);
}

} // namespace Kyty::Math::m

#define VEC2_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/
#define VEC3_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/
#define VEC4_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/
#define MAT2_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/
#define MAT3_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/
#define MAT4_DECL inline /*NOLINT(cppcoreguidelines-macro-usage)*/

// IWYU pragma: begin_exports
#include "Kyty/Math/Mat2Impl.h"
#include "Kyty/Math/Mat3Impl.h"
#include "Kyty/Math/Mat4Impl.h"
#include "Kyty/Math/Vec2Impl.h"
#include "Kyty/Math/Vec3Impl.h"
#include "Kyty/Math/Vec4Impl.h"
// IWYU pragma: end_exports

#endif /* INCLUDE_KYTY_MATH_VECTORANDMATRIX_H_ */
