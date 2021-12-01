#ifndef INCLUDE_KYTY_MATH_MAT3IMPL_H_
#define INCLUDE_KYTY_MATH_MAT3IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// MAT3_DECL mat3::mat3() = default;
//
// MAT3_DECL mat3::mat3(const mat3 &m)
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z;
//	data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z;
//}
//
// MAT3_DECL mat3::mat3(mat3 &&m) noexcept
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z;
//	data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z;
//}

MAT3_DECL mat3::mat3(const mat4& m) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = m.data[0].x;
	data[0].y = m.data[0].y;
	data[0].z = m.data[0].z;
	data[1].x = m.data[1].x;
	data[1].y = m.data[1].y;
	data[1].z = m.data[1].z;
	data[2].x = m.data[2].x;
	data[2].y = m.data[2].y;
	data[2].z = m.data[2].z;
}

MAT3_DECL mat3::mat3(float x) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = x;
	data[0].y = 0;
	data[0].z = 0;
	data[1].x = 0;
	data[1].y = x;
	data[1].z = 0;
	data[2].x = 0;
	data[2].y = 0;
	data[2].z = x;
}

MAT3_DECL mat3::mat3(float x1, float y1, float z1, // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
                     float x2, float y2, float z2, float x3, float y3, float z3) noexcept
{
	data[0].x = x1;
	data[0].y = y1;
	data[0].z = z1;
	data[1].x = x2;
	data[1].y = y2;
	data[1].z = z2;
	data[2].x = x3;
	data[2].y = y3;
	data[2].z = z3;
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
MAT3_DECL mat3::mat3(const vec3& v1, const vec3& v2, const vec3& v3) noexcept
{
	data[0].x = v1.x;
	data[0].y = v1.y;
	data[0].z = v1.z;
	data[1].x = v2.x;
	data[1].y = v2.y;
	data[1].z = v2.z;
	data[2].x = v3.x;
	data[2].y = v3.y;
	data[2].z = v3.z;
}

MAT3_DECL vec3& mat3::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 3));

	return data[i];
}

MAT3_DECL const vec3& mat3::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 3));

	return data[i];
}

// MAT3_DECL mat3 &mat3::operator = (const mat3 &m)
//{
//	if (this != &m)
//	{
//		data[0].x = m.data[0].x; data[0].y = m.data[0].y; data[0].z = m.data[0].z;
//		data[1].x = m.data[1].x; data[1].y = m.data[1].y; data[1].z = m.data[1].z;
//		data[2].x = m.data[2].x; data[2].y = m.data[2].y; data[2].z = m.data[2].z;
//	}
//	return *this;
//}
//
// MAT3_DECL mat3 &mat3::operator = (mat3 &&m) noexcept
//{
//	*this = m;
//	return *this;
//}

MAT3_DECL mat3& mat3::operator+=(float s)
{
	data[0].x += s;
	data[0].y += s;
	data[0].z += s;
	data[1].x += s;
	data[1].y += s;
	data[1].z += s;
	data[2].x += s;
	data[2].y += s;
	data[2].z += s;
	return *this;
}

MAT3_DECL mat3& mat3::operator+=(const mat3& m)
{
	data[0].x += m.data[0].x;
	data[0].y += m.data[0].y;
	data[0].z += m.data[0].z;
	data[1].x += m.data[1].x;
	data[1].y += m.data[1].y;
	data[1].z += m.data[1].z;
	data[2].x += m.data[2].x;
	data[2].y += m.data[2].y;
	data[2].z += m.data[2].z;
	return *this;
}

MAT3_DECL mat3& mat3::operator-=(float s)
{
	data[0].x -= s;
	data[0].y -= s;
	data[0].z -= s;
	data[1].x -= s;
	data[1].y -= s;
	data[1].z -= s;
	data[2].x -= s;
	data[2].y -= s;
	data[2].z -= s;
	return *this;
}

MAT3_DECL mat3& mat3::operator-=(const mat3& m)
{
	data[0].x -= m.data[0].x;
	data[0].y -= m.data[0].y;
	data[0].z -= m.data[0].z;
	data[1].x -= m.data[1].x;
	data[1].y -= m.data[1].y;
	data[1].z -= m.data[1].z;
	data[2].x -= m.data[2].x;
	data[2].y -= m.data[2].y;
	data[2].z -= m.data[2].z;
	return *this;
}

MAT3_DECL mat3& mat3::operator*=(float s)
{
	data[0].x *= s;
	data[0].y *= s;
	data[0].z *= s;
	data[1].x *= s;
	data[1].y *= s;
	data[1].z *= s;
	data[2].x *= s;
	data[2].y *= s;
	data[2].z *= s;
	return *this;
}

MAT3_DECL mat3& mat3::operator*=(const mat3& m)
{
	*this = *this * m;
	return *this;
}

MAT3_DECL mat3& mat3::operator/=(float s)
{
	data[0].x /= s;
	data[0].y /= s;
	data[0].z /= s;
	data[1].x /= s;
	data[1].y /= s;
	data[1].z /= s;
	data[2].x /= s;
	data[2].y /= s;
	data[2].z /= s;
	return *this;
}

MAT3_DECL mat3 operator+(const mat3& m, float s)
{
	return mat3(m.data[0].x + s, m.data[0].y + s, m.data[0].z + s, m.data[1].x + s, m.data[1].y + s, m.data[1].z + s, m.data[2].x + s,
	            m.data[2].y + s, m.data[2].z + s);
}

MAT3_DECL mat3 operator+(float s, const mat3& m)
{
	return mat3(m.data[0].x + s, m.data[0].y + s, m.data[0].z + s, m.data[1].x + s, m.data[1].y + s, m.data[1].z + s, m.data[2].x + s,
	            m.data[2].y + s, m.data[2].z + s);
}

MAT3_DECL mat3 operator+(const mat3& m1, const mat3& m2)
{
	return mat3(m1.data[0].x + m2.data[0].x, m1.data[0].y + m2.data[0].y, m1.data[0].z + m2.data[0].z, m1.data[1].x + m2.data[1].x,
	            m1.data[1].y + m2.data[1].y, m1.data[1].z + m2.data[1].z, m1.data[2].x + m2.data[2].x, m1.data[2].y + m2.data[2].y,
	            m1.data[2].z + m2.data[2].z);
}

MAT3_DECL mat3 operator-(const mat3& m, float s)
{
	return mat3(m.data[0].x - s, m.data[0].y - s, m.data[0].z - s, m.data[1].x - s, m.data[1].y - s, m.data[1].z - s, m.data[2].x - s,
	            m.data[2].y - s, m.data[2].z - s);
}

MAT3_DECL mat3 operator-(float s, const mat3& m)
{
	return mat3(s - m.data[0].x, s - m.data[0].y, s - m.data[0].z, s - m.data[1].x, s - m.data[1].y, s - m.data[1].z, s - m.data[2].x,
	            s - m.data[2].y, s - m.data[2].z);
}

MAT3_DECL mat3 operator-(const mat3& m1, const mat3& m2)
{
	return mat3(m1.data[0].x - m2.data[0].x, m1.data[0].y - m2.data[0].y, m1.data[0].z - m2.data[0].z, m1.data[1].x - m2.data[1].x,
	            m1.data[1].y - m2.data[1].y, m1.data[1].z - m2.data[1].z, m1.data[2].x - m2.data[2].x, m1.data[2].y - m2.data[2].y,
	            m1.data[2].z - m2.data[2].z);
}

MAT3_DECL mat3 operator*(const mat3& m, float s)
{
	return mat3(m.data[0].x * s, m.data[0].y * s, m.data[0].z * s, m.data[1].x * s, m.data[1].y * s, m.data[1].z * s, m.data[2].x * s,
	            m.data[2].y * s, m.data[2].z * s);
}

MAT3_DECL mat3 operator*(float s, const mat3& m)
{
	return mat3(m.data[0].x * s, m.data[0].y * s, m.data[0].z * s, m.data[1].x * s, m.data[1].y * s, m.data[1].z * s, m.data[2].x * s,
	            m.data[2].y * s, m.data[2].z * s);
}

MAT3_DECL mat3 operator*(const mat3& m1, const mat3& m2)
{
	/*#define MUL3(i, j) */ auto mul3 = [&](auto i, auto j) {
		return (m1.data[0].data[(j)] * m2.data[(i)].data[0] + m1.data[1].data[(j)] * m2.data[(i)].data[1] +
		        m1.data[2].data[(j)] * m2.data[(i)].data[2]);
	};

	return mat3(mul3(0, 0), mul3(0, 1), mul3(0, 2), mul3(1, 0), mul3(1, 1), mul3(1, 2), mul3(2, 0), mul3(2, 1), mul3(2, 2));

	//#undef MUL3
}

MAT3_DECL vec3 operator*(const mat3& m, const vec3& v)
{
	return vec3(m.data[0].x * v.x + m.data[1].x * v.y + m.data[2].x * v.z, m.data[0].y * v.x + m.data[1].y * v.y + m.data[2].y * v.z,
	            m.data[0].z * v.x + m.data[1].z * v.y + m.data[2].z * v.z);
}

MAT3_DECL vec3 operator*(const vec3& v, const mat3& m)
{
	return vec3(m.data[0].x * v.x + m.data[0].y * v.y + m.data[0].z * v.z, m.data[1].x * v.x + m.data[1].y * v.y + m.data[1].z * v.z,
	            m.data[2].x * v.x + m.data[2].y * v.y + m.data[2].z * v.z);
}

MAT3_DECL mat3 operator/(const mat3& m, float s)
{
	return mat3(m.data[0].x / s, m.data[0].y / s, m.data[0].z / s, m.data[1].x / s, m.data[1].y / s, m.data[1].z / s, m.data[2].x / s,
	            m.data[2].y / s, m.data[2].z / s);
}

MAT3_DECL mat3 operator/(float s, const mat3& m)
{
	return mat3(s / m.data[0].x, s / m.data[0].y, s / m.data[0].z, s / m.data[1].x, s / m.data[1].y, s / m.data[1].z, s / m.data[2].x,
	            s / m.data[2].y, s / m.data[2].z);
}

MAT3_DECL bool operator==(const mat3& m1, const mat3& m2)
{
	return (m1.data[0].x == m2.data[0].x) && (m1.data[0].y == m2.data[0].y) && (m1.data[0].z == m2.data[0].z) &&
	       (m1.data[1].x == m2.data[1].x) && (m1.data[1].y == m2.data[1].y) && (m1.data[1].z == m2.data[1].z) &&
	       (m1.data[2].x == m2.data[2].x) && (m1.data[2].y == m2.data[2].y) && (m1.data[2].z == m2.data[2].z);
}

MAT3_DECL bool operator!=(const mat3& m1, const mat3& m2)
{
	return (m1.data[0].x != m2.data[0].x) || (m1.data[0].y != m2.data[0].y) || (m1.data[0].z != m2.data[0].z) ||
	       (m1.data[1].x != m2.data[1].x) || (m1.data[1].y != m2.data[1].y) || (m1.data[1].z != m2.data[1].z) ||
	       (m1.data[2].x != m2.data[2].x) || (m1.data[2].y != m2.data[2].y) || (m1.data[2].z != m2.data[2].z);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_MAT3IMPL_H_ */
