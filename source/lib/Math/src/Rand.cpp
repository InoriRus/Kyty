#include "Kyty/Math/Rand.h"

#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/DbgAssert.h"

#include <cfloat>
#include <cmath>
#include <random>

namespace Kyty::Math {

struct RandContextT // NOLINT(cert-msc32-c,cert-msc51-cpp)
{
	std::mt19937                           rnd;
	std::uniform_real_distribution<double> double_distribution = std::uniform_real_distribution<double>(0.0, 1.0);
	std::uniform_real_distribution<double> double_distribution_i =
	    std::uniform_real_distribution<double>(0.0, std::nextafter(1.0, DBL_MAX));
	std::uniform_real_distribution<float> float_distribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
	std::uniform_real_distribution<float> float_distribution_i =
	    std::uniform_real_distribution<float>(0.0f, std::nextafterf(1.0f, FLT_MAX));
};

RandContextT* g_rand_context = nullptr;

void Rand::Init()
{
	g_rand_context = new RandContextT;
}

// random in range [0, 2^32-1]
uint32_t Rand::Uint()
{
	return g_rand_context->rnd();
}

// random in range [0.0, 1.0]
double Rand::DoubleInclusive()
{
	return g_rand_context->double_distribution_i(g_rand_context->rnd);
}

// random in range [0.0, 1.0)
double Rand::Double()
{
	return g_rand_context->double_distribution(g_rand_context->rnd);
}

// random in range [from, to]
double Rand::DoubleInclusiveRange(double from_incl, double to_incl)
{
	EXIT_IF(!(from_incl <= to_incl));

	if (from_incl == to_incl)
	{
		return from_incl;
	}

	std::uniform_real_distribution<double> d(from_incl, std::nextafter(to_incl, DBL_MAX));
	return d(g_rand_context->rnd);
}

// random in range [from, to)
double Rand::DoubleRange(double from_incl, double to_excl)
{
	EXIT_IF(!(from_incl < to_excl));

	std::uniform_real_distribution<double> d(from_incl, to_excl);
	return d(g_rand_context->rnd);
}

// random in range [0.0, 1.0]
float Rand::FloatInclusive()
{
	return g_rand_context->float_distribution_i(g_rand_context->rnd);
}

// random in range [0.0, 1.0)
float Rand::Float()
{
	return g_rand_context->float_distribution(g_rand_context->rnd);
}

// random in range [from, to]
float Rand::FloatInclusiveRange(float from_incl, float to_incl)
{
	EXIT_IF(!(from_incl <= to_incl));

	if (from_incl == to_incl)
	{
		return from_incl;
	}

	std::uniform_real_distribution<float> d(from_incl, std::nextafterf(to_incl, FLT_MAX));
	return d(g_rand_context->rnd);
}

// random in range [from, to)
float Rand::FloatRange(float from_incl, float to_excl)
{
	EXIT_IF(!(from_incl < to_excl));

	std::uniform_real_distribution<float> d(from_incl, to_excl);
	return d(g_rand_context->rnd);
}

// random in range [from, to]
uint32_t Rand::UintInclusiveRange(uint32_t from_incl, uint32_t to_incl)
{
	EXIT_IF(!(from_incl <= to_incl));

	std::uniform_int_distribution<uint32_t> d(from_incl, to_incl);
	return d(g_rand_context->rnd);
}

void Rand::Seed(unsigned int s)
{
	g_rand_context->rnd.seed(s);
}

// random in range [-2147483648, 2147483647]
int32_t Rand::Int()
{
	union cast_u
	{
		uint32_t in;
		int32_t  out;
	} cast {};

	cast.in = Uint();

	return cast.out;
}

// random in range [from, to]
int32_t Rand::IntInclusiveRange(int32_t from_incl, int32_t to_incl)
{
	EXIT_IF(!(from_incl <= to_incl));

	std::uniform_int_distribution<int32_t> d(from_incl, to_incl);
	return d(g_rand_context->rnd);
}

void Rand::SeedBySystemTime()
{
	Rand::Seed(Core::Time::FromSystem().MsecTotal());
}

std::mt19937 Rand::GetRandomEngine()
{
	return g_rand_context->rnd;
}

} // namespace Kyty::Math
