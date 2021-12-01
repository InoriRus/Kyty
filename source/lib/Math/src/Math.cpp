#include "Kyty/Math/MathAll.h" // IWYU pragma: associated
#include "Kyty/Math/Rand.h"

namespace Kyty::Math {

KYTY_SUBSYSTEM_INIT(Math)
{
	Rand::Init();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Math) {}

KYTY_SUBSYSTEM_DESTROY(Math) {}

} // namespace Kyty::Math
