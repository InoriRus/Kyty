#include "Kyty/Core/String8.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/UnitTest.h"

UT_BEGIN(CoreCharString8);

using Core::StringList8;

void test()
{
	String8 e;
	String8 s = String8("abcdABCDАБВГДабвгд");

	EXPECT_EQ(s.Size(), 28U);
	EXPECT_TRUE(!s.IsEmpty());
	EXPECT_EQ(e.Size(), 0U);
	EXPECT_TRUE(e.IsEmpty());

	e = s;
	s.Clear();

	EXPECT_EQ(s.Size(), 0U);
	EXPECT_TRUE(s.IsEmpty());
	EXPECT_EQ(e.Size(), 28U);
	EXPECT_TRUE(!e.IsEmpty());

	EXPECT_EQ(e[1], 'b');
	EXPECT_EQ(e[9], '\x90');

	s = String8("acdABCАВГДабвгд");
	s = String8("abcdABCDАБВГДабвгд");

	EXPECT_EQ(s[3], 'd');
	EXPECT_EQ(s[11], '\x91');

	s = '0';

	EXPECT_EQ(s.Size(), 1U);

	String8 ss = String8("abcd");
	ss += String8("efgh");
	EXPECT_EQ(strcmp(ss.c_str(), "abcdefgh"), 0);

	String8 sum1 = String8("ab1") + String8("cd2");
	String8 sum2 = sum1 + "cd2" + "апр";

	EXPECT_TRUE(sum2 == "ab1cd2cd2апр");
	EXPECT_TRUE(sum2 != "ab1cd2cd2апР");

	String8 mt = String8("0123456789");

	EXPECT_TRUE(String8("").IsEmpty());

	EXPECT_EQ(mt.Mid(0, 4), "0123");
	EXPECT_EQ(mt.Mid(6, 4), "6789");
	EXPECT_EQ(mt.Mid(6, 5), "6789");
	EXPECT_EQ(mt.Mid(10, 1), "");
	EXPECT_EQ(mt.Mid(0, 0), "");

	EXPECT_EQ(mt.Left(2), "01");
	EXPECT_EQ(mt.Left(0), "");
	EXPECT_EQ(mt.Left(12), "0123456789");

	EXPECT_EQ(mt.Right(2), "89");
	EXPECT_EQ(mt.Right(0), "");
	EXPECT_EQ(mt.Right(12), "0123456789");
}

void test_3()
{
	String8 tt  = String8("  123  abc\r\n");
	String8 tt2 = String8(" \t  \r\n ");
	String8 tt3 = String8("no_space");
	EXPECT_EQ(tt.TrimLeft(), "123  abc\r\n");
	EXPECT_EQ(tt.TrimRight(), "  123  abc");
	EXPECT_EQ(tt.Trim(), "123  abc");
	EXPECT_EQ(tt.Simplify(), "123 abc");
	EXPECT_EQ(tt2.TrimLeft(), "");
	EXPECT_EQ(tt2.TrimRight(), "");
	EXPECT_EQ(tt2.Trim(), "");
	EXPECT_EQ(tt2.Simplify(), "");
	EXPECT_EQ(tt3.TrimLeft(), tt3);
	EXPECT_EQ(tt3.TrimRight(), tt3);
	EXPECT_EQ(tt3.Trim(), tt3);
	EXPECT_EQ(tt3.Simplify(), tt3);

	EXPECT_EQ(String8("й ц у к е н").ReplaceChar(' ', '_'), "й_ц_у_к_е_н");
	EXPECT_EQ(String8("й ц у к е н").RemoveChar(' '), "йцукен");
	// EXPECT_EQ(String8("й ц у к е н").RemoveAt(1).RemoveAt(2).RemoveAt(3), "йцук е н");
	EXPECT_EQ(String8("йцукен").RemoveAt(0, 0), "йцукен");
	// EXPECT_EQ(String8("йцукен").RemoveAt(10, 1), "йцукен");
	// EXPECT_EQ(String8("йцукен").RemoveAt(0, 10), "");
	// EXPECT_EQ(String8("йцукен").RemoveAt(1, 10), "й");
	// EXPECT_EQ(String8("йцукен").RemoveAt(1, 2), "йкен");

	EXPECT_EQ(String8("\nй\nц\n\nу к е н\n").ReplaceStr("\n", "\r\n"), "\r\nй\r\nц\r\n\r\nу к е н\r\n");
	EXPECT_EQ(String8("\nй\nц\n\nу к е н\n").RemoveStr("\n"), "йцу к е н");
	EXPECT_EQ(String8("abcabcabc").RemoveStr("bc"), "aaa");

	String8 it = String8("abcЙЦУ123");
	EXPECT_EQ(it.InsertAt(0, 'q'), "qabcЙЦУ123");
	EXPECT_EQ(it.InsertAt(3, "_d_"), "abc_d_ЙЦУ123");
	// EXPECT_EQ(it.InsertAt(10, "000"), "abcЙЦУ123000");

	String8 ft = String8("ab ab ab");
	EXPECT_EQ(ft.FindIndex("ab", 0), 0U);
	EXPECT_EQ(ft.FindIndex("ab", 1), 3U);
	EXPECT_EQ(ft.FindIndex("AB", 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("ab", 7), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("ab", 15), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("abc", 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("", 5), 5U);

	EXPECT_EQ(ft.FindLastIndex("ab"), 6U);
	EXPECT_EQ(ft.FindLastIndex("ab", 6), 6U);
	EXPECT_EQ(ft.FindLastIndex("ab", 5), 3U);
	EXPECT_EQ(ft.FindLastIndex("AB"), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindLastIndex(""), 7U);

	EXPECT_EQ(ft.FindIndex('a', 0), 0U);
	EXPECT_EQ(ft.FindIndex('a', 1), 3U);
	EXPECT_EQ(ft.FindIndex('A', 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('a', 7), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('a', 15), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('c', 0), Core::STRING8_INVALID_INDEX);

	EXPECT_EQ(ft.FindLastIndex('a'), 6U);
	EXPECT_EQ(ft.FindLastIndex('a', 6), 6U);
	EXPECT_EQ(ft.FindLastIndex('a', 5), 3U);
	EXPECT_EQ(ft.FindLastIndex('A'), Core::STRING8_INVALID_INDEX);

	String8 ft2 = String8("ab AB ab");
}

void test_2()
{
	String8 pf;
	EXPECT_TRUE(pf.Printf("s%s_d%d\n", "abcdАБВГ", 4));
	EXPECT_EQ(pf, "sabcdАБВГ_d4\n");

	EXPECT_TRUE(String8("abcdabcd").StartsWith("abc"));
	EXPECT_TRUE(String8("abcdabcd").StartsWith(""));
	EXPECT_TRUE(String8("").StartsWith(""));
	EXPECT_TRUE(!String8("").StartsWith("abc"));
	EXPECT_TRUE(!String8("abcdabcd").StartsWith("aBc"));
	EXPECT_TRUE(String8("abcdabcd").StartsWith('a'));
	EXPECT_TRUE(!String8("").StartsWith('a'));
	EXPECT_TRUE(!String8("abcdabcd").StartsWith('A'));
	EXPECT_TRUE(String8("abcdabcd").EndsWith("cd"));
	EXPECT_TRUE(!String8("abcdabcd").EndsWith("bc"));
	EXPECT_TRUE(!String8("").EndsWith("cd"));
	EXPECT_TRUE(String8("abcdabcd").EndsWith(""));
	EXPECT_TRUE(String8("").EndsWith(""));
	EXPECT_TRUE(String8("abcdabcd").EndsWith('d'));
	EXPECT_TRUE(!String8("abcdabcd").EndsWith('D'));
	EXPECT_TRUE(!String8("").EndsWith('d'));

	EXPECT_TRUE(String8("abcdabcd").ContainsStr("cda"));
	EXPECT_TRUE(String8("abcdabcd").ContainsStr(""));
	EXPECT_TRUE(!String8("").ContainsStr("cda"));
	EXPECT_TRUE(String8("").ContainsStr(""));
	EXPECT_TRUE(!String8("abcdabcd").ContainsStr("cDa"));

	EXPECT_TRUE(String8("abcd").ContainsChar('b'));
	EXPECT_TRUE(!String8("").ContainsChar('c'));
	EXPECT_TRUE(!String8("abcdabcd").ContainsChar('D'));

	EXPECT_EQ(String8("123456789012345678900").ToInt32(), INT32_MAX);
	EXPECT_EQ(String8("-123456789012345678900").ToInt32(), INT32_MIN);
	EXPECT_EQ(String8("123456789012345678900").ToUint32(), UINT32_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String8("-123456789012345678900").ToUint32(), UINT32_MAX);
#else
	// EXPECT_EQ(String8("-123456789012345678900").ToUint32(), (~UINT32_MAX) + 1);
#endif
	EXPECT_EQ(String8("123456789012345678900").ToInt64(), INT64_MAX);
	EXPECT_EQ(String8("-123456789012345678900").ToInt64(), INT64_MIN);
	EXPECT_EQ(String8("123456789012345678900").ToUint64(), UINT64_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String8("-123456789012345678900").ToUint64(), UINT64_MAX);
#else
	// EXPECT_EQ(String8("-123456789012345678900").ToUint64(), (~UINT64_MAX) + 1);
#endif
	EXPECT_EQ(String8("0xabcd").ToUint32(16), 0xabcdU);
	EXPECT_EQ(String8("-0.345").ToDouble(), -0.345);

	EXPECT_EQ(String8("abcd/dede/dcdc").DirectoryWithoutFilename(), String8("abcd/dede/"));
	EXPECT_EQ(String8("abcd/dede/").DirectoryWithoutFilename(), String8("abcd/dede/"));
	EXPECT_EQ(String8("/abcd/dede/dcdc").DirectoryWithoutFilename(), String8("/abcd/dede/"));
	EXPECT_EQ(String8("abcddededcdc").DirectoryWithoutFilename(), String8(""));
	EXPECT_EQ(String8("abcd/dede/dcdc").FilenameWithoutDirectory(), String8("dcdc"));
	EXPECT_EQ(String8("abcd/dede/").FilenameWithoutDirectory(), String8(""));
	EXPECT_EQ(String8("/abcd/dede/dcdc").FilenameWithoutDirectory(), String8("dcdc"));
	EXPECT_EQ(String8("abcddededcdc").FilenameWithoutDirectory(), String8("abcddededcdc"));

	EXPECT_EQ(String8("abcdef").RemoveLast(0), String8("abcdef"));
	EXPECT_EQ(String8("abcdef").RemoveLast(1), String8("abcde"));
	EXPECT_EQ(String8("abcdef").RemoveLast(6), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveLast(7), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveFirst(0), String8("abcdef"));
	EXPECT_EQ(String8("abcdef").RemoveFirst(1), String8("bcdef"));
	EXPECT_EQ(String8("abcdef").RemoveFirst(6), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveFirst(7), String8(""));

	EXPECT_EQ(String8("abcd.ext").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8("abcd").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8("abcd.").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8(".ext").FilenameWithoutExtension(), String8(""));

	EXPECT_EQ(String8("abcd.ext").ExtensionWithoutFilename(), String8(".ext"));
	EXPECT_EQ(String8("abcd").ExtensionWithoutFilename(), String8(""));
	EXPECT_EQ(String8("abcd.").ExtensionWithoutFilename(), String8("."));
	EXPECT_EQ(String8(".ext").ExtensionWithoutFilename(), String8(".ext"));

	String8 s = String8("test");
	String8 s2(s.GetDataConst() + 2);
	EXPECT_EQ(s2, "st");

	String8 word = "ebadc";
	EXPECT_EQ(word.SortChars(), "abcde");
}

void test_U()
{
	String8 e;
	String8 s = "abcdABCDАБВГДабвгд";

	EXPECT_EQ(s.Size(), 28U);
	EXPECT_TRUE(!s.IsEmpty());
	EXPECT_EQ(e.Size(), 0U);
	EXPECT_TRUE(e.IsEmpty());

	e = s;
	s.Clear();

	EXPECT_EQ(s.Size(), 0U);
	EXPECT_TRUE(s.IsEmpty());
	EXPECT_EQ(e.Size(), 28U);
	EXPECT_TRUE(!e.IsEmpty());

	EXPECT_EQ(e[1], 'b');
	EXPECT_EQ(e[9], '\x90');

	s = "acdABCАВГДабвгд";
	s = "abcdABCDАБВГДабвгд";

	EXPECT_EQ(s[3], 'd');
	EXPECT_EQ(s[11], '\x91');

	s = '0';

	EXPECT_EQ(s.Size(), 1U);

	String8 ss = "abcd";
	ss += "efgh";
	EXPECT_EQ(strcmp(ss.c_str(), "abcdefgh"), 0);

	String8 sum1 = "ab1" + String8("cd2");
	String8 sum2 = sum1 + "cd2" + "апр";

	EXPECT_TRUE(sum2 == "ab1cd2cd2апр");
	EXPECT_TRUE(sum2 != "ab1cd2cd2апР");

	sum1 = "_ab1" + String8("cd2");
	sum2 = sum1 + "cd2" + "апр";

	EXPECT_TRUE(sum2 == "_ab1cd2cd2апр");
	EXPECT_TRUE(sum2 != "_ab1cd2cd2апР");

	String8 mt = "0123456789";

	EXPECT_TRUE(String8("").IsEmpty());

	EXPECT_EQ(mt.Mid(0, 4), "0123");
	EXPECT_EQ(mt.Mid(6, 4), "6789");
	EXPECT_EQ(mt.Mid(6, 5), "6789");
	EXPECT_EQ(mt.Mid(10, 1), "");
	EXPECT_EQ(mt.Mid(0, 0), "");

	EXPECT_EQ(mt.Left(2), "01");
	EXPECT_EQ(mt.Left(0), "");
	EXPECT_EQ(mt.Left(12), "0123456789");

	EXPECT_EQ(mt.Right(2), "89");
	EXPECT_EQ(mt.Right(0), "");
	EXPECT_EQ(mt.Right(12), "0123456789");
}

void test_U_3()
{
	String8 tt  = "  123  abc\r\n";
	String8 tt2 = " \t  \r\n ";
	String8 tt3 = "no_space";
	EXPECT_EQ(tt.TrimLeft(), "123  abc\r\n");
	EXPECT_EQ(tt.TrimRight(), "  123  abc");
	EXPECT_EQ(tt.Trim(), "123  abc");
	EXPECT_EQ(tt.Simplify(), "123 abc");
	EXPECT_EQ(tt2.TrimLeft(), "");
	EXPECT_EQ(tt2.TrimRight(), "");
	EXPECT_EQ(tt2.Trim(), "");
	EXPECT_EQ(tt2.Simplify(), "");
	EXPECT_EQ(tt3.TrimLeft(), tt3);
	EXPECT_EQ(tt3.TrimRight(), tt3);
	EXPECT_EQ(tt3.Trim(), tt3);
	EXPECT_EQ(tt3.Simplify(), tt3);

	EXPECT_EQ(String8("й ц у к е н").ReplaceChar(' ', '_'), "й_ц_у_к_е_н");
	EXPECT_EQ(String8("й ц у к е н").RemoveChar(' '), "йцукен");
	// EXPECT_EQ(String8("й ц у к е н").RemoveAt(1).RemoveAt(2).RemoveAt(3), "йцук е н");
	EXPECT_EQ(String8("йцукен").RemoveAt(0, 0), "йцукен");
	// EXPECT_EQ(String8("йцукен").RemoveAt(10, 1), "йцукен");
	// EXPECT_EQ(String8("йцукен").RemoveAt(0, 10), "");
	// EXPECT_EQ(String8("йцукен").RemoveAt(1, 10), "й");
	// EXPECT_EQ(String8("йцукен").RemoveAt(1, 2), "йкен");

	EXPECT_EQ(String8("\nй\nц\n\nу к е н\n").ReplaceStr("\n", "\r\n"), "\r\nй\r\nц\r\n\r\nу к е н\r\n");
	EXPECT_EQ(String8("\nй\nц\n\nу к е н\n").RemoveStr("\n"), "йцу к е н");
	EXPECT_EQ(String8("abcabcabc").RemoveStr("bc"), "aaa");

	String8 it = "abcЙЦУ123";
	EXPECT_EQ(it.InsertAt(0, 'q'), "qabcЙЦУ123");
	EXPECT_EQ(it.InsertAt(3, "_d_"), "abc_d_ЙЦУ123");
	// EXPECT_EQ(it.InsertAt(10, "000"), "abcЙЦУ123000");

	String8 ft = "ab ab ab";
	EXPECT_EQ(ft.FindIndex("ab", 0), 0U);
	EXPECT_EQ(ft.FindIndex("ab", 1), 3U);
	EXPECT_EQ(ft.FindIndex("AB", 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("ab", 7), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("ab", 15), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("abc", 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex("", 5), 5U);

	EXPECT_EQ(ft.FindLastIndex("ab"), 6U);
	EXPECT_EQ(ft.FindLastIndex("ab", 6), 6U);
	EXPECT_EQ(ft.FindLastIndex("ab", 5), 3U);
	EXPECT_EQ(ft.FindLastIndex("AB"), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindLastIndex(""), 7U);

	EXPECT_EQ(ft.FindIndex('a', 0), 0U);
	EXPECT_EQ(ft.FindIndex('a', 1), 3U);
	EXPECT_EQ(ft.FindIndex('A', 0), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('a', 7), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('a', 15), Core::STRING8_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex('c', 0), Core::STRING8_INVALID_INDEX);

	EXPECT_EQ(ft.FindLastIndex('a'), 6U);
	EXPECT_EQ(ft.FindLastIndex('a', 6), 6U);
	EXPECT_EQ(ft.FindLastIndex('a', 5), 3U);
	EXPECT_EQ(ft.FindLastIndex('A'), Core::STRING8_INVALID_INDEX);

	String8 ft2 = "ab AB ab";
}

void test_U_2()
{
	String8 pf;
	EXPECT_TRUE(pf.Printf("s%s_d%d\n", "abcdАБВГ", 4));
	EXPECT_EQ(pf, "sabcdАБВГ_d4\n");

	EXPECT_TRUE(String8("abcdabcd").StartsWith("abc"));
	EXPECT_TRUE(String8("abcdabcd").StartsWith(""));
	EXPECT_TRUE(String8("").StartsWith(""));
	EXPECT_TRUE(!String8("").StartsWith("abc"));
	EXPECT_TRUE(!String8("abcdabcd").StartsWith("aBc"));
	EXPECT_TRUE(String8("abcdabcd").StartsWith('a'));
	EXPECT_TRUE(!String8("").StartsWith('a'));
	EXPECT_TRUE(!String8("abcdabcd").StartsWith('A'));
	EXPECT_TRUE(String8("abcdabcd").EndsWith("cd"));
	EXPECT_TRUE(!String8("abcdabcd").EndsWith("bc"));
	EXPECT_TRUE(!String8("").EndsWith("cd"));
	EXPECT_TRUE(String8("abcdabcd").EndsWith(""));
	EXPECT_TRUE(String8("").EndsWith(""));
	EXPECT_TRUE(String8("abcdabcd").EndsWith('d'));
	EXPECT_TRUE(!String8("abcdabcd").EndsWith('D'));
	EXPECT_TRUE(!String8("").EndsWith('d'));

	EXPECT_TRUE(String8("abcdabcd").ContainsStr("cda"));
	EXPECT_TRUE(String8("abcdabcd").ContainsStr(""));
	EXPECT_TRUE(!String8("").ContainsStr("cda"));
	EXPECT_TRUE(String8("").ContainsStr(""));
	EXPECT_TRUE(!String8("abcdabcd").ContainsStr("cDa"));

	EXPECT_TRUE(String8("abcd").ContainsChar('b'));
	EXPECT_TRUE(!String8("").ContainsChar('c'));
	EXPECT_TRUE(!String8("abcdabcd").ContainsChar('D'));

	EXPECT_EQ(String8("123456789012345678900").ToInt32(), INT32_MAX);
	EXPECT_EQ(String8("-123456789012345678900").ToInt32(), INT32_MIN);
	EXPECT_EQ(String8("123456789012345678900").ToUint32(), UINT32_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String8("-123456789012345678900").ToUint32(), UINT32_MAX);
#else
	// EXPECT_EQ(String8("-123456789012345678900").ToUint32(), (~UINT32_MAX) + 1);
#endif
	EXPECT_EQ(String8("123456789012345678900").ToInt64(), INT64_MAX);
	EXPECT_EQ(String8("-123456789012345678900").ToInt64(), INT64_MIN);
	EXPECT_EQ(String8("123456789012345678900").ToUint64(), UINT64_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String8("-123456789012345678900").ToUint64(), UINT64_MAX);
#else
	// EXPECT_EQ(String8("-123456789012345678900").ToUint64(), (~UINT64_MAX) + 1);
#endif
	EXPECT_EQ(String8("0xabcd").ToUint32(16), 0xabcdU);
	EXPECT_EQ(String8("-0.345").ToDouble(), -0.345);

	EXPECT_EQ(String8("abcd/dede/dcdc").DirectoryWithoutFilename(), String8("abcd/dede/"));
	EXPECT_EQ(String8("abcd/dede/").DirectoryWithoutFilename(), String8("abcd/dede/"));
	EXPECT_EQ(String8("/abcd/dede/dcdc").DirectoryWithoutFilename(), String8("/abcd/dede/"));
	EXPECT_EQ(String8("abcddededcdc").DirectoryWithoutFilename(), String8(""));
	EXPECT_EQ(String8("abcd/dede/dcdc").FilenameWithoutDirectory(), String8("dcdc"));
	EXPECT_EQ(String8("abcd/dede/").FilenameWithoutDirectory(), String8(""));
	EXPECT_EQ(String8("/abcd/dede/dcdc").FilenameWithoutDirectory(), String8("dcdc"));
	EXPECT_EQ(String8("abcddededcdc").FilenameWithoutDirectory(), String8("abcddededcdc"));

	EXPECT_EQ(String8("abcdef").RemoveLast(0), String8("abcdef"));
	EXPECT_EQ(String8("abcdef").RemoveLast(1), String8("abcde"));
	EXPECT_EQ(String8("abcdef").RemoveLast(6), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveLast(7), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveFirst(0), String8("abcdef"));
	EXPECT_EQ(String8("abcdef").RemoveFirst(1), String8("bcdef"));
	EXPECT_EQ(String8("abcdef").RemoveFirst(6), String8(""));
	EXPECT_EQ(String8("abcdef").RemoveFirst(7), String8(""));

	EXPECT_EQ(String8("abcd.ext").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8("abcd").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8("abcd.").FilenameWithoutExtension(), String8("abcd"));
	EXPECT_EQ(String8(".ext").FilenameWithoutExtension(), String8(""));

	EXPECT_EQ(String8("abcd.ext").ExtensionWithoutFilename(), String8(".ext"));
	EXPECT_EQ(String8("abcd").ExtensionWithoutFilename(), String8(""));
	EXPECT_EQ(String8("abcd.").ExtensionWithoutFilename(), String8("."));
	EXPECT_EQ(String8(".ext").ExtensionWithoutFilename(), String8(".ext"));

	String8 s = "test";
	String8 s2(s.GetDataConst() + 2);
	EXPECT_EQ(s2, "st");
}

void test_list()
{
	StringList8 list = String8(" a b  У р e ").Split(" ");

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String8(",a,b,,У,р,e,").Split(",", String8::SplitType::WithEmptyParts);

	EXPECT_EQ(list[0], "");
	EXPECT_EQ(list[1], "a");
	EXPECT_EQ(list[2], "b");
	EXPECT_EQ(list[3], "");
	EXPECT_EQ(list[4], "У");
	EXPECT_EQ(list[5], "р");
	EXPECT_EQ(list[6], "e");
	EXPECT_EQ(list[7], "");

	list = String8("qaQbqqУQрqeQ").Split("q", String8::SplitType::SplitNoEmptyParts);

	EXPECT_EQ(list[0], "aQb");
	EXPECT_EQ(list[1], "УQр");
	EXPECT_EQ(list[2], "eQ");

	StringList8 list2 = list;

	list[0] = String8("a");
	list[1] = "b";
	list[2] = "c";

	EXPECT_EQ(list2[0], "aQb");
	EXPECT_EQ(list2[1], "УQр");
	EXPECT_EQ(list2[2], "eQ");

	EXPECT_TRUE(list.Contains("b"));
	EXPECT_TRUE(!list.Contains("B"));

	EXPECT_TRUE(String8("fdabn").ContainsAnyStr(list));
	EXPECT_TRUE(!String8("Cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String8("cfdabn").ContainsAllStr(list));

	EXPECT_EQ(list.Concat(", "), "a, b, c");

	auto chars = list.Concat("");

	EXPECT_TRUE(String8("fdabn").ContainsAnyChar(chars));
	EXPECT_TRUE(!String8("Cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String8("cfdabn").ContainsAllChar(chars));

	StringList8 l1 = String8("a b c d e f").Split(" ");
	StringList8 l2 = String8("a b c d e f").Split(" ");
	EXPECT_TRUE(l1.Equal(l2));
	EXPECT_TRUE(l1 == l2);
	l1.Add("Q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	l2.Add("q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
}

void test_list_2()
{
	StringList8 list = String8(" a b  У р e ").Split(' ');

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String8(",a,b,,У,р,e,").Split(',', String8::SplitType::WithEmptyParts);

	EXPECT_EQ(list[0], "");
	EXPECT_EQ(list[1], "a");
	EXPECT_EQ(list[2], "b");
	EXPECT_EQ(list[3], "");
	EXPECT_EQ(list[4], "У");
	EXPECT_EQ(list[5], "р");
	EXPECT_EQ(list[6], "e");
	EXPECT_EQ(list[7], "");

	list = String8("qaQbqqУQрqeQ").Split('q', String8::SplitType::SplitNoEmptyParts);

	EXPECT_EQ(list[0], "aQb");
	EXPECT_EQ(list[1], "УQр");
	EXPECT_EQ(list[2], "eQ");

	StringList8 list2 = list;

	list[0] = String8("a");
	list[1] = "b";
	list[2] = "c";

	EXPECT_EQ(list2[0], "aQb");
	EXPECT_EQ(list2[1], "УQр");
	EXPECT_EQ(list2[2], "eQ");

	EXPECT_TRUE(list.Contains("b"));
	EXPECT_TRUE(!list.Contains("B"));

	EXPECT_TRUE(String8("fdabn").ContainsAnyStr(list));
	EXPECT_TRUE(!String8("Cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String8("cfdabn").ContainsAllStr(list));

	EXPECT_EQ(list.Concat(','), "a,b,c");

	auto chars = list.Concat("");

	EXPECT_TRUE(String8("fdabn").ContainsAnyChar(chars));
	EXPECT_TRUE(!String8("Cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String8("cfdabn").ContainsAllChar(chars));

	StringList8 l1 = String8("a b c d e f").Split(' ');
	StringList8 l2 = String8("a b c d e f").Split(' ');
	EXPECT_TRUE(l1.Equal(l2));
	EXPECT_TRUE(l1 == l2);
	l1.Add("Q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	l2.Add("q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
}

static void test_printf()
{
	int8_t   i8_1  = -1;
	uint8_t  u8_1  = i8_1;
	int16_t  i16_1 = -1;
	uint16_t u16_1 = i16_1;
	int32_t  i32_1 = -1;
	uint32_t u32_1 = i32_1;
	int64_t  i64_1 = -1;
	uint64_t u64_1 = i64_1;

	int8_t   i8_0  = 0;
	uint8_t  u8_0  = 0;
	int16_t  i16_0 = 0;
	uint16_t u16_0 = 0;
	int32_t  i32_0 = 0;
	uint32_t u32_0 = 0;
	int64_t  i64_0 = 0;
	uint64_t u64_0 = 0;

	String8 s;
	s.Printf("%" PRIi8 " %" PRIi8 " %" PRIi8 " %" PRIi8, i8_1, i8_0, i8_1, i8_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRId8 " %" PRId8 " %" PRId8 " %" PRId8, i8_1, i8_0, i8_1, i8_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRIu8 " %" PRIu8 " %" PRIu8 " %" PRIu8, u8_1, u8_0, u8_1, u8_0);
	EXPECT_EQ(s, "255 0 255 0");
	s.Printf("%02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8, u8_1, u8_0, u8_1, u8_0);
	EXPECT_EQ(s, "ff 00 ff 00");
	s.Printf("%02" PRIX8 " %02" PRIX8 " %02" PRIX8 " %02" PRIX8, u8_1, u8_0, u8_1, u8_0);
	EXPECT_EQ(s, "FF 00 FF 00");

	s.Printf("%" PRIi16 " %" PRIi16 " %" PRIi16 " %" PRIi16, i16_1, i16_0, i16_1, i16_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRId16 " %" PRId16 " %" PRId16 " %" PRId16, i16_1, i16_0, i16_1, i16_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRIu16 " %" PRIu16 " %" PRIu16 " %" PRIu16, u16_1, u16_0, u16_1, u16_0);
	EXPECT_EQ(s, "65535 0 65535 0");
	s.Printf("%04" PRIx16 " %04" PRIx16 " %04" PRIx16 " %04" PRIx16, u16_1, u16_0, u16_1, u16_0);
	EXPECT_EQ(s, "ffff 0000 ffff 0000");
	s.Printf("%04" PRIX16 " %04" PRIX16 " %04" PRIX16 " %04" PRIX16, u16_1, u16_0, u16_1, u16_0);
	EXPECT_EQ(s, "FFFF 0000 FFFF 0000");

	s.Printf("%" PRIi32 " %" PRIi32 " %" PRIi32 " %" PRIi32, i32_1, i32_0, i32_1, i32_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRId32 " %" PRId32 " %" PRId32 " %" PRId32, i32_1, i32_0, i32_1, i32_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32, u32_1, u32_0, u32_1, u32_0);
	EXPECT_EQ(s, "4294967295 0 4294967295 0");
	s.Printf("%08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32, u32_1, u32_0, u32_1, u32_0);
	EXPECT_EQ(s, "ffffffff 00000000 ffffffff 00000000");
	s.Printf("%08" PRIX32 " %08" PRIX32 " %08" PRIX32 " %08" PRIX32, u32_1, u32_0, u32_1, u32_0);
	EXPECT_EQ(s, "FFFFFFFF 00000000 FFFFFFFF 00000000");

	s.Printf("%" PRIi64 " %" PRIi64 " %" PRIi64 " %" PRIi64, i64_1, i64_0, i64_1, i64_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRId64 " %" PRId64 " %" PRId64 " %" PRId64, i64_1, i64_0, i64_1, i64_0);
	EXPECT_EQ(s, "-1 0 -1 0");
	s.Printf("%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64, u64_1, u64_0, u64_1, u64_0);
	EXPECT_EQ(s, "18446744073709551615 0 18446744073709551615 0");
	s.Printf("%016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %016" PRIx64, u64_1, u64_0, u64_1, u64_0);
	EXPECT_EQ(s, "ffffffffffffffff 0000000000000000 ffffffffffffffff 0000000000000000");
	s.Printf("%016" PRIX64 " %016" PRIX64 " %016" PRIX64 " %016" PRIX64, u64_1, u64_0, u64_1, u64_0);
	EXPECT_EQ(s, "FFFFFFFFFFFFFFFF 0000000000000000 FFFFFFFFFFFFFFFF 0000000000000000");
}

void test_cpp14()
{
	StringList8 s = {"a", "b", "cd"};

	EXPECT_EQ(s.Size(), 3U);
	EXPECT_EQ(s.At(0), "a");
	EXPECT_EQ(s.At(1), "b");
	EXPECT_EQ(s.At(2), "cd");

	String8 ss;
	for (const auto& str: s)
	{
		ss += str;
	}

	EXPECT_EQ(ss, "abcd");

	uint8_t int_c[] = {0xc3, 0xae, 0xed, 0x82, 0xb0, 0xf0, 0xa0, 0xbf, 0xbf};
	String8 s1      = String8("î킰𠿿");
	String8 s2      = "î킰𠿿";

	int i = 0;
	i     = 0;
	for (auto ch: s1)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;
	for (auto& ch: s1)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;
	for (const auto& ch: s1)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;
	for (auto ch: s2)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;
	for (auto& ch: s2)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;
	for (const auto& ch: s2)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++]));
	}
	i = 0;

	EXPECT_EQ(s1, s2);

	EXPECT_EQ(s1.Hash(), s2.Hash());

	for (auto& ch: s1)
	{
		ch++;
	}

	EXPECT_NE(s1, s2);
	EXPECT_NE(s1.Hash(), s2.Hash());

	i = 0;
	for (const auto& ch: s1)
	{
		EXPECT_EQ(ch, static_cast<char>(int_c[i++] + 1));
	}
}

void test_move()
{
	String8     str = "123";
	StringList8 list;
	EXPECT_TRUE(!str.IsInvalid());
	list.Add(std::move(str));
	EXPECT_TRUE(str.IsInvalid()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
	str = "234";
	EXPECT_TRUE(!str.IsInvalid());
	EXPECT_EQ(str, "234");
	EXPECT_EQ(list, StringList8({"123"}));
}

TEST(Core, CharString8)
{
	UT_MEM_CHECK_INIT();

	test();
	test_2();
	test_3();
	test_U();
	test_U_2();
	test_U_3();
	test_list();
	test_list_2();
	test_printf();
	test_cpp14();
	test_move();

	UT_MEM_CHECK();
}

UT_END();
