#include "Kyty/Core/MSpace.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/Math/Rand.h"
#include "Kyty/UnitTest.h"

UT_BEGIN(CoreMSpace);

using Core::MSpaceCreate;
using Core::MSpaceDestroy;
using Core::MSpaceFree;
using Core::MSpaceMalloc;
using Core::MSpaceRealloc;
using Math::Rand;

static void test_fail()
{
	size_t s   = 1000;
	auto*  buf = new uint8_t[s];

	auto* m = MSpaceCreate("test", buf, s, true, nullptr);

	EXPECT_EQ(m, nullptr);

	void* ptr = MSpaceMalloc(m, 56);

	EXPECT_EQ(ptr, nullptr);

	EXPECT_FALSE(MSpaceFree(m, ptr));

	EXPECT_FALSE(MSpaceDestroy(m));

	delete[] buf;
}

static size_t   g_size = 0;
static uint8_t* g_ptr  = nullptr;

static void test_callback(Core::mspace_t m, size_t /*free_size*/, size_t size)
{
	g_size = size;
	if (g_ptr != nullptr)
	{
		EXPECT_TRUE(MSpaceFree(m, g_ptr));
	}
}

static void test_ok()
{
	size_t s   = 2000;
	auto*  buf = new uint8_t[s];

	auto* m = MSpaceCreate("test", buf, s, true, test_callback);

	EXPECT_NE(m, nullptr);

	auto* ptr = static_cast<uint8_t*>(MSpaceMalloc(m, 56));
	EXPECT_NE(ptr, nullptr);
	EXPECT_TRUE(ptr > buf && ptr < buf + s);

	g_size     = 0;
	g_ptr      = nullptr;
	auto* ptr2 = static_cast<uint8_t*>(MSpaceMalloc(m, 460));
	EXPECT_EQ(ptr2, nullptr);
	EXPECT_EQ(g_size, 460u);

	g_size = 0;
	g_ptr  = ptr;
	ptr2   = static_cast<uint8_t*>(MSpaceMalloc(m, 460));
	EXPECT_NE(ptr2, nullptr);
	EXPECT_TRUE(ptr2 > buf && ptr2 < buf + s);

	EXPECT_TRUE(MSpaceFree(m, ptr2));

	g_size = 0;
	g_ptr  = nullptr;
	ptr2   = static_cast<uint8_t*>(MSpaceRealloc(m, nullptr, 60));
	EXPECT_NE(ptr2, nullptr);
	EXPECT_TRUE(ptr2 > buf && ptr2 < buf + s);
	ptr2 = static_cast<uint8_t*>(MSpaceRealloc(m, ptr2, 160));
	EXPECT_NE(ptr2, nullptr);
	EXPECT_TRUE(ptr2 > buf && ptr2 < buf + s);

	EXPECT_TRUE(MSpaceDestroy(m));

	delete[] buf;
}

static void test_align()
{
	uint32_t s   = Rand::UintInclusiveRange(5000, 20000) & ~0x7u;
	auto*    buf = new uint8_t[s];

	auto* m = MSpaceCreate("test", buf, s, true, nullptr);

	EXPECT_NE(m, nullptr);

	// printf("buf = %016" PRIx64 ", %u\n", reinterpret_cast<uint64_t>(buf), s);

	int iter_num = Rand::IntInclusiveRange(10, 20);

	for (int i = 0; i < iter_num; i++)
	{
		uint32_t size     = Rand::UintInclusiveRange(1, 64);
		uint32_t size_r   = Rand::UintInclusiveRange(32, 128);
		auto*    buf32    = MSpaceMalloc(m, size);
		auto*    buf32_r  = MSpaceRealloc(m, buf32, size_r);
		auto     addr32   = reinterpret_cast<uint64_t>(buf32);
		auto     addr32_r = reinterpret_cast<uint64_t>(buf32_r);
		// printf("%016" PRIx64 ", %u => %016" PRIx64 ", %u\n", addr32, size, addr32_r, size_r);
		EXPECT_NE(addr32, 0u);
		EXPECT_EQ(addr32 & 0x1fu, 0u);
		EXPECT_NE(addr32_r, 0u);
		EXPECT_EQ(addr32_r & 0x1fu, 0u);
	}

	EXPECT_TRUE(MSpaceDestroy(m));

	delete[] buf;
}

struct TestRecord
{
	uint8_t* buf     = nullptr;
	uint8_t  pattern = 0;
	uint32_t size    = 0;
};

static void test_fill()
{
	uint32_t s   = Rand::UintInclusiveRange(2000, 10000) & ~0x7u;
	auto*    buf = new uint8_t[s];

	auto* m = MSpaceCreate("test", buf, s, true, nullptr);
	EXPECT_NE(m, nullptr);

	Vector<TestRecord> rs;

	for (int step = 0; step < 5; step++)
	{
		int add = 0;
		int del = 0;
		int rea = 0;
		for (;;)
		{
			uint32_t size  = Rand::UintInclusiveRange(1, 200);
			auto*    buf32 = static_cast<uint8_t*>(MSpaceMalloc(m, size));
			if (buf32 == nullptr)
			{
				break;
			}
			uint8_t pattern = Rand::UintInclusiveRange(0, 255);
			memset(buf32, pattern, size);

			TestRecord r {};
			r.buf     = buf32;
			r.pattern = pattern;
			r.size    = size;

			rs.Add(r);
			add++;
		}

		for (auto& r: rs)
		{
			if (r.buf != nullptr && (Rand::Uint() % 8) == 0)
			{
				MSpaceFree(m, r.buf);
				r.buf = nullptr;
				del++;
			}
		}

		for (auto& r: rs)
		{
			if (r.buf != nullptr && (Rand::Uint() % 4) == 0)
			{
				auto* n = static_cast<uint8_t*>(MSpaceRealloc(m, r.buf, r.size + Rand::UintInclusiveRange(1, 200)));
				if (n != nullptr)
				{
					r.buf = n;
					rea++;
				}
			}
		}

		// printf("add = %d, del = %d, rea = %d\n", add, del, rea);
	}

	bool ok = true;
	for (const auto& r: rs)
	{
		if (r.buf != nullptr)
		{
			for (uint32_t i = 0; i < r.size; i++)
			{
				if (r.pattern != r.buf[i])
				{
					ok = false;
					break;
				}
			}
			if (!ok)
			{
				break;
			}
		}
	}
	EXPECT_TRUE(ok);

	EXPECT_TRUE(MSpaceDestroy(m));
	delete[] buf;
}

TEST(Core, MSpace)
{
	UT_MEM_CHECK_INIT();

	test_fail();
	test_ok();

	for (int i = 0; i < 5; i++)
	{
		test_align();
		test_fill();
	}

	UT_MEM_CHECK();
}

UT_END();
