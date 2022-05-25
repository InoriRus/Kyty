#ifndef CORE_KYTYSTRING_H_
#define CORE_KYTYSTRING_H_

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/SimpleArray.h" // IWYU pragma: export
#include "Kyty/Core/Vector.h"

#include <cstdarg> // IWYU pragma: export

namespace Kyty::Core {

constexpr uint32_t STRING_INVALID_INDEX = static_cast<uint32_t>(-1);

class StringList;

#define C_Str   utf8_str().GetData   /* NOLINT(cppcoreguidelines-macro-usage) */
#define Ru_Str  cp866_str().GetData  /* NOLINT(cppcoreguidelines-macro-usage) */
#define Win_Str cp1251_str().GetData /* NOLINT(cppcoreguidelines-macro-usage) */

struct CharProperty
{
	int decimal     : 1;
	int alpha       : 1;
	int lower       : 1;
	int upper       : 1;
	int space       : 1;
	int hex         : 1;
	int hex_data    : 5;
	int case_offset : 21;
};

class Char
{
public:
	static int ToCp866(char32_t src, uint8_t* dst);
	static int ToCp1251(char32_t src, uint8_t* dst);
	static int ToUtf8(char32_t src, uint8_t* dst);
	static int ToUtf16(char32_t src, char16_t* dst);
	static int ToUtf32(char32_t src, char32_t* dst);

	static bool     IsDecimal(char32_t ucs4);
	static bool     IsAlpha(char32_t ucs4);
	static bool     IsAlphaNum(char32_t ucs4);
	static bool     IsLower(char32_t ucs4);
	static bool     IsUpper(char32_t ucs4);
	static bool     IsSpace(char32_t ucs4);
	static bool     IsHex(char32_t ucs4);
	static int      HexDigit(char32_t ucs4);
	static int      DecimalDigit(char32_t ucs4);
	static char32_t ToLower(char32_t ucs4);
	static char32_t ToUpper(char32_t ucs4);

	static char32_t ReadCp866(const uint8_t** str);
	static char32_t ReadCp1251(const uint8_t** str);
	static char32_t ReadUtf8(const uint8_t** str);
	static char32_t ReadUtf16(const char16_t** str);
	static char32_t ReadUtf32(const char32_t** str);

	static bool EqualAscii(const char32_t* utf32_str, const char* ascii_str);
	static bool EqualAsciiNoCase(const char32_t* utf32_str, const char* ascii_str);
	static bool EqualAsciiN(const char32_t* utf32_str, const char* ascii_str, int n);
	static bool EqualAsciiNoCaseN(const char32_t* utf32_str, const char* ascii_str, int n);
};

class String
{
public:
	using DataType = SimpleArray<char32_t>;
	using Utf8     = Vector<char>;
	using Cp866    = Vector<char>;
	using Cp1251   = Vector<char>;
	using Utf16    = Vector<char16_t>;
	using Utf32    = Vector<char32_t>;

	enum class Case
	{
		Insensitive = 0,
		Sensitive   = 1
	};
	enum class SplitType
	{
		WithEmptyParts,
		SplitNoEmptyParts
	};

	String();
	String(const String& src);
	String(String&& src) noexcept;
	String(char32_t ch, uint32_t repeat = 1); // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
	String(const char32_t* str);              // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
	explicit String(const Utf8& utf8);
	virtual ~String();

	[[nodiscard]] uint32_t Size() const;
	[[nodiscard]] bool     IsEmpty() const;
	[[nodiscard]] bool     IsInvalid() const;

	void Clear();

	char32_t&                     operator[](uint32_t index);
	const char32_t&               operator[](uint32_t index) const;
	[[nodiscard]] const char32_t& At(uint32_t index) const;
	char32_t*                     GetData();
	[[nodiscard]] const char32_t* GetData() const;
	[[nodiscard]] const char32_t* GetDataConst() const;

	[[nodiscard]] Utf8   utf8_str() const;   // NOLINT(readability-identifier-naming)
	[[nodiscard]] Utf16  utf16_str() const;  // NOLINT(readability-identifier-naming)
	[[nodiscard]] Utf32  utf32_str() const;  // NOLINT(readability-identifier-naming)
	[[nodiscard]] Cp866  cp866_str() const;  // NOLINT(readability-identifier-naming)
	[[nodiscard]] Cp1251 cp1251_str() const; // NOLINT(readability-identifier-naming)

	String& operator=(const String& src);
	String& operator=(String&& src) noexcept;
	String& operator=(char32_t ch);
	String& operator=(const char32_t* utf32_str);
	String& operator=(const char* utf8_str);
	String& operator=(const char16_t* utf16_str);
	String& operator+=(const String& src);
	String& operator+=(char32_t ch);
	String& operator+=(const char32_t* utf32_str);
	String& operator+=(const char* utf8_str);
	String& operator+=(const char16_t* utf16_str);

	friend String operator+(const String& str1, const String& str2);
	friend String operator+(const char* utf8_str1, const String& str2);
	friend String operator+(const String& str1, const char* utf8_str2);
	friend String operator+(const char32_t* utf32_str1, const String& str2);
	friend String operator+(const String& str1, const char32_t* utf32_str2);
	friend String operator+(char32_t ch, const String& str2);
	friend String operator+(const String& str1, char32_t ch);

	[[nodiscard]] bool Equal(const String& src) const;
	[[nodiscard]] bool Equal(char32_t ch) const;
	bool               Equal(const char32_t* utf32_str) const;
	bool               Equal(const char* utf8_str) const;
	[[nodiscard]] bool EqualNoCase(const String& src) const;
	[[nodiscard]] bool EqualNoCase(char32_t ch) const;
	bool               EqualNoCase(const char32_t* utf32_str) const;
	bool               EqualNoCase(const char* utf8_str) const;

	friend bool operator==(const String& str1, const String& str2) { return str2.Equal(str1); }
	friend bool operator==(const char* utf8_str1, const String& str2) { return str2.Equal(utf8_str1); }
	friend bool operator==(const String& str1, const char* utf8_str2) { return str1.Equal(utf8_str2); }
	friend bool operator==(const char32_t* utf32_str1, const String& str2) { return str2.Equal(utf32_str1); }
	friend bool operator==(const String& str1, const char32_t* utf32_str2) { return str1.Equal(utf32_str2); }
	friend bool operator==(char32_t ch, const String& str2) { return str2.Equal(ch); }
	friend bool operator==(const String& str1, char32_t ch) { return str1.Equal(ch); }
	friend bool operator!=(const String& str1, const String& str2) { return !str2.Equal(str1); }
	friend bool operator!=(const char* utf8_str1, const String& str2) { return !str2.Equal(utf8_str1); }
	friend bool operator!=(const String& str1, const char* utf8_str2) { return !str1.Equal(utf8_str2); }
	friend bool operator!=(const char32_t* utf32_str1, const String& str2) { return !str2.Equal(utf32_str1); }
	friend bool operator!=(const String& str1, const char32_t* utf32_str2) { return !str1.Equal(utf32_str2); }
	friend bool operator!=(char32_t ch, const String& str2) { return !str2.Equal(ch); }
	friend bool operator!=(const String& str1, char32_t ch) { return !str1.Equal(ch); }

	[[nodiscard]] String Mid(uint32_t first, uint32_t count) const;
	[[nodiscard]] String Mid(uint32_t first) const;
	[[nodiscard]] String Left(uint32_t count) const;
	[[nodiscard]] String Right(uint32_t count) const;

	[[nodiscard]] String ToUpper() const;
	[[nodiscard]] String ToLower() const;

	[[nodiscard]] String TrimRight() const;
	[[nodiscard]] String TrimLeft() const;
	[[nodiscard]] String Trim() const;
	[[nodiscard]] String Simplify() const;

	[[nodiscard]] String ReplaceChar(char32_t old_char, char32_t new_char, Case cs = Case::Sensitive) const;
	[[nodiscard]] String ReplaceStr(const String& old_str, const String& new_str, Case cs = Case::Sensitive) const;
	[[nodiscard]] String RemoveAt(uint32_t index, uint32_t count = 1) const;
	[[nodiscard]] String RemoveChar(char32_t ch, Case cs = Case::Sensitive) const;
	[[nodiscard]] String RemoveStr(const String& str, Case cs = Case::Sensitive) const;
	[[nodiscard]] String RemoveLast(uint32_t num) const;
	[[nodiscard]] String RemoveFirst(uint32_t num) const;
	[[nodiscard]] String InsertAt(uint32_t index, const String& str) const;

	[[nodiscard]] String SafeLua() const;
	[[nodiscard]] String SafeCsv() const;

	[[nodiscard]] uint32_t FindIndex(const String& str, uint32_t from = 0, Case cs = Case::Sensitive) const;
	[[nodiscard]] uint32_t FindLastIndex(const String& str, uint32_t from = STRING_INVALID_INDEX, Case cs = Case::Sensitive) const;
	[[nodiscard]] uint32_t FindIndex(char32_t chr, uint32_t from = 0, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     IndexValid(uint32_t index) const;
	[[nodiscard]] uint32_t FindLastIndex(char32_t chr, uint32_t from = STRING_INVALID_INDEX, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsStr(const String& str, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsAnyStr(const StringList& list, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsAllStr(const StringList& list, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsChar(char32_t chr, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsAnyChar(const String& list, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     ContainsAllChar(const String& list, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     EndsWith(const String& str, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     StartsWith(const String& str, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     EndsWith(char32_t chr, Case cs = Case::Sensitive) const;
	[[nodiscard]] bool     StartsWith(char32_t chr, Case cs = Case::Sensitive) const;

	[[nodiscard]] String DirectoryWithoutFilename() const;
	[[nodiscard]] String FilenameWithoutDirectory() const;
	[[nodiscard]] String FilenameWithoutExtension() const;
	[[nodiscard]] String ExtensionWithoutFilename() const;
	[[nodiscard]] String FixFilenameSlash() const;
	[[nodiscard]] String FixDirectorySlash() const;

	bool Printf(const char* format, va_list args);

	bool          Printf(const char* format, ...) KYTY_FORMAT_PRINTF(2, 3);
	static String FromPrintf(const char* format, ...) KYTY_FORMAT_PRINTF(1, 2);

	[[nodiscard]] StringList Split(const String& sep, SplitType type = SplitType::SplitNoEmptyParts, Case cs = Case::Sensitive) const;
	[[nodiscard]] StringList Split(char32_t sep, SplitType type = SplitType::SplitNoEmptyParts, Case cs = Case::Sensitive) const;

	[[nodiscard]] uint32_t ToUint32(int base = 10) const;
	[[nodiscard]] uint64_t ToUint64(int base = 10) const;
	[[nodiscard]] int32_t  ToInt32(int base = 10) const;
	[[nodiscard]] int64_t  ToInt64(int base = 10) const;
	[[nodiscard]] double   ToDouble() const;
	[[nodiscard]] float    ToFloat() const;

	[[nodiscard]] uint32_t Hash() const;

	[[nodiscard]] ByteBuffer HexToBin() const;
	static String            HexFromBin(const ByteBuffer& bin);

	bool EqualAscii(const char* ascii_str) const;
	bool EqualAsciiNoCase(const char* ascii_str) const;

	[[nodiscard]] bool IsAlpha() const;

	static String FromCp866(const char* utf8_str) { return String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadCp866); }
	static String FromCp1251(const char* utf8_str) { return String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadCp1251); }
	static String FromUtf8(const char* utf8_str) { return String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8); }
	static String FromUtf8(const char* utf8_str, uint32_t size)
	{
		return String(reinterpret_cast<const uint8_t*>(utf8_str), Char::ReadUtf8, size, true);
	}
	static String FromUtf16(const char16_t* utf16_str) { return String(utf16_str, Char::ReadUtf16); }
	static String FromUtf32(const char32_t* utf32_str) { return String(utf32_str, Char::ReadUtf32); }

	[[nodiscard]] String SortChars() const;

	using iterator       = char32_t*;
	using const_iterator = const char32_t*;

	iterator                     begin() { return GetData(); }                    // NOLINT(readability-identifier-naming)
	iterator                     end() { return GetData() + Size(); }             // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator begin() const { return GetDataConst(); }         // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator end() const { return GetDataConst() + Size(); }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cbegin() const { return GetDataConst(); }        // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cend() const { return GetDataConst() + Size(); } // NOLINT(readability-identifier-naming)

private:
	explicit String(const char32_t* array, uint32_t size);
	// String(const uint8_t *utf8_str);
	// String(const char16_t *utf16_str);

	template <class T, class OP>
	explicit String(const T* str, OP&& read, uint32_t size = 0, bool with_size = false): m_data(new DataType)
	{
		if (str == nullptr)
		{
			m_data->Add(U'\0');
		} else
		{
			// const T *ptr = (const T*)str;
			const auto* ptr = str;
			for (;;)
			{
				if (with_size && ptr - str >= size)
				{
					m_data->Add(U'\0');
					break;
				}
				char32_t u = read(&ptr);
				m_data->Add(u);
				if (u == U'\0')
				{
					break;
				}
			}
		}
	}

	DataType* m_data;
};

class StringList: public Vector<String>
{
public:
	using Vector<String>::Vector;
	// StringList(): Vector<String>() {};
	// StringList(std::initializer_list<String> list): Vector<String>(list) {};
	// virtual ~StringList() {};

	[[nodiscard]] bool   Contains(const String& str, String::Case cs = String::Case::Sensitive) const;
	[[nodiscard]] String Concat(const String& str) const;
	[[nodiscard]] String Concat(char32_t chr) const;

	[[nodiscard]] bool Equal(const StringList& str) const;
	[[nodiscard]] bool EqualNoCase(const StringList& str) const;
	bool               operator==(const StringList& str) const { return this->Equal(str); }
	bool               operator!=(const StringList& str) const { return !(*this == str); }
};

} // namespace Kyty::Core

namespace Kyty {
using String = Core::String;
} // namespace Kyty

#endif /* CORE_KYTYSTRING_H_ */
