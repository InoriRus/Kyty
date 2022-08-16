#include "Kyty/UnitTest.h"

namespace Kyty::UnitTest {

UT_LINK(CoreCharString);
UT_LINK(CoreCharString8);
UT_LINK(CoreMSpace);
UT_LINK(CoreDateTime);

KYTY_SUBSYSTEM_INIT(UnitTest)
{
	testing::InitGoogleTest(KYTY_SUBSYSTEM_ARGC, KYTY_SUBSYSTEM_ARGV);
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(UnitTest) {}

KYTY_SUBSYSTEM_DESTROY(UnitTest) {}

bool unit_test_all()
{
	return RUN_ALL_TESTS() == 0;
}

} // namespace Kyty::UnitTest
