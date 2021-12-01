#ifndef INCLUDE_KYTY_MATH_RAND_H_
#define INCLUDE_KYTY_MATH_RAND_H_

#include "Kyty/Core/Common.h"

#include <random> // IWYU pragma: export

namespace Kyty::Math {

class Rand
{
public:
	static void Init();

	// random in range [0, 4294967295]
	static uint32_t Uint();

	// random in range [-2147483648, 2147483647]
	static int32_t Int();

	// random in range [0.0, 1.0]
	static double DoubleInclusive();

	// random in range [0.0, 1.0)
	static double Double();

	// random in range [from, to]
	static double DoubleInclusiveRange(double from_incl, double to_incl);

	// random in range [from, to)
	static double DoubleRange(double from_incl, double to_excl);

	// random in range [0.0, 1.0]
	static float FloatInclusive();

	// random in range [0.0, 1.0)
	static float Float();

	// random in range [from, to]
	static float FloatInclusiveRange(float from_incl, float to_incl);

	// random in range [from, to)
	static float FloatRange(float from_incl, float to_excl);

	// random in range [from, to]
	static uint32_t UintInclusiveRange(uint32_t from_incl, uint32_t to_incl);

	// random in range [from, to]
	static int32_t IntInclusiveRange(int32_t from_incl, int32_t to_incl);

	static void Seed(unsigned int s);

	static void SeedBySystemTime();

	static std::mt19937 GetRandomEngine();
};

// using Rand = Math::Rand;

} // namespace Kyty::Math

#endif /* INCLUDE_KYTY_MATH_RAND_H_ */
