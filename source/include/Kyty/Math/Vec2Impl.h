#ifndef INCLUDE_KYTY_MATH_VEC2IMPL_H_
#define INCLUDE_KYTY_MATH_VEC2IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// VEC2_DECL vec2::vec2() {}                                   // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
// VEC2_DECL vec2::vec2(const vec2 &v): x(v.x), y(v.y) {}      // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
// VEC2_DECL vec2::vec2(vec2 &&v) noexcept: x(v.x), y(v.y) {}           // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC2_DECL vec2::vec2(const vec3& v) noexcept: x(v.x), y(v.y) {}    // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC2_DECL vec2::vec2(const vec4& v) noexcept: x(v.x), y(v.y) {}    // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC2_DECL vec2::vec2(float s) noexcept: x(s), y(s) {}              // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
VEC2_DECL vec2::vec2(float s1, float s2) noexcept: x(s1), y(s2) {} // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)

VEC2_DECL float& vec2::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 2));

	return data[i];
}

VEC2_DECL float const& vec2::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 2));

	return data[i];
}

// VEC2_DECL vec2 &vec2::operator = (const vec2 &v)
//{
//	if (this != &v)
//	{
//		x = v.x;
//		y = v.y;
//	}
//	return *this;
//}
//
// VEC2_DECL vec2 &vec2::operator = (vec2 &&v) noexcept
//{
//	*this = v;
//	return *this;
//}

VEC2_DECL vec2& vec2::operator+=(float s)
{
	x += s;
	y += s;
	return *this;
}

VEC2_DECL vec2& vec2::operator+=(const vec2& v)
{
	x += v.x;
	y += v.y;
	return *this;
}

VEC2_DECL vec2& vec2::operator-=(float s)
{
	x -= s;
	y -= s;
	return *this;
}

VEC2_DECL vec2& vec2::operator-=(const vec2& v)
{
	x -= v.x;
	y -= v.y;
	return *this;
}

VEC2_DECL vec2& vec2::operator*=(float s)
{
	x *= s;
	y *= s;
	return *this;
}

VEC2_DECL vec2& vec2::operator*=(const vec2& v)
{
	x *= v.x;
	y *= v.y;
	return *this;
}

VEC2_DECL vec2& vec2::operator/=(float s)
{
	x /= s;
	y /= s;
	return *this;
}

VEC2_DECL vec2& vec2::operator/=(const vec2& v)
{
	x /= v.x;
	y /= v.y;
	return *this;
}

VEC2_DECL vec2 vec2::operator-() const
{
	return vec2(-x, -y);
}

VEC2_DECL vec2 operator+(const vec2& v, float s)
{
	return vec2(v.x + s, v.y + s);
}

VEC2_DECL vec2 operator+(float s, const vec2& v)
{
	return vec2(v.x + s, v.y + s);
}

VEC2_DECL vec2 operator+(const vec2& v1, const vec2& v2)
{
	return vec2(v1.x + v2.x, v1.y + v2.y);
}

VEC2_DECL vec2 operator-(const vec2& v, float s)
{
	return vec2(v.x - s, v.y - s);
}

VEC2_DECL vec2 operator-(float s, const vec2& v)
{
	return vec2(s - v.x, s - v.y);
}

VEC2_DECL vec2 operator-(const vec2& v1, const vec2& v2)
{
	return vec2(v1.x - v2.x, v1.y - v2.y);
}

VEC2_DECL vec2 operator*(const vec2& v, float s)
{
	return vec2(v.x * s, v.y * s);
}

VEC2_DECL vec2 operator*(float s, const vec2& v)
{
	return vec2(v.x * s, v.y * s);
}

VEC2_DECL vec2 operator*(const vec2& v1, const vec2& v2)
{
	return vec2(v1.x * v2.x, v1.y * v2.y);
}

VEC2_DECL vec2 operator/(const vec2& v, float s)
{
	return vec2(v.x / s, v.y / s);
}

VEC2_DECL vec2 operator/(float s, const vec2& v)
{
	return vec2(s / v.x, s / v.y);
}

VEC2_DECL vec2 operator/(const vec2& v1, const vec2& v2)
{
	return vec2(v1.x / v2.x, v1.y / v2.y);
}

VEC2_DECL bool operator==(const vec2& v1, const vec2& v2)
{
	return (v1.x == v2.x) && (v1.y == v2.y);
}

VEC2_DECL bool operator!=(const vec2& v1, const vec2& v2)
{
	return (v1.x != v2.x) || (v1.y != v2.y);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_VEC2IMPL_H_ */
