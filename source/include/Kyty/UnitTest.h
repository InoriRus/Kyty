#ifndef UNIT_TEST_INCLUDE_UNITTEST_H_
#define UNIT_TEST_INCLUDE_UNITTEST_H_

#include "Kyty/Core/Common.h"      // IWYU pragma: export
#include "Kyty/Core/MemoryAlloc.h" // IWYU pragma: keep
#include "Kyty/Core/Subsystems.h"

#include "gtest/gtest-message.h"   // IWYU pragma: export
#include "gtest/gtest-test-part.h" // IWYU pragma: export
#include "gtest/gtest.h"           // IWYU pragma: export

#include <memory> // IWYU pragma: export

namespace Kyty::UnitTest {

bool unit_test_all();

KYTY_SUBSYSTEM_DEFINE(UnitTest);

#define UT_MEM_CHECK_INIT() int test_ms = Core::mem_new_state();

#define UT_MEM_CHECK()                                                                                                                     \
	if (!HasFailure())                                                                                                                     \
	{                                                                                                                                      \
		Core::MemStats test_mem_stat = {test_ms, 0, 0};                                                                                    \
		Core::mem_get_stat(&test_mem_stat);                                                                                                \
		size_t   ut_total_allocated = test_mem_stat.total_allocated;                                                                       \
		uint32_t ut_blocks_num      = test_mem_stat.blocks_num;                                                                            \
		if (ut_total_allocated != 0U || ut_blocks_num != 0U) Core::mem_print(test_ms);                                                     \
		EXPECT_EQ(ut_total_allocated, 0U);                                                                                                 \
		EXPECT_EQ(ut_blocks_num, 0U);                                                                                                      \
	}

#define UT_NAMESPACE(name)                                                                                                                 \
	namespace Kyty {                                                                                                                       \
	namespace UnitTest {                                                                                                                   \
	namespace name {

#define UT_BEGIN(name)                                                                                                                     \
	namespace Kyty {                                                                                                                       \
	namespace UnitTest {                                                                                                                   \
	KYTY_FORCE_LINK_THIS(UnitTest##name);                                                                                                  \
	namespace name {

#define UT_END()                                                                                                                           \
	}                                                                                                                                      \
	}                                                                                                                                      \
	}

#define UT_LINK(test) KYTY_FORCE_LINK_THAT(UnitTest##test);

} // namespace Kyty::UnitTest

#endif /* UNIT_TEST_INCLUDE_UNITTEST_H_ */
