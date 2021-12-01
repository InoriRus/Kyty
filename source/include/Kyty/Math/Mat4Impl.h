#ifndef INCLUDE_KYTY_MATH_MAT4IMPL_H_
#define INCLUDE_KYTY_MATH_MAT4IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// MAT4_DECL mat4::mat4()  = default;
//
// MAT4_DECL mat4::mat4(const mat4 &m)
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z; data[0].w = m.data[0].w;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z; data[1].w = m.data[1].w;
//	data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z; data[2].w = m.data[2].w;
//	data[3].x = m.data[3].x; data[3].y = m.data[3].y; data[3].z = m.data[3].z; data[3].w = m.data[3].w;
//}
//
// MAT4_DECL mat4::mat4(mat4 &&m) noexcept
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z; data[0].w = m.data[0].w;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z; data[1].w = m.data[1].w;
//	data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z; data[2].w = m.data[2].w;
//	data[3].x = m.data[3].x; data[3].y = m.data[3].y; data[3].z = m.data[3].z; data[3].w = m.data[3].w;
//}

MAT4_DECL mat4::mat4(float x) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = x;
	data[0].y = 0;
	data[0].z = 0;
	data[0].w = 0;
	data[1].x = 0;
	data[1].y = x;
	data[1].z = 0;
	data[1].w = 0;
	data[2].x = 0;
	data[2].y = 0;
	data[2].z = x;
	data[2].w = 0;
	data[3].x = 0;
	data[3].y = 0;
	data[3].z = 0;
	data[3].w = x;
}

MAT4_DECL mat4::mat4(float x1, float y1, float z1, float w1, // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
                     float x2, float y2, float z2, float w2, float x3, float y3, float z3, float w3, float x4, float y4, float z4,
                     float w4) noexcept
{
	data[0].x = x1;
	data[0].y = y1;
	data[0].z = z1;
	data[0].w = w1;
	data[1].x = x2;
	data[1].y = y2;
	data[1].z = z2;
	data[1].w = w2;
	data[2].x = x3;
	data[2].y = y3;
	data[2].z = z3;
	data[2].w = w3;
	data[3].x = x4;
	data[3].y = y4;
	data[3].z = z4;
	data[3].w = w4;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
MAT4_DECL mat4::mat4(const vec4& v1, const vec4& v2, const vec4& v3, const vec4& v4) noexcept
{
	data[0].x = v1.x;
	data[0].y = v1.y;
	data[0].z = v1.z;
	data[0].w = v1.w;
	data[1].x = v2.x;
	data[1].y = v2.y;
	data[1].z = v2.z;
	data[1].w = v2.w;
	data[2].x = v3.x;
	data[2].y = v3.y;
	data[2].z = v3.z;
	data[2].w = v3.w;
	data[3].x = v4.x;
	data[3].y = v4.y;
	data[3].z = v4.z;
	data[3].w = v4.w;
}

MAT4_DECL vec4& mat4::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 4));

	return data[i];
}

MAT4_DECL const vec4& mat4::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 4));

	return data[i];
}

// MAT4_DECL mat4 &mat4::operator = (const mat4 &m)
//{
//	if (this != &m)
//	{
//		data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z; data[0].w = m.data[0].w;
//		data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z; data[1].w = m.data[1].w;
//		data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z; data[2].w = m.data[2].w;
//		data[3].x = m.data[3].x; data[3].y = m.data[3].y; data[3].z = m.data[3].z; data[3].w = m.data[3].w;
//	}
//	return *this;
//}
//
// MAT4_DECL mat4 &mat4::operator = (mat4 &&m) noexcept
//{
//	*this = m;
//	return *this;
//}

MAT4_DECL mat4& mat4::operator+=(float s)
{
	data[0].x += s;
	data[0].y += s;
	data[0].z += s;
	data[0].w += s;
	data[1].x += s;
	data[1].y += s;
	data[1].z += s;
	data[1].w += s;
	data[2].x += s;
	data[2].y += s;
	data[2].z += s;
	data[2].w += s;
	data[3].x += s;
	data[3].y += s;
	data[3].z += s;
	data[3].w += s;
	return *this;
}

MAT4_DECL mat4& mat4::operator+=(const mat4& m)
{
	data[0].x += m.data[0].x;
	data[0].y += m.data[0].y;
	data[0].z += m.data[0].z;
	data[0].w += m.data[0].w;
	data[1].x += m.data[1].x;
	data[1].y += m.data[1].y;
	data[1].z += m.data[1].z;
	data[1].w += m.data[1].w;
	data[2].x += m.data[2].x;
	data[2].y += m.data[2].y;
	data[2].z += m.data[2].z;
	data[2].w += m.data[2].w;
	data[3].x += m.data[3].x;
	data[3].y += m.data[3].y;
	data[3].z += m.data[3].z;
	data[3].w += m.data[3].w;
	return *this;
}

MAT4_DECL mat4& mat4::operator-=(float s)
{
	data[0].x -= s;
	data[0].y -= s;
	data[0].z -= s;
	data[0].w -= s;
	data[1].x -= s;
	data[1].y -= s;
	data[1].z -= s;
	data[1].w -= s;
	data[2].x -= s;
	data[2].y -= s;
	data[2].z -= s;
	data[2].w -= s;
	data[3].x -= s;
	data[3].y -= s;
	data[3].z -= s;
	data[3].w -= s;
	return *this;
}

MAT4_DECL mat4& mat4::operator-=(const mat4& m)
{
	data[0].x -= m.data[0].x;
	data[0].y -= m.data[0].y;
	data[0].z -= m.data[0].z;
	data[0].w -= m.data[0].w;
	data[1].x -= m.data[1].x;
	data[1].y -= m.data[1].y;
	data[1].z -= m.data[1].z;
	data[1].w -= m.data[1].w;
	data[2].x -= m.data[2].x;
	data[2].y -= m.data[2].y;
	data[2].z -= m.data[2].z;
	data[2].w -= m.data[2].w;
	data[3].x -= m.data[3].x;
	data[3].y -= m.data[3].y;
	data[3].z -= m.data[3].z;
	data[3].w -= m.data[3].w;
	return *this;
}

MAT4_DECL mat4& mat4::operator*=(float s)
{
	data[0].x *= s;
	data[0].y *= s;
	data[0].z *= s;
	data[0].w *= s;
	data[1].x *= s;
	data[1].y *= s;
	data[1].z *= s;
	data[1].w *= s;
	data[2].x *= s;
	data[2].y *= s;
	data[2].z *= s;
	data[2].w *= s;
	data[3].x *= s;
	data[3].y *= s;
	data[3].z *= s;
	data[3].w *= s;
	return *this;
}

MAT4_DECL mat4& mat4::operator*=(const mat4& m)
{
	*this = *this * m;
	return *this;
}

MAT4_DECL mat4& mat4::operator/=(float s)
{
	data[0].x /= s;
	data[0].y /= s;
	data[0].z /= s;
	data[0].w /= s;
	data[1].x /= s;
	data[1].y /= s;
	data[1].z /= s;
	data[1].w /= s;
	data[2].x /= s;
	data[2].y /= s;
	data[2].z /= s;
	data[2].w /= s;
	data[3].x /= s;
	data[3].y /= s;
	data[3].z /= s;
	data[3].w /= s;
	return *this;
}

MAT4_DECL mat4 operator+(const mat4& m, float s)
{
	return mat4(m.data[0].x + s, m.data[0].y + s, m.data[0].z + s, m.data[0].w + s, m.data[1].x + s, m.data[1].y + s, m.data[1].z + s,
	            m.data[1].w + s, m.data[2].x + s, m.data[2].y + s, m.data[2].z + s, m.data[2].w + s, m.data[3].x + s, m.data[3].y + s,
	            m.data[3].z + s, m.data[3].w + s);
}

MAT4_DECL mat4 operator+(float s, const mat4& m)
{
	return mat4(m.data[0].x + s, m.data[0].y + s, m.data[0].z + s, m.data[0].w + s, m.data[1].x + s, m.data[1].y + s, m.data[1].z + s,
	            m.data[1].w + s, m.data[2].x + s, m.data[2].y + s, m.data[2].z + s, m.data[2].w + s, m.data[3].x + s, m.data[3].y + s,
	            m.data[3].z + s, m.data[3].w + s);
}

MAT4_DECL mat4 operator+(const mat4& m1, const mat4& m2)
{
	return mat4(m1.data[0].x + m2.data[0].x, m1.data[0].y + m2.data[0].y, m1.data[0].z + m2.data[0].z, m1.data[0].w + m2.data[0].w,
	            m1.data[1].x + m2.data[1].x, m1.data[1].y + m2.data[1].y, m1.data[1].z + m2.data[1].z, m1.data[1].w + m2.data[1].w,
	            m1.data[2].x + m2.data[2].x, m1.data[2].y + m2.data[2].y, m1.data[2].z + m2.data[2].z, m1.data[2].w + m2.data[2].w,
	            m1.data[3].x + m2.data[3].x, m1.data[3].y + m2.data[3].y, m1.data[3].z + m2.data[3].z, m1.data[3].w + m2.data[3].w);
}

MAT4_DECL mat4 operator-(const mat4& m, float s)
{
	return mat4(m.data[0].x - s, m.data[0].y - s, m.data[0].z - s, m.data[0].w - s, m.data[1].x - s, m.data[1].y - s, m.data[1].z - s,
	            m.data[1].w - s, m.data[2].x - s, m.data[2].y - s, m.data[2].z - s, m.data[2].w - s, m.data[3].x - s, m.data[3].y - s,
	            m.data[3].z - s, m.data[3].w - s);
}

MAT4_DECL mat4 operator-(float s, const mat4& m)
{
	return mat4(s - m.data[0].x, s - m.data[0].y, s - m.data[0].z, s - m.data[0].w, s - m.data[1].x, s - m.data[1].y, s - m.data[1].z,
	            s - m.data[1].w, s - m.data[2].x, s - m.data[2].y, s - m.data[2].z, s - m.data[2].w, s - m.data[3].x, s - m.data[3].y,
	            s - m.data[3].z, s - m.data[3].w);
}

MAT4_DECL mat4 operator-(const mat4& m1, const mat4& m2)
{
	return mat4(m1.data[0].x - m2.data[0].x, m1.data[0].y - m2.data[0].y, m1.data[0].z - m2.data[0].z, m1.data[0].w - m2.data[0].w,
	            m1.data[1].x - m2.data[1].x, m1.data[1].y - m2.data[1].y, m1.data[1].z - m2.data[1].z, m1.data[1].w - m2.data[1].w,
	            m1.data[2].x - m2.data[2].x, m1.data[2].y - m2.data[2].y, m1.data[2].z - m2.data[2].z, m1.data[2].w - m2.data[2].w,
	            m1.data[3].x - m2.data[3].x, m1.data[3].y - m2.data[3].y, m1.data[3].z - m2.data[3].z, m1.data[3].w - m2.data[3].w);
}

MAT4_DECL mat4 operator*(const mat4& m, float s)
{
	return mat4(m.data[0].x * s, m.data[0].y * s, m.data[0].z * s, m.data[0].w * s, m.data[1].x * s, m.data[1].y * s, m.data[1].z * s,
	            m.data[1].w * s, m.data[2].x * s, m.data[2].y * s, m.data[2].z * s, m.data[2].w * s, m.data[3].x * s, m.data[3].y * s,
	            m.data[3].z * s, m.data[3].w * s);
}

MAT4_DECL mat4 operator*(float s, const mat4& m)
{
	return mat4(m.data[0].x * s, m.data[0].y * s, m.data[0].z * s, m.data[0].w * s, m.data[1].x * s, m.data[1].y * s, m.data[1].z * s,
	            m.data[1].w * s, m.data[2].x * s, m.data[2].y * s, m.data[2].z * s, m.data[2].w * s, m.data[3].x * s, m.data[3].y * s,
	            m.data[3].z * s, m.data[3].w * s);
}

MAT4_DECL mat4 operator*(const mat4& m1, const mat4& m2)
{
	/*#define MUL4(i, j)*/ auto mul4 = [&](auto i, auto j) {
		return (m1.data[0].data[(j)] * m2.data[(i)].data[0] + m1.data[1].data[(j)] * m2.data[(i)].data[1] +
		        m1.data[2].data[(j)] * m2.data[(i)].data[2] + m1.data[3].data[(j)] * m2.data[(i)].data[3]);
	};

	return mat4(mul4(0, 0), mul4(0, 1), mul4(0, 2), mul4(0, 3), mul4(1, 0), mul4(1, 1), mul4(1, 2), mul4(1, 3), mul4(2, 0), mul4(2, 1),
	            mul4(2, 2), mul4(2, 3), mul4(3, 0), mul4(3, 1), mul4(3, 2), mul4(3, 3));

	//#undef MUL4
}

MAT4_DECL vec4 operator*(const mat4& m, const vec4& v)
{
	float x1 = m.data[0].x * v.x + m.data[1].x * v.y;
	float x2 = m.data[2].x * v.z + m.data[3].x * v.w;
	float y1 = m.data[0].y * v.x + m.data[1].y * v.y;
	float y2 = m.data[2].y * v.z + m.data[3].y * v.w;
	float z1 = m.data[0].z * v.x + m.data[1].z * v.y;
	float z2 = m.data[2].z * v.z + m.data[3].z * v.w;
	float w1 = m.data[0].w * v.x + m.data[1].w * v.y;
	float w2 = m.data[2].w * v.z + m.data[3].w * v.w;

	return vec4(x1 + x2, y1 + y2, z1 + z2, w1 + w2);

	//	return vec4(m.data[0].x * v.x + m.data[1].x * v.y + m.data[2].x * v.z + m.data[3].x * v.w,
	//				m.data[0].y * v.x + m.data[1].y * v.y + m.data[2].y * v.z + m.data[3].y * v.w,
	//				m.data[0].z * v.x + m.data[1].z * v.y + m.data[2].z * v.z + m.data[3].z * v.w,
	//				m.data[0].w * v.x + m.data[1].w * v.y + m.data[2].w * v.z + m.data[3].w * v.w);
}

MAT4_DECL vec4 operator*(const vec4& v, const mat4& m)
{
	return vec4(m.data[0].x * v.x + m.data[0].y * v.y + m.data[0].z * v.z + m.data[0].w * v.w,
	            m.data[1].x * v.x + m.data[1].y * v.y + m.data[1].z * v.z + m.data[1].w * v.w,
	            m.data[2].x * v.x + m.data[2].y * v.y + m.data[2].z * v.z + m.data[2].w * v.w,
	            m.data[3].x * v.x + m.data[3].y * v.y + m.data[3].z * v.z + m.data[3].w * v.w);
}

MAT4_DECL mat4 operator/(const mat4& m, float s)
{
	return mat4(m.data[0].x / s, m.data[0].y / s, m.data[0].z / s, m.data[0].w / s, m.data[1].x / s, m.data[1].y / s, m.data[1].z / s,
	            m.data[1].w / s, m.data[2].x / s, m.data[2].y / s, m.data[2].z / s, m.data[2].w / s, m.data[3].x / s, m.data[3].y / s,
	            m.data[3].z / s, m.data[3].w / s);
}

MAT4_DECL mat4 operator/(float s, const mat4& m)
{
	return mat4(s / m.data[0].x, s / m.data[0].y, s / m.data[0].z, s / m.data[0].w, s / m.data[1].x, s / m.data[1].y, s / m.data[1].z,
	            s / m.data[1].w, s / m.data[2].x, s / m.data[2].y, s / m.data[2].z, s / m.data[2].w, s / m.data[3].x, s / m.data[3].y,
	            s / m.data[3].z, s / m.data[3].w);
}

MAT4_DECL bool operator==(const mat4& m1, const mat4& m2)
{
	return (m1.data[0].x == m2.data[0].x) && (m1.data[0].y == m2.data[0].y) && (m1.data[0].z == m2.data[0].z) &&
	       (m1.data[0].w == m2.data[0].w) && (m1.data[1].x == m2.data[1].x) && (m1.data[1].y == m2.data[1].y) &&
	       (m1.data[1].z == m2.data[1].z) && (m1.data[1].w == m2.data[1].w) && (m1.data[2].x == m2.data[2].x) &&
	       (m1.data[2].y == m2.data[2].y) && (m1.data[2].z == m2.data[2].z) && (m1.data[2].w == m2.data[2].w) &&
	       (m1.data[3].x == m2.data[3].x) && (m1.data[3].y == m2.data[3].y) && (m1.data[3].z == m2.data[3].z) &&
	       (m1.data[3].w == m2.data[3].w);
}

MAT4_DECL bool operator!=(const mat4& m1, const mat4& m2)
{
	return (m1.data[0].x != m2.data[0].x) || (m1.data[0].y != m2.data[0].y) || (m1.data[0].z != m2.data[0].z) ||
	       (m1.data[0].w != m2.data[0].w) || (m1.data[1].x != m2.data[1].x) || (m1.data[1].y != m2.data[1].y) ||
	       (m1.data[1].z != m2.data[1].z) || (m1.data[1].w != m2.data[1].w) || (m1.data[2].x != m2.data[2].x) ||
	       (m1.data[2].y != m2.data[2].y) || (m1.data[2].z != m2.data[2].z) || (m1.data[2].w != m2.data[2].w) ||
	       (m1.data[3].x != m2.data[3].x) || (m1.data[3].y != m2.data[3].y) || (m1.data[3].z != m2.data[3].z) ||
	       (m1.data[3].w != m2.data[3].w);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_MAT4IMPL_H_ */
