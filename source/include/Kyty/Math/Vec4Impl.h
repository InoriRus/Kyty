#ifndef INCLUDE_KYTY_MATH_VEC4IMPL_H_
#define INCLUDE_KYTY_MATH_VEC4IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC4_DECL vec4::vec4(float s) noexcept: x(s), y(s), z(s), w(s) {}
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC4_DECL vec4::vec4(float s1, float s2, float s3, float s4) noexcept: x(s1), y(s2), z(s3), w(s4) {}
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC4_DECL vec4::vec4(const vec3& v, float s4) noexcept: x(v.x), y(v.y), z(v.z), w(s4) {}

VEC4_DECL float& vec4::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 4));

	return data[i];
}

VEC4_DECL float const& vec4::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 4));

	return data[i];
}

// VEC4_DECL vec4 &vec4::operator = (const vec4 &v)
//{
//	if (this != &v)
//	{
//		x = v.x;
//		y = v.y;
//		z = v.z;
//		w = v.w;
//	}
//	return *this;
//}
//
// VEC4_DECL vec4 &vec4::operator = (vec4 &&v) noexcept
//{
//	*this = v;
//	return *this;
//}

VEC4_DECL vec4& vec4::operator+=(float s)
{
	x += s;
	y += s;
	z += s;
	w += s;
	return *this;
}

VEC4_DECL vec4& vec4::operator+=(const vec4& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}

VEC4_DECL vec4& vec4::operator-=(float s)
{
	x -= s;
	y -= s;
	z -= s;
	w -= s;
	return *this;
}

VEC4_DECL vec4& vec4::operator-=(const vec4& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return *this;
}

VEC4_DECL vec4& vec4::operator*=(float s)
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return *this;
}

VEC4_DECL vec4& vec4::operator*=(const vec4& v)
{
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
	return *this;
}

VEC4_DECL vec4& vec4::operator/=(float s)
{
	x /= s;
	y /= s;
	z /= s;
	w /= s;
	return *this;
}

VEC4_DECL vec4& vec4::operator/=(const vec4& v)
{
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
	return *this;
}

VEC4_DECL vec4 vec4::operator-() const
{
	return vec4(-x, -y, -z, -w);
}

VEC4_DECL vec4 operator+(const vec4& v, float s)
{
	return vec4(v.x + s, v.y + s, v.z + s, v.w + s);
}

VEC4_DECL vec4 operator+(float s, const vec4& v)
{
	return vec4(v.x + s, v.y + s, v.z + s, v.w + s);
}

VEC4_DECL vec4 operator+(const vec4& v1, const vec4& v2)
{
	return vec4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

VEC4_DECL vec4 operator-(const vec4& v, float s)
{
	return vec4(v.x - s, v.y - s, v.z - s, v.w - s);
}

VEC4_DECL vec4 operator-(float s, const vec4& v)
{
	return vec4(s - v.x, s - v.y, s - v.z, s - v.w);
}

VEC4_DECL vec4 operator-(const vec4& v1, const vec4& v2)
{
	return vec4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

VEC4_DECL vec4 operator*(const vec4& v, float s)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

VEC4_DECL vec4 operator*(float s, const vec4& v)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

VEC4_DECL vec4 operator*(const vec4& v1, const vec4& v2)
{
	return vec4(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

VEC4_DECL vec4 operator/(const vec4& v, float s)
{
	return vec4(v.x / s, v.y / s, v.z / s, v.w / s);
}

VEC4_DECL vec4 operator/(float s, const vec4& v)
{
	return vec4(s / v.x, s / v.y, s / v.z, s / v.w);
}

VEC4_DECL vec4 operator/(const vec4& v1, const vec4& v2)
{
	return vec4(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}

VEC4_DECL bool operator==(const vec4& v1, const vec4& v2)
{
	return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);
}

VEC4_DECL bool operator!=(const vec4& v1, const vec4& v2)
{
	return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_VEC4IMPL_H_ */
