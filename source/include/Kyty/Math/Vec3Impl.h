#ifndef INCLUDE_KYTY_MATH_VEC3IMPL_H_
#define INCLUDE_KYTY_MATH_VEC3IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC3_DECL vec3::vec3(const vec4& v) noexcept: x(v.x), y(v.y), z(v.z) {}
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC3_DECL vec3::vec3(float s) noexcept: x(s), y(s), z(s) {}
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC3_DECL vec3::vec3(float s1, float s2, float s3) noexcept: x(s1), y(s2), z(s3) {}
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC3_DECL vec3::vec3(const vec2& v, float s3) noexcept: x(v.x), y(v.y), z(s3) {}

VEC3_DECL float& vec3::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 3));

	return data[i];
}

VEC3_DECL float const& vec3::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 3));

	return data[i];
}

// VEC3_DECL vec3 &vec3::operator = (const vec3 &v)
//{
//	if (this != &v)
//	{
//		x = v.x;
//		y = v.y;
//		z = v.z;
//	}
//	return *this;
//}
//
// VEC3_DECL vec3 &vec3::operator = (vec3 &&v) noexcept
//{
//	*this = v;
//	return *this;
//}

VEC3_DECL vec3& vec3::operator+=(float s)
{
	x += s;
	y += s;
	z += s;
	return *this;
}

VEC3_DECL vec3& vec3::operator+=(const vec3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

VEC3_DECL vec3& vec3::operator-=(float s)
{
	x -= s;
	y -= s;
	z -= s;
	return *this;
}

VEC3_DECL vec3& vec3::operator-=(const vec3& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

VEC3_DECL vec3& vec3::operator*=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}

VEC3_DECL vec3& vec3::operator*=(const vec3& v)
{
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

VEC3_DECL vec3& vec3::operator/=(float s)
{
	x /= s;
	y /= s;
	z /= s;
	return *this;
}

VEC3_DECL vec3& vec3::operator/=(const vec3& v)
{
	x /= v.x;
	y /= v.y;
	z /= v.z;
	return *this;
}

VEC3_DECL vec3 vec3::operator-() const
{
	return vec3(-x, -y, -z);
}

VEC3_DECL vec3 operator+(const vec3& v, float s)
{
	return vec3(v.x + s, v.y + s, v.z + s);
}

VEC3_DECL vec3 operator+(float s, const vec3& v)
{
	return vec3(v.x + s, v.y + s, v.z + s);
}

VEC3_DECL vec3 operator+(const vec3& v1, const vec3& v2)
{
	return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

VEC3_DECL vec3 operator-(const vec3& v, float s)
{
	return vec3(v.x - s, v.y - s, v.z - s);
}

VEC3_DECL vec3 operator-(float s, const vec3& v)
{
	return vec3(s - v.x, s - v.y, s - v.z);
}

VEC3_DECL vec3 operator-(const vec3& v1, const vec3& v2)
{
	return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

VEC3_DECL vec3 operator*(const vec3& v, float s)
{
	return vec3(v.x * s, v.y * s, v.z * s);
}

VEC3_DECL vec3 operator*(float s, const vec3& v)
{
	return vec3(v.x * s, v.y * s, v.z * s);
}

VEC3_DECL vec3 operator*(const vec3& v1, const vec3& v2)
{
	return vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}

VEC3_DECL vec3 operator/(const vec3& v, float s)
{
	return vec3(v.x / s, v.y / s, v.z / s);
}

VEC3_DECL vec3 operator/(float s, const vec3& v)
{
	return vec3(s / v.x, s / v.y, s / v.z);
}

VEC3_DECL vec3 operator/(const vec3& v1, const vec3& v2)
{
	return vec3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

VEC3_DECL bool operator==(const vec3& v1, const vec3& v2)
{
	return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}

VEC3_DECL bool operator!=(const vec3& v1, const vec3& v2)
{
	return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_VEC3IMPL_H_ */
