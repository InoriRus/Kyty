#ifndef INCLUDE_KYTY_MATH_MAT2IMPL_H_
#define INCLUDE_KYTY_MATH_MAT2IMPL_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Math::m {

// MAT2_DECL mat2::mat2() = default;
//
// MAT2_DECL mat2::mat2(const mat2 &m)
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y;
//}
//
// MAT2_DECL mat2::mat2(mat2 &&m) noexcept
//{
//	data[0].x = m.data[0].x; data[0].y = m.data[0].y;
//	data[1].x = m.data[1].x; data[1].y = m.data[1].y;
//}

MAT2_DECL mat2::mat2(const mat3& m) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = m.data[0].x;
	data[0].y = m.data[0].y;
	data[1].x = m.data[1].x;
	data[1].y = m.data[1].y;
}

MAT2_DECL mat2::mat2(const mat4& m) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = m.data[0].x;
	data[0].y = m.data[0].y;
	data[1].x = m.data[1].x;
	data[1].y = m.data[1].y;
}

MAT2_DECL mat2::mat2(float x) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = x;
	data[0].y = 0;
	data[1].x = 0;
	data[1].y = x;
}

MAT2_DECL mat2::mat2(float x1, float y1, // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
                     float x2, float y2) noexcept
{
	data[0].x = x1;
	data[0].y = y1;
	data[1].x = x2;
	data[1].y = y2;
}

MAT2_DECL mat2::mat2(const vec2& v1, const vec2& v2) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
{
	data[0].x = v1.x;
	data[0].y = v1.y;
	data[1].x = v2.x;
	data[1].y = v2.y;
}

MAT2_DECL vec2& mat2::operator[](int i)
{
	EXIT_IF(!(i >= 0 && i < 2));

	return data[i];
}

MAT2_DECL const vec2& mat2::operator[](int i) const
{
	EXIT_IF(!(i >= 0 && i < 2));

	return data[i];
}

// MAT2_DECL mat2 &mat2::operator = (const mat2 &m)
//{
//	if (this != &m)
//	{
//		data[0].x = m.data[0].x; data[0].y = m.data[0].y;
//		data[1].x = m.data[1].x; data[1].y = m.data[1].y;
//	}
//	return *this;
//}
//
// MAT2_DECL mat2 &mat2::operator = (mat2 &&m) noexcept
//{
//	*this = m;
//	return *this;
//}

MAT2_DECL mat2& mat2::operator+=(float s)
{
	data[0].x += s;
	data[0].y += s;
	data[1].x += s;
	data[1].y += s;
	return *this;
}

MAT2_DECL mat2& mat2::operator+=(const mat2& m)
{
	data[0].x += m.data[0].x;
	data[0].y += m.data[0].y;
	data[1].x += m.data[1].x;
	data[1].y += m.data[1].y;
	return *this;
}

MAT2_DECL mat2& mat2::operator-=(float s)
{
	data[0].x -= s;
	data[0].y -= s;
	data[1].x -= s;
	data[1].y -= s;
	return *this;
}

MAT2_DECL mat2& mat2::operator-=(const mat2& m)
{
	data[0].x -= m.data[0].x;
	data[0].y -= m.data[0].y;
	data[1].x -= m.data[1].x;
	data[1].y -= m.data[1].y;
	return *this;
}

MAT2_DECL mat2& mat2::operator*=(float s)
{
	data[0].x *= s;
	data[0].y *= s;
	data[1].x *= s;
	data[1].y *= s;
	return *this;
}

MAT2_DECL mat2& mat2::operator*=(const mat2& m)
{
	*this = *this * m;
	return *this;
}

MAT2_DECL mat2& mat2::operator/=(float s)
{
	data[0].x /= s;
	data[0].y /= s;
	data[1].x /= s;
	data[1].y /= s;
	return *this;
}

MAT2_DECL mat2 operator+(const mat2& m, float s)
{
	return mat2(m.data[0].x + s, m.data[0].y + s, m.data[1].x + s, m.data[1].y + s);
}

MAT2_DECL mat2 operator+(float s, const mat2& m)
{
	return mat2(m.data[0].x + s, m.data[0].y + s, m.data[1].x + s, m.data[1].y + s);
}

MAT2_DECL mat2 operator+(const mat2& m1, const mat2& m2)
{
	return mat2(m1.data[0].x + m2.data[0].x, m1.data[0].y + m2.data[0].y, m1.data[1].x + m2.data[1].x, m1.data[1].y + m2.data[1].y);
}

MAT2_DECL mat2 operator-(const mat2& m, float s)
{
	return mat2(m.data[0].x - s, m.data[0].y - s, m.data[1].x - s, m.data[1].y - s);
}

MAT2_DECL mat2 operator-(float s, const mat2& m)
{
	return mat2(s - m.data[0].x, s - m.data[0].y, s - m.data[1].x, s - m.data[1].y);
}

MAT2_DECL mat2 operator-(const mat2& m1, const mat2& m2)
{
	return mat2(m1.data[0].x - m2.data[0].x, m1.data[0].y - m2.data[0].y, m1.data[1].x - m2.data[1].x, m1.data[1].y - m2.data[1].y);
}

MAT2_DECL mat2 operator*(const mat2& m, float s)
{
	return mat2(m.data[0].x * s, m.data[0].y * s, m.data[1].x * s, m.data[1].y * s);
}

MAT2_DECL mat2 operator*(float s, const mat2& m)
{
	return mat2(m.data[0].x * s, m.data[0].y * s, m.data[1].x * s, m.data[1].y * s);
}

MAT2_DECL mat2 operator*(const mat2& m1, const mat2& m2)
{
	/*#define MUL2(i, j) */ auto mul = [&m1, &m2](auto i, auto j) {
		return (m1.data[0].data[(j)] * m2.data[(i)].data[0] + m1.data[1].data[(j)] * m2.data[(i)].data[1]);
	};

	return mat2(mul(0, 0), mul(0, 1), mul(1, 0), mul(1, 1));

	//#undef MUL2
}

MAT2_DECL vec2 operator*(const mat2& m, const vec2& v)
{
	return vec2(m.data[0].x * v.x + m.data[1].x * v.y, m.data[0].y * v.x + m.data[1].y * v.y);
}

MAT2_DECL vec2 operator*(const vec2& v, const mat2& m)
{
	return vec2(m.data[0].x * v.x + m.data[0].y * v.y, m.data[1].x * v.x + m.data[1].y * v.y);
}

MAT2_DECL mat2 operator/(const mat2& m, float s)
{
	return mat2(m.data[0].x / s, m.data[0].y / s, m.data[1].x / s, m.data[1].y / s);
}

MAT2_DECL mat2 operator/(float s, const mat2& m)
{
	return mat2(s / m.data[0].x, s / m.data[0].y, s / m.data[1].x, s / m.data[1].y);
}

MAT2_DECL bool operator==(const mat2& m1, const mat2& m2)
{
	return (m1.data[0].x == m2.data[0].x) && (m1.data[0].y == m2.data[0].y) && (m1.data[1].x == m2.data[1].x) &&
	       (m1.data[1].y == m2.data[1].y);
}

MAT2_DECL bool operator!=(const mat2& m1, const mat2& m2)
{
	return (m1.data[0].x != m2.data[0].x) || (m1.data[0].y != m2.data[0].y) || (m1.data[1].x != m2.data[1].x) ||
	       (m1.data[1].y != m2.data[1].y);
}

} // namespace Kyty::Math::m

#endif /* INCLUDE_KYTY_MATH_MAT2IMPL_H_ */
