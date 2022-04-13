#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/UnitTest.h"

UT_BEGIN(CoreCharString);

using Core::Char;
using Core::CharProperty;
using Core::StringList;

void test_char()
{
	EXPECT_EQ(sizeof(char32_t), 4U);
	EXPECT_EQ(sizeof(CharProperty), 4U);

	EXPECT_TRUE(Char::IsDecimal(U'1'));
	EXPECT_TRUE(Char::IsDecimal(U'9'));
	EXPECT_TRUE(!Char::IsDecimal(U'g'));
	EXPECT_TRUE(!Char::IsDecimal(U'У'));

	EXPECT_TRUE(!Char::IsAlpha(U'4'));
	EXPECT_TRUE(!Char::IsAlpha(U'0'));
	EXPECT_TRUE(Char::IsAlpha(U'P'));
	EXPECT_TRUE(Char::IsAlpha(U'ы'));

	EXPECT_TRUE(!Char::IsAlphaNum(U'!'));
	EXPECT_TRUE(!Char::IsAlphaNum(U'#'));
	EXPECT_TRUE(Char::IsAlphaNum(U'8'));
	EXPECT_TRUE(Char::IsAlphaNum(U'Ё'));

	EXPECT_TRUE(Char::IsLower(U'r'));
	EXPECT_TRUE(Char::IsLower(U'д'));
	EXPECT_TRUE(!Char::IsLower(U'W'));
	EXPECT_TRUE(!Char::IsLower(U'И'));

	EXPECT_TRUE(Char::IsUpper(U'Q'));
	EXPECT_TRUE(Char::IsUpper(U'Б'));
	EXPECT_TRUE(!Char::IsUpper(U'f'));
	EXPECT_TRUE(!Char::IsUpper(U'ж'));

	EXPECT_TRUE(Char::IsSpace(U' '));
	EXPECT_TRUE(Char::IsSpace(U'\t'));
	EXPECT_TRUE(Char::IsSpace(U'\r'));
	EXPECT_TRUE(Char::IsSpace(U'\n'));
	EXPECT_TRUE(!Char::IsSpace(U'.'));
	EXPECT_TRUE(!Char::IsSpace(U'_'));
	EXPECT_TRUE(!Char::IsSpace(U'5'));
	EXPECT_TRUE(!Char::IsSpace(U'Y'));

	EXPECT_EQ(Char::HexDigit(U'0'), 0);
	EXPECT_EQ(Char::HexDigit(U'9'), 9);
	EXPECT_EQ(Char::HexDigit(U'B'), 11);
	EXPECT_EQ(Char::HexDigit(U'f'), 15);
	EXPECT_EQ(Char::HexDigit(U'g'), -1);
	EXPECT_EQ(Char::HexDigit(U'x'), -1);

	EXPECT_EQ(Char::ToUpper(U'a'), U'A');
	EXPECT_EQ(Char::ToUpper(U'ю'), U'Ю');
	EXPECT_EQ(Char::ToUpper(U';'), U';');
	EXPECT_EQ(Char::ToUpper(U'P'), U'P');

	EXPECT_EQ(Char::ToLower(U'a'), U'a');
	EXPECT_EQ(Char::ToLower(U'T'), U't');
	EXPECT_EQ(Char::ToLower(U'Ц'), U'ц');
	EXPECT_EQ(Char::ToLower(U'?'), U'?');
}

void test()
{
	String e;
	String s = String::FromUtf8("abcdABCDАБВГДабвгд");

	EXPECT_EQ(s.Size(), 18U);
	EXPECT_TRUE(!s.IsEmpty());
	EXPECT_EQ(e.Size(), 0U);
	EXPECT_TRUE(e.IsEmpty());

	e = s;
	s.Clear();

	EXPECT_EQ(s.Size(), 0U);
	EXPECT_TRUE(s.IsEmpty());
	EXPECT_EQ(e.Size(), 18U);
	EXPECT_TRUE(!e.IsEmpty());

	EXPECT_EQ(e[1], U'b');
	EXPECT_EQ(e[9], U'Б');

	s = String::FromUtf8("acdABCАВГДабвгд");
	s = String::FromUtf8("abcdABCDАБВГДабвгд");

	EXPECT_EQ(s[3], U'd');
	EXPECT_EQ(s[11], U'Г');

	s = '0';

	EXPECT_EQ(s.Size(), 1U);

	String::Utf8 utf = String::FromUtf8("abcdABCDАБВГДабвгд").utf8_str();
	EXPECT_EQ(strcmp(utf.GetData(), "abcdABCDАБВГДабвгд"), 0);

	EXPECT_EQ(utf.Size(), 29U);
	EXPECT_EQ(utf.At(0), 0x61);
	EXPECT_EQ(utf.At(4), 0x41);
	EXPECT_EQ(utf.At(16), (char)0xD0);
	EXPECT_EQ(utf.At(17), (char)0x94);

	String::Utf8 cp866 = String::FromUtf8("abcABCАБВабвЭЮЯэюя╫▓").cp866_str();

	EXPECT_EQ(cp866.Size(), 21U);
	EXPECT_EQ(cp866.At(0), 0x61);
	EXPECT_EQ(cp866.At(3), 0x41);
	EXPECT_EQ(cp866.At(9), (char)0xA0);
	EXPECT_EQ(cp866.At(19), (char)0xB2);

	String ss = String::FromUtf8("abcd");
	ss += String::FromUtf8("efgh");
	EXPECT_EQ(strcmp(ss.utf8_str().GetData(), "abcdefgh"), 0);

	String sum1 = String::FromUtf8("ab1") + String::FromUtf8("cd2");
	String sum2 = sum1 + U"cd2" + U"апр";

	EXPECT_TRUE(sum2 == "ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase("AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != "ab1cd2cd2апР");
	EXPECT_TRUE(sum2 == U"ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase(U"AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != U"ab1cd2cd2апР");

	String mt = String::FromUtf8("0123456789");

	EXPECT_TRUE(String::FromUtf8("").IsEmpty());

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

	EXPECT_EQ(String::FromUtf8("abcdабвг%#123").ToUpper(), "ABCDАБВГ%#123");
	EXPECT_EQ(String::FromUtf8("ABCDАБВГ%#123").ToLower(), "abcdабвг%#123");
	EXPECT_EQ(String::FromUtf8("abcdабвг%#123").ToUpper(), U"ABCDАБВГ%#123");
	EXPECT_EQ(String::FromUtf8("ABCDАБВГ%#123").ToLower(), U"abcdабвг%#123");
}

void test_3()
{
	String tt  = String::FromUtf8("  123  abc\r\n");
	String tt2 = String::FromUtf8(" \t  \r\n ");
	String tt3 = String::FromUtf8("no_space");
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

	EXPECT_EQ(String::FromUtf8("й ц у к е н").ReplaceChar(U' ', U'_'), "й_ц_у_к_е_н");
	EXPECT_EQ(String::FromUtf8("й ц у к е н").RemoveChar(U' '), "йцукен");
	EXPECT_EQ(String::FromUtf8("йцУКен").RemoveChar(U'у', String::Case::Insensitive), "йцКен");
	EXPECT_EQ(String::FromUtf8("йцУКен").ReplaceChar(U'к', U'q', String::Case::Insensitive), "йцУqен");
	EXPECT_EQ(String::FromUtf8("й ц у к е н").RemoveAt(1).RemoveAt(2).RemoveAt(3), "йцук е н");
	EXPECT_EQ(String::FromUtf8("йцукен").RemoveAt(0, 0), "йцукен");
	EXPECT_EQ(String::FromUtf8("йцукен").RemoveAt(10, 1), "йцукен");
	EXPECT_EQ(String::FromUtf8("йцукен").RemoveAt(0, 10), "");
	EXPECT_EQ(String::FromUtf8("йцукен").RemoveAt(1, 10), "й");
	EXPECT_EQ(String::FromUtf8("йцукен").RemoveAt(1, 2), "йкен");

	EXPECT_EQ(String::FromUtf8("\nй\nц\n\nу к е н\n").ReplaceStr(U"\n", U"\r\n"), "\r\nй\r\nц\r\n\r\nу к е н\r\n");
	EXPECT_EQ(String::FromUtf8("\nй\nц\n\nу к е н\n").RemoveStr(U"\n"), "йцу к е н");
	EXPECT_EQ(String::FromUtf8("abcabcabc").RemoveStr(U"bc"), "aaa");

	String it = String::FromUtf8("abcЙЦУ123");
	EXPECT_EQ(it.InsertAt(0, U'q'), "qabcЙЦУ123");
	EXPECT_EQ(it.InsertAt(3, U"_d_"), "abc_d_ЙЦУ123");
	EXPECT_EQ(it.InsertAt(10, U"000"), "abcЙЦУ123000");

	String ft = String::FromUtf8("ab ab ab");
	EXPECT_EQ(ft.FindIndex(U"ab", 0), 0U);
	EXPECT_EQ(ft.FindIndex(U"ab", 1), 3U);
	EXPECT_EQ(ft.FindIndex(U"AB", 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"ab", 7), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"ab", 15), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"abc", 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"", 5), 5U);

	EXPECT_EQ(ft.FindLastIndex(U"ab"), 6U);
	EXPECT_EQ(ft.FindLastIndex(U"ab", 6), 6U);
	EXPECT_EQ(ft.FindLastIndex(U"ab", 5), 3U);
	EXPECT_EQ(ft.FindLastIndex(U"AB"), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindLastIndex(U""), 7U);

	EXPECT_EQ(ft.FindIndex(U'a', 0), 0U);
	EXPECT_EQ(ft.FindIndex(U'a', 1), 3U);
	EXPECT_EQ(ft.FindIndex(U'A', 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'a', 7), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'a', 15), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'c', 0), Core::STRING_INVALID_INDEX);

	EXPECT_EQ(ft.FindLastIndex(U'a'), 6U);
	EXPECT_EQ(ft.FindLastIndex(U'a', 6), 6U);
	EXPECT_EQ(ft.FindLastIndex(U'a', 5), 3U);
	EXPECT_EQ(ft.FindLastIndex(U'A'), Core::STRING_INVALID_INDEX);

	String ft2 = String::FromUtf8("ab AB ab");
	EXPECT_EQ(ft2.FindIndex(U"aB", 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindIndex(U"Ab", 1, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U"AB", 3, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U"ab", 4, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"aB", Core::STRING_INVALID_INDEX, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"Ab", 6, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"AB", 5, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindLastIndex(U"ab", 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindLastIndex(U"abc", Core::STRING_INVALID_INDEX, String::Case::Insensitive), Core::STRING_INVALID_INDEX);

	EXPECT_EQ(ft2.FindIndex(U'a', 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindIndex(U'A', 1, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U'A', 3, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U'a', 4, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'a', Core::STRING_INVALID_INDEX, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'A', 6, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'A', 5, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindLastIndex(U'a', 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindLastIndex(U'c', Core::STRING_INVALID_INDEX, String::Case::Insensitive), Core::STRING_INVALID_INDEX);
}

void test_2()
{
	String pf;
	EXPECT_TRUE(pf.Printf("s%s_d%d\n", "abcdАБВГ", 4));
	EXPECT_EQ(pf, "sabcdАБВГ_d4\n");

	EXPECT_TRUE(String::FromUtf8("abcdabcd").StartsWith(U"abc"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").StartsWith(U""));
	EXPECT_TRUE(String::FromUtf8("").StartsWith(U""));
	EXPECT_TRUE(!String::FromUtf8("").StartsWith(U"abc"));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").StartsWith(U"aBc"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").StartsWith(U"aBc", String::Case::Insensitive));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").StartsWith(U'a'));
	EXPECT_TRUE(!String::FromUtf8("").StartsWith(U'a'));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").StartsWith(U'A'));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").StartsWith(U'A', String::Case::Insensitive));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").EndsWith(U"cd"));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").EndsWith(U"bc"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").EndsWith(U"cD", String::Case::Insensitive));
	EXPECT_TRUE(!String::FromUtf8("").EndsWith(U"cd"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").EndsWith(U""));
	EXPECT_TRUE(String::FromUtf8("").EndsWith(U""));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").EndsWith(U'd'));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").EndsWith(U'D'));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").EndsWith(U'D', String::Case::Insensitive));
	EXPECT_TRUE(!String::FromUtf8("").EndsWith(U'd'));

	EXPECT_TRUE(String::FromUtf8("abcdabcd").ContainsStr(U"cda"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").ContainsStr(U""));
	EXPECT_TRUE(!String::FromUtf8("").ContainsStr(U"cda"));
	EXPECT_TRUE(String::FromUtf8("").ContainsStr(U""));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").ContainsStr(U"cDa"));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").ContainsStr(U"cDa", String::Case::Insensitive));

	EXPECT_TRUE(String::FromUtf8("abcd").ContainsChar(U'b'));
	EXPECT_TRUE(!String::FromUtf8("").ContainsChar(U'c'));
	EXPECT_TRUE(!String::FromUtf8("abcdabcd").ContainsChar(U'D'));
	EXPECT_TRUE(String::FromUtf8("abcdabcd").ContainsChar(U'D', String::Case::Insensitive));

	EXPECT_EQ(String::FromUtf8("123456789012345678900").ToInt32(), INT32_MAX);
	EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToInt32(), INT32_MIN);
	EXPECT_EQ(String::FromUtf8("123456789012345678900").ToUint32(), UINT32_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToUint32(), UINT32_MAX);
#else
	// EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToUint32(), (~UINT32_MAX) + 1);
#endif
	EXPECT_EQ(String::FromUtf8("123456789012345678900").ToInt64(), INT64_MAX);
	EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToInt64(), INT64_MIN);
	EXPECT_EQ(String::FromUtf8("123456789012345678900").ToUint64(), UINT64_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToUint64(), UINT64_MAX);
#else
	// EXPECT_EQ(String::FromUtf8("-123456789012345678900").ToUint64(), (~UINT64_MAX) + 1);
#endif
	EXPECT_EQ(String::FromUtf8("0xabcd").ToUint32(16), 0xabcdU);
	EXPECT_EQ(String::FromUtf8("-0.345").ToDouble(), -0.345);

	EXPECT_EQ(String::FromUtf8("abcd/dede/dcdc").DirectoryWithoutFilename(), String::FromUtf8("abcd/dede/"));
	EXPECT_EQ(String::FromUtf8("abcd/dede/").DirectoryWithoutFilename(), String::FromUtf8("abcd/dede/"));
	EXPECT_EQ(String::FromUtf8("/abcd/dede/dcdc").DirectoryWithoutFilename(), String::FromUtf8("/abcd/dede/"));
	EXPECT_EQ(String::FromUtf8("abcddededcdc").DirectoryWithoutFilename(), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("abcd/dede/dcdc").FilenameWithoutDirectory(), String::FromUtf8("dcdc"));
	EXPECT_EQ(String::FromUtf8("abcd/dede/").FilenameWithoutDirectory(), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("/abcd/dede/dcdc").FilenameWithoutDirectory(), String::FromUtf8("dcdc"));
	EXPECT_EQ(String::FromUtf8("abcddededcdc").FilenameWithoutDirectory(), String::FromUtf8("abcddededcdc"));

	EXPECT_EQ(String::FromUtf8("abcdef").RemoveLast(0), String::FromUtf8("abcdef"));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveLast(1), String::FromUtf8("abcde"));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveLast(6), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveLast(7), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveFirst(0), String::FromUtf8("abcdef"));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveFirst(1), String::FromUtf8("bcdef"));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveFirst(6), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("abcdef").RemoveFirst(7), String::FromUtf8(""));

	EXPECT_EQ(String::FromUtf8("abcd.ext").FilenameWithoutExtension(), String::FromUtf8("abcd"));
	EXPECT_EQ(String::FromUtf8("abcd").FilenameWithoutExtension(), String::FromUtf8("abcd"));
	EXPECT_EQ(String::FromUtf8("abcd.").FilenameWithoutExtension(), String::FromUtf8("abcd"));
	EXPECT_EQ(String::FromUtf8(".ext").FilenameWithoutExtension(), String::FromUtf8(""));

	EXPECT_EQ(String::FromUtf8("abcd.ext").ExtensionWithoutFilename(), String::FromUtf8(".ext"));
	EXPECT_EQ(String::FromUtf8("abcd").ExtensionWithoutFilename(), String::FromUtf8(""));
	EXPECT_EQ(String::FromUtf8("abcd.").ExtensionWithoutFilename(), String::FromUtf8("."));
	EXPECT_EQ(String::FromUtf8(".ext").ExtensionWithoutFilename(), String::FromUtf8(".ext"));

	EXPECT_TRUE(String::FromUtf8("abcd").EqualAscii("abcd"));
	EXPECT_TRUE(!String::FromUtf8("abc").EqualAscii("abcd"));
	EXPECT_TRUE(!String::FromUtf8("abcd").EqualAscii("abc"));

	EXPECT_TRUE(Char::EqualAscii(String::FromUtf8("abcd").GetDataConst(), "abcd"));
	EXPECT_TRUE(!Char::EqualAscii(String::FromUtf8("abc").GetDataConst(), "abcd"));
	EXPECT_TRUE(!Char::EqualAscii(String::FromUtf8("abcd").GetDataConst(), "abc"));
	EXPECT_TRUE(Char::EqualAsciiN(String::FromUtf8("abcd").GetDataConst(), "abcd", 4));
	EXPECT_TRUE(Char::EqualAsciiN(String::FromUtf8("abcd").GetDataConst(), "abcd", 3));
	EXPECT_TRUE(!Char::EqualAsciiN(String::FromUtf8("abc").GetDataConst(), "abcd", 4));
	EXPECT_TRUE(Char::EqualAsciiN(String::FromUtf8("abc").GetDataConst(), "abcd", 3));
	EXPECT_TRUE(Char::EqualAsciiN(String::FromUtf8("abcd").GetDataConst(), "abc", 3));

	String s = String::FromUtf8("test");
	String s2(s.GetDataConst() + 2);
	EXPECT_EQ(s2, "st");

	String word = U"ebadc";
	EXPECT_EQ(word.SortChars(), U"abcde");

	EXPECT_EQ(String::FromUtf8("abc123abcd", 0), U"");
	EXPECT_EQ(String::FromUtf8("abc123abcd", 1), U"a");
	EXPECT_EQ(String::FromUtf8("abc123abcd", 4), U"abc1");
}

void test_U()
{
	String e;
	String s = U"abcdABCDАБВГДабвгд";

	EXPECT_EQ(s.Size(), 18U);
	EXPECT_TRUE(!s.IsEmpty());
	EXPECT_EQ(e.Size(), 0U);
	EXPECT_TRUE(e.IsEmpty());

	e = s;
	s.Clear();

	EXPECT_EQ(s.Size(), 0U);
	EXPECT_TRUE(s.IsEmpty());
	EXPECT_EQ(e.Size(), 18U);
	EXPECT_TRUE(!e.IsEmpty());

	EXPECT_EQ(e[1], U'b');
	EXPECT_EQ(e[9], U'Б');

	s = U"acdABCАВГДабвгд";
	s = U"abcdABCDАБВГДабвгд";

	EXPECT_EQ(s[3], U'd');
	EXPECT_EQ(s[11], U'Г');

	s = U'0';

	EXPECT_EQ(s.Size(), 1U);

	String::Utf8 utf = String(U"abcdABCDАБВГДабвгд").utf8_str();
	EXPECT_EQ(strcmp(utf.GetData(), "abcdABCDАБВГДабвгд"), 0);

	EXPECT_EQ(utf.Size(), 29U);
	EXPECT_EQ(utf.At(0), 0x61);
	EXPECT_EQ(utf.At(4), 0x41);
	EXPECT_EQ(utf.At(16), (char)0xD0);
	EXPECT_EQ(utf.At(17), (char)0x94);

	String::Utf8 cp866 = String(U"abcABCАБВабвЭЮЯэюя╫▓").cp866_str();

	EXPECT_EQ(cp866.Size(), 21U);
	EXPECT_EQ(cp866.At(0), 0x61);
	EXPECT_EQ(cp866.At(3), 0x41);
	EXPECT_EQ(cp866.At(9), (char)0xA0);
	EXPECT_EQ(cp866.At(19), (char)0xB2);

	String ss = U"abcd";
	ss += U"efgh";
	EXPECT_EQ(strcmp(ss.utf8_str().GetData(), "abcdefgh"), 0);

	ss = "abcd";
	ss += "efgh2";
	EXPECT_EQ(strcmp(ss.utf8_str().GetData(), "abcdefgh2"), 0);

	String sum1 = U"ab1" + String(U"cd2");
	String sum2 = sum1 + U"cd2" + U"апр";

	EXPECT_TRUE(sum2 == "ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase("AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != "ab1cd2cd2апР");
	EXPECT_TRUE(sum2 == U"ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase(U"AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != U"ab1cd2cd2апР");

	sum1 = "_ab1" + String(U"cd2");
	sum2 = sum1 + "cd2" + "апр";

	EXPECT_TRUE(sum2 == "_ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase("_AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != "_ab1cd2cd2апР");
	EXPECT_TRUE(sum2 == U"_ab1cd2cd2апр");
	EXPECT_TRUE(sum2.EqualNoCase(U"_AB1CD2CD2АПР"));
	EXPECT_TRUE(sum2 != U"_ab1cd2cd2апР");

	String mt = U"0123456789";

	EXPECT_TRUE(String(U"").IsEmpty());

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

	EXPECT_EQ(String(U"abcdабвг%#123").ToUpper(), "ABCDАБВГ%#123");
	EXPECT_EQ(String(U"ABCDАБВГ%#123").ToLower(), "abcdабвг%#123");
}

void test_U_3()
{
	String tt  = U"  123  abc\r\n";
	String tt2 = U" \t  \r\n ";
	String tt3 = U"no_space";
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

	EXPECT_EQ(String(U"й ц у к е н").ReplaceChar(U' ', U'_'), "й_ц_у_к_е_н");
	EXPECT_EQ(String(U"й ц у к е н").RemoveChar(U' '), "йцукен");
	EXPECT_EQ(String(U"йцУКен").RemoveChar(U'у', String::Case::Insensitive), "йцКен");
	EXPECT_EQ(String(U"йцУКен").ReplaceChar(U'к', U'q', String::Case::Insensitive), "йцУqен");
	EXPECT_EQ(String(U"й ц у к е н").RemoveAt(1).RemoveAt(2).RemoveAt(3), "йцук е н");
	EXPECT_EQ(String(U"йцукен").RemoveAt(0, 0), "йцукен");
	EXPECT_EQ(String(U"йцукен").RemoveAt(10, 1), "йцукен");
	EXPECT_EQ(String(U"йцукен").RemoveAt(0, 10), "");
	EXPECT_EQ(String(U"йцукен").RemoveAt(1, 10), "й");
	EXPECT_EQ(String(U"йцукен").RemoveAt(1, 2), "йкен");

	EXPECT_EQ(String(U"\nй\nц\n\nу к е н\n").ReplaceStr(U"\n", U"\r\n"), "\r\nй\r\nц\r\n\r\nу к е н\r\n");
	EXPECT_EQ(String(U"\nй\nц\n\nу к е н\n").RemoveStr(U"\n"), "йцу к е н");
	EXPECT_EQ(String(U"abcabcabc").RemoveStr(U"bc"), "aaa");

	String it = U"abcЙЦУ123";
	EXPECT_EQ(it.InsertAt(0, U'q'), "qabcЙЦУ123");
	EXPECT_EQ(it.InsertAt(3, U"_d_"), "abc_d_ЙЦУ123");
	EXPECT_EQ(it.InsertAt(10, U"000"), "abcЙЦУ123000");

	String ft = U"ab ab ab";
	EXPECT_EQ(ft.FindIndex(U"ab", 0), 0U);
	EXPECT_EQ(ft.FindIndex(U"ab", 1), 3U);
	EXPECT_EQ(ft.FindIndex(U"AB", 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"ab", 7), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"ab", 15), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"abc", 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U"", 5), 5U);

	EXPECT_EQ(ft.FindLastIndex(U"ab"), 6U);
	EXPECT_EQ(ft.FindLastIndex(U"ab", 6), 6U);
	EXPECT_EQ(ft.FindLastIndex(U"ab", 5), 3U);
	EXPECT_EQ(ft.FindLastIndex(U"AB"), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindLastIndex(U""), 7U);

	EXPECT_EQ(ft.FindIndex(U'a', 0), 0U);
	EXPECT_EQ(ft.FindIndex(U'a', 1), 3U);
	EXPECT_EQ(ft.FindIndex(U'A', 0), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'a', 7), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'a', 15), Core::STRING_INVALID_INDEX);
	EXPECT_EQ(ft.FindIndex(U'c', 0), Core::STRING_INVALID_INDEX);

	EXPECT_EQ(ft.FindLastIndex(U'a'), 6U);
	EXPECT_EQ(ft.FindLastIndex(U'a', 6), 6U);
	EXPECT_EQ(ft.FindLastIndex(U'a', 5), 3U);
	EXPECT_EQ(ft.FindLastIndex(U'A'), Core::STRING_INVALID_INDEX);

	String ft2 = U"ab AB ab";
	EXPECT_EQ(ft2.FindIndex(U"aB", 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindIndex(U"Ab", 1, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U"AB", 3, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U"ab", 4, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"aB", Core::STRING_INVALID_INDEX, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"Ab", 6, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U"AB", 5, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindLastIndex(U"ab", 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindLastIndex(U"abc", Core::STRING_INVALID_INDEX, String::Case::Insensitive), Core::STRING_INVALID_INDEX);

	EXPECT_EQ(ft2.FindIndex(U'a', 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindIndex(U'A', 1, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U'A', 3, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindIndex(U'a', 4, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'a', Core::STRING_INVALID_INDEX, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'A', 6, String::Case::Insensitive), 6U);
	EXPECT_EQ(ft2.FindLastIndex(U'A', 5, String::Case::Insensitive), 3U);
	EXPECT_EQ(ft2.FindLastIndex(U'a', 0, String::Case::Insensitive), 0U);
	EXPECT_EQ(ft2.FindLastIndex(U'c', Core::STRING_INVALID_INDEX, String::Case::Insensitive), Core::STRING_INVALID_INDEX);
}

void test_U_2()
{
	String pf;
	EXPECT_TRUE(pf.Printf("s%s_d%d\n", "abcdАБВГ", 4));
	EXPECT_EQ(pf, "sabcdАБВГ_d4\n");

	EXPECT_TRUE(String(U"abcdabcd").StartsWith(U"abc"));
	EXPECT_TRUE(String(U"abcdabcd").StartsWith(U""));
	EXPECT_TRUE(String(U"").StartsWith(U""));
	EXPECT_TRUE(!String(U"").StartsWith(U"abc"));
	EXPECT_TRUE(!String(U"abcdabcd").StartsWith(U"aBc"));
	EXPECT_TRUE(String(U"abcdabcd").StartsWith(U"aBc", String::Case::Insensitive));
	EXPECT_TRUE(String(U"abcdabcd").StartsWith(U'a'));
	EXPECT_TRUE(!String(U"").StartsWith(U'a'));
	EXPECT_TRUE(!String(U"abcdabcd").StartsWith(U'A'));
	EXPECT_TRUE(String(U"abcdabcd").StartsWith(U'A', String::Case::Insensitive));
	EXPECT_TRUE(String(U"abcdabcd").EndsWith(U"cd"));
	EXPECT_TRUE(!String(U"abcdabcd").EndsWith(U"bc"));
	EXPECT_TRUE(String(U"abcdabcd").EndsWith(U"cD", String::Case::Insensitive));
	EXPECT_TRUE(!String(U"").EndsWith(U"cd"));
	EXPECT_TRUE(String(U"abcdabcd").EndsWith(U""));
	EXPECT_TRUE(String(U"").EndsWith(U""));
	EXPECT_TRUE(String(U"abcdabcd").EndsWith(U'd'));
	EXPECT_TRUE(!String(U"abcdabcd").EndsWith(U'D'));
	EXPECT_TRUE(String(U"abcdabcd").EndsWith(U'D', String::Case::Insensitive));
	EXPECT_TRUE(!String(U"").EndsWith(U'd'));

	EXPECT_TRUE(String(U"abcdabcd").ContainsStr(U"cda"));
	EXPECT_TRUE(String(U"abcdabcd").ContainsStr(U""));
	EXPECT_TRUE(!String(U"").ContainsStr(U"cda"));
	EXPECT_TRUE(String(U"").ContainsStr(U""));
	EXPECT_TRUE(!String(U"abcdabcd").ContainsStr(U"cDa"));
	EXPECT_TRUE(String(U"abcdabcd").ContainsStr(U"cDa", String::Case::Insensitive));

	EXPECT_TRUE(String(U"abcd").ContainsChar(U'b'));
	EXPECT_TRUE(!String(U"").ContainsChar(U'c'));
	EXPECT_TRUE(!String(U"abcdabcd").ContainsChar(U'D'));
	EXPECT_TRUE(String(U"abcdabcd").ContainsChar(U'D', String::Case::Insensitive));

	EXPECT_EQ(String(U"123456789012345678900").ToInt32(), INT32_MAX);
	EXPECT_EQ(String(U"-123456789012345678900").ToInt32(), INT32_MIN);
	EXPECT_EQ(String(U"123456789012345678900").ToUint32(), UINT32_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String(U"-123456789012345678900").ToUint32(), UINT32_MAX);
#else
	// EXPECT_EQ(String(U"-123456789012345678900").ToUint32(), (~UINT32_MAX) + 1);
#endif
	EXPECT_EQ(String(U"123456789012345678900").ToInt64(), INT64_MAX);
	EXPECT_EQ(String(U"-123456789012345678900").ToInt64(), INT64_MIN);
	EXPECT_EQ(String(U"123456789012345678900").ToUint64(), UINT64_MAX);
#if FC_PLATFORM == FC_PLATFORM_ANDROID || FC_COMPILER == FC_COMPILER_MSVC
	// EXPECT_EQ(String(U"-123456789012345678900").ToUint64(), UINT64_MAX);
#else
	// EXPECT_EQ(String(U"-123456789012345678900").ToUint64(), (~UINT64_MAX) + 1);
#endif
	EXPECT_EQ(String(U"0xabcd").ToUint32(16), 0xabcdU);
	EXPECT_EQ(String(U"-0.345").ToDouble(), -0.345);

	EXPECT_EQ(String(U"abcd/dede/dcdc").DirectoryWithoutFilename(), String(U"abcd/dede/"));
	EXPECT_EQ(String(U"abcd/dede/").DirectoryWithoutFilename(), String(U"abcd/dede/"));
	EXPECT_EQ(String(U"/abcd/dede/dcdc").DirectoryWithoutFilename(), String(U"/abcd/dede/"));
	EXPECT_EQ(String(U"abcddededcdc").DirectoryWithoutFilename(), String(U""));
	EXPECT_EQ(String(U"abcd/dede/dcdc").FilenameWithoutDirectory(), String(U"dcdc"));
	EXPECT_EQ(String(U"abcd/dede/").FilenameWithoutDirectory(), String(U""));
	EXPECT_EQ(String(U"/abcd/dede/dcdc").FilenameWithoutDirectory(), String(U"dcdc"));
	EXPECT_EQ(String(U"abcddededcdc").FilenameWithoutDirectory(), String(U"abcddededcdc"));

	EXPECT_EQ(String(U"abcdef").RemoveLast(0), String(U"abcdef"));
	EXPECT_EQ(String(U"abcdef").RemoveLast(1), String(U"abcde"));
	EXPECT_EQ(String(U"abcdef").RemoveLast(6), String(U""));
	EXPECT_EQ(String(U"abcdef").RemoveLast(7), String(U""));
	EXPECT_EQ(String(U"abcdef").RemoveFirst(0), String(U"abcdef"));
	EXPECT_EQ(String(U"abcdef").RemoveFirst(1), String(U"bcdef"));
	EXPECT_EQ(String(U"abcdef").RemoveFirst(6), String(U""));
	EXPECT_EQ(String(U"abcdef").RemoveFirst(7), String(U""));

	EXPECT_EQ(String(U"abcd.ext").FilenameWithoutExtension(), String(U"abcd"));
	EXPECT_EQ(String(U"abcd").FilenameWithoutExtension(), String(U"abcd"));
	EXPECT_EQ(String(U"abcd.").FilenameWithoutExtension(), String(U"abcd"));
	EXPECT_EQ(String(U".ext").FilenameWithoutExtension(), String(U""));

	EXPECT_EQ(String(U"abcd.ext").ExtensionWithoutFilename(), String(U".ext"));
	EXPECT_EQ(String(U"abcd").ExtensionWithoutFilename(), String(U""));
	EXPECT_EQ(String(U"abcd.").ExtensionWithoutFilename(), String(U"."));
	EXPECT_EQ(String(U".ext").ExtensionWithoutFilename(), String(U".ext"));

	EXPECT_TRUE(String(U"abcd").EqualAscii("abcd"));
	EXPECT_TRUE(!String(U"abc").EqualAscii("abcd"));
	EXPECT_TRUE(!String(U"abcd").EqualAscii("abc"));

	EXPECT_TRUE(Char::EqualAscii(String(U"abcd").GetDataConst(), "abcd"));
	EXPECT_TRUE(!Char::EqualAscii(String(U"abc").GetDataConst(), "abcd"));
	EXPECT_TRUE(!Char::EqualAscii(String(U"abcd").GetDataConst(), "abc"));
	EXPECT_TRUE(Char::EqualAsciiN(String(U"abcd").GetDataConst(), "abcd", 4));
	EXPECT_TRUE(Char::EqualAsciiN(String(U"abcd").GetDataConst(), "abcd", 3));
	EXPECT_TRUE(!Char::EqualAsciiN(String(U"abc").GetDataConst(), "abcd", 4));
	EXPECT_TRUE(Char::EqualAsciiN(String(U"abc").GetDataConst(), "abcd", 3));
	EXPECT_TRUE(Char::EqualAsciiN(String(U"abcd").GetDataConst(), "abc", 3));

	String s = U"test";
	String s2(s.GetDataConst() + 2);
	EXPECT_EQ(s2, "st");
}

void test_list()
{
	StringList list = String(U" a b  У р e ").Split(U" ");

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String(U",a,b,,У,р,e,").Split(U",", String::SplitType::WithEmptyParts);

	EXPECT_EQ(list[0], "");
	EXPECT_EQ(list[1], "a");
	EXPECT_EQ(list[2], "b");
	EXPECT_EQ(list[3], "");
	EXPECT_EQ(list[4], "У");
	EXPECT_EQ(list[5], "р");
	EXPECT_EQ(list[6], "e");
	EXPECT_EQ(list[7], "");

	list = String(U"qaQbqqУQрqeQ").Split(U"q", String::SplitType::SplitNoEmptyParts, String::Case::Insensitive);

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String(U"qaQbqqУQрqeQ").Split(U"q", String::SplitType::SplitNoEmptyParts, String::Case::Sensitive);

	EXPECT_EQ(list[0], "aQb");
	EXPECT_EQ(list[1], "УQр");
	EXPECT_EQ(list[2], "eQ");

	StringList list2 = list;

	list[0] = String::FromUtf8("a");
	list[1] = U"b";
	list[2] = U"c";

	EXPECT_EQ(list2[0], "aQb");
	EXPECT_EQ(list2[1], "УQр");
	EXPECT_EQ(list2[2], "eQ");

	EXPECT_TRUE(list.Contains(U"b"));
	EXPECT_TRUE(!list.Contains(U"B"));
	EXPECT_TRUE(list.Contains(U"B", String::Case::Insensitive));

	EXPECT_TRUE(String(U"fdabn").ContainsAnyStr(list));
	EXPECT_TRUE(!String(U"Cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String(U"cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String(U"Cfdabn").ContainsAllStr(list, String::Case::Insensitive));

	EXPECT_EQ(list.Concat(U", "), U"a, b, c");

	auto chars = list.Concat(U"");

	EXPECT_TRUE(String(U"fdabn").ContainsAnyChar(chars));
	EXPECT_TRUE(!String(U"Cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String(U"cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String(U"Cfdabn").ContainsAllChar(chars, String::Case::Insensitive));

	StringList l1 = String(U"a b c d e f").Split(U" ");
	StringList l2 = String(U"a b c d e f").Split(U" ");
	EXPECT_TRUE(l1.Equal(l2));
	EXPECT_TRUE(l1 == l2);
	l1.Add(U"Q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	l2.Add(U"q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	EXPECT_TRUE(l1.EqualNoCase(l2));
}

void test_list_2()
{
	StringList list = String(U" a b  У р e ").Split(U' ');

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String(U",a,b,,У,р,e,").Split(U',', String::SplitType::WithEmptyParts);

	EXPECT_EQ(list[0], "");
	EXPECT_EQ(list[1], "a");
	EXPECT_EQ(list[2], "b");
	EXPECT_EQ(list[3], "");
	EXPECT_EQ(list[4], "У");
	EXPECT_EQ(list[5], "р");
	EXPECT_EQ(list[6], "e");
	EXPECT_EQ(list[7], "");

	list = String(U"qaQbqqУQрqeQ").Split(U'q', String::SplitType::SplitNoEmptyParts, String::Case::Insensitive);

	EXPECT_EQ(list[0], "a");
	EXPECT_EQ(list[1], "b");
	EXPECT_EQ(list[2], "У");
	EXPECT_EQ(list[3], "р");
	EXPECT_EQ(list[4], "e");

	list = String(U"qaQbqqУQрqeQ").Split(U'q', String::SplitType::SplitNoEmptyParts, String::Case::Sensitive);

	EXPECT_EQ(list[0], "aQb");
	EXPECT_EQ(list[1], "УQр");
	EXPECT_EQ(list[2], "eQ");

	StringList list2 = list;

	list[0] = String::FromUtf8("a");
	list[1] = U"b";
	list[2] = U"c";

	EXPECT_EQ(list2[0], "aQb");
	EXPECT_EQ(list2[1], "УQр");
	EXPECT_EQ(list2[2], "eQ");

	EXPECT_TRUE(list.Contains(U"b"));
	EXPECT_TRUE(!list.Contains(U"B"));
	EXPECT_TRUE(list.Contains(U"B", String::Case::Insensitive));

	EXPECT_TRUE(String(U"fdabn").ContainsAnyStr(list));
	EXPECT_TRUE(!String(U"Cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String(U"cfdabn").ContainsAllStr(list));
	EXPECT_TRUE(String(U"Cfdabn").ContainsAllStr(list, String::Case::Insensitive));

	EXPECT_EQ(list.Concat(U','), U"a,b,c");

	auto chars = list.Concat(U"");

	EXPECT_TRUE(String(U"fdabn").ContainsAnyChar(chars));
	EXPECT_TRUE(!String(U"Cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String(U"cfdabn").ContainsAllChar(chars));
	EXPECT_TRUE(String(U"Cfdabn").ContainsAllChar(chars, String::Case::Insensitive));

	StringList l1 = String(U"a b c d e f").Split(U' ');
	StringList l2 = String(U"a b c d e f").Split(U' ');
	EXPECT_TRUE(l1.Equal(l2));
	EXPECT_TRUE(l1 == l2);
	l1.Add(U"Q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	l2.Add(U"q");
	EXPECT_TRUE(!l1.Equal(l2));
	EXPECT_TRUE(l1 != l2);
	EXPECT_TRUE(l1.EqualNoCase(l2));
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

	String s;
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

void test_hex()
{
	String s = U"00112233445566778899AABBCCDDEEFF";

	// printf("%s\n", bin_to_hex(hex_to_bin(s)).C_Str());

	EXPECT_EQ(s, String::HexFromBin(s.HexToBin()));

	s = U"24FC79CCBF0979E9371AC23C6D68DE36";

	// printf("%s\n", bin_to_hex(hex_to_bin(s)).C_Str());

	EXPECT_EQ(s, String::HexFromBin(s.HexToBin()));
}

void text_utf16()
{
	String s(U"abcdЙцукен123");

	EXPECT_EQ(String::FromUtf16(s.utf16_str().GetData()), s);
}

void text_cp866()
{
	String s(U"abcdЙцукен123");

	EXPECT_EQ(String::FromCp866(s.cp866_str().GetData()), s);
}

void text_cp1251()
{
	String s(U"abcdЙцукен123");

	EXPECT_EQ(String::FromCp1251(s.cp1251_str().GetData()), s);
}

void test_cpp14()
{
	StringList s = {U"a", U"b", U"cd"};

	EXPECT_EQ(s.Size(), 3U);
	EXPECT_EQ(s.At(0), "a");
	EXPECT_EQ(s.At(1), "b");
	EXPECT_EQ(s.At(2), "cd");

	String ss;
	for (const auto& str: s)
	{
		ss += str;
	}

	EXPECT_EQ(ss, U"abcd");

	uint32_t int_c[] = {0xEE, 0xD0B0, 0x20FFF};
	String   s1      = String::FromUtf8("î킰𠿿");
	String   s2      = U"î킰𠿿";
	String   s3      = String::FromUtf16(u"î킰𠿿");

	int i = 0;
	i     = 0;
	for (auto ch: s1)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (auto& ch: s1)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (const auto& ch: s1)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (auto ch: s2)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (auto& ch: s2)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (const auto& ch: s2)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (auto ch: s3)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (auto& ch: s3)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}
	i = 0;
	for (const auto& ch: s3)
	{
		EXPECT_EQ(ch, int_c[i++]);
	}

	EXPECT_EQ(s1, s2);
	EXPECT_EQ(s2, s3);

	EXPECT_EQ(s1.Hash(), s2.Hash());
	EXPECT_EQ(s2.Hash(), s3.Hash());

	for (auto& ch: s1)
	{
		ch++;
	}

	EXPECT_NE(s1, s2);
	EXPECT_NE(s1.Hash(), s2.Hash());

	i = 0;
	for (const auto& ch: s1)
	{
		EXPECT_EQ(ch, int_c[i++] + 1);
	}
}

void test_move()
{
	String     str = U"123";
	StringList list;
	EXPECT_TRUE(!str.IsInvalid());
	list.Add(std::move(str));
	EXPECT_TRUE(str.IsInvalid()); // NOLINT(hicpp-invalid-access-moved,bugprone-use-after-move)
	str = U"234";
	EXPECT_TRUE(!str.IsInvalid());
	EXPECT_EQ(str, U"234");
	EXPECT_EQ(list, StringList({U"123"}));
}

TEST(Core, CharString)
{
	UT_MEM_CHECK_INIT();

	test_char();
	test();
	test_2();
	test_3();
	test_U();
	test_U_2();
	test_U_3();
	test_list();
	test_list_2();
	test_printf();
	test_hex();
	text_utf16();
	text_cp866();
	text_cp1251();
	test_cpp14();
	test_move();

	UT_MEM_CHECK();
}

UT_END();
