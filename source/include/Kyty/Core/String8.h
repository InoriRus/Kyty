#ifndef CORE_KYTYSTRING8_H_
#define CORE_KYTYSTRING8_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SimpleArray.h" // IWYU pragma: export
#include "Kyty/Core/Vector.h"

#include <cstdarg> // IWYU pragma: export

namespace Kyty::Core {

constexpr uint32_t STRING8_INVALID_INDEX = static_cast<uint32_t>(-1);

class StringList8;

class String8
{
public:
	using DataType = SimpleArray<char>;

	enum class SplitType
	{
		WithEmptyParts,
		SplitNoEmptyParts
	};

	String8();
	String8(const String8& src);
	String8(String8&& src) noexcept;
	String8(char ch, uint32_t repeat = 1); // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
	String8(const char* str);              // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
	explicit String8(const char* array, uint32_t size);
	virtual ~String8();

	[[nodiscard]] uint32_t Size() const;
	[[nodiscard]] bool     IsEmpty() const;
	[[nodiscard]] bool     IsInvalid() const;

	void Clear();

	char&                     operator[](uint32_t index);
	const char&               operator[](uint32_t index) const;
	[[nodiscard]] const char& At(uint32_t index) const;
	char*                     GetData();
	[[nodiscard]] const char* GetData() const;
	[[nodiscard]] const char* GetDataConst() const;

	String8& operator=(const String8& src);
	String8& operator=(String8&& src) noexcept;
	String8& operator=(char ch);
	String8& operator=(const char* utf8_str);
	String8& operator+=(const String8& src);
	String8& operator+=(char ch);
	String8& operator+=(const char* utf8_str);

	friend String8 operator+(const String8& str1, const String8& str2);
	friend String8 operator+(const char* utf8_str1, const String8& str2);
	friend String8 operator+(const String8& str1, const char* utf8_str2);
	friend String8 operator+(char ch, const String8& str2);
	friend String8 operator+(const String8& str1, char ch);

	[[nodiscard]] bool Equal(const String8& src) const;
	[[nodiscard]] bool Equal(char ch) const;
	bool               Equal(const char* utf8_str) const;

	friend bool operator==(const String8& str1, const String8& str2) { return str2.Equal(str1); }
	friend bool operator==(const char* utf8_str1, const String8& str2) { return str2.Equal(utf8_str1); }
	friend bool operator==(const String8& str1, const char* utf8_str2) { return str1.Equal(utf8_str2); }
	friend bool operator==(char ch, const String8& str2) { return str2.Equal(ch); }
	friend bool operator==(const String8& str1, char ch) { return str1.Equal(ch); }
	friend bool operator!=(const String8& str1, const String8& str2) { return !str2.Equal(str1); }
	friend bool operator!=(const char* utf8_str1, const String8& str2) { return !str2.Equal(utf8_str1); }
	friend bool operator!=(const String8& str1, const char* utf8_str2) { return !str1.Equal(utf8_str2); }
	friend bool operator!=(char ch, const String8& str2) { return !str2.Equal(ch); }
	friend bool operator!=(const String8& str1, char ch) { return !str1.Equal(ch); }

	[[nodiscard]] String8 Mid(uint32_t first, uint32_t count) const;
	[[nodiscard]] String8 Mid(uint32_t first) const;
	[[nodiscard]] String8 Left(uint32_t count) const;
	[[nodiscard]] String8 Right(uint32_t count) const;

	[[nodiscard]] String8 TrimRight() const;
	[[nodiscard]] String8 TrimLeft() const;
	[[nodiscard]] String8 Trim() const;
	[[nodiscard]] String8 Simplify() const;

	[[nodiscard]] String8 ReplaceChar(char old_char, char new_char) const;
	[[nodiscard]] String8 ReplaceStr(const String8& old_str, const String8& new_str) const;
	[[nodiscard]] String8 RemoveAt(uint32_t index, uint32_t count = 1) const;
	[[nodiscard]] String8 RemoveChar(char ch) const;
	[[nodiscard]] String8 RemoveStr(const String8& str) const;
	[[nodiscard]] String8 RemoveLast(uint32_t num) const;
	[[nodiscard]] String8 RemoveFirst(uint32_t num) const;
	[[nodiscard]] String8 InsertAt(uint32_t index, const String8& str) const;

	[[nodiscard]] String8 SafeLua() const;
	[[nodiscard]] String8 SafeCsv() const;

	[[nodiscard]] uint32_t FindIndex(const String8& str, uint32_t from = 0) const;
	[[nodiscard]] uint32_t FindLastIndex(const String8& str, uint32_t from = STRING8_INVALID_INDEX) const;
	[[nodiscard]] uint32_t FindIndex(char chr, uint32_t from = 0) const;
	[[nodiscard]] bool     IndexValid(uint32_t index) const;
	[[nodiscard]] uint32_t FindLastIndex(char chr, uint32_t from = STRING8_INVALID_INDEX) const;
	[[nodiscard]] bool     ContainsStr(const String8& str) const;
	[[nodiscard]] bool     ContainsAnyStr(const StringList8& list) const;
	[[nodiscard]] bool     ContainsAllStr(const StringList8& list) const;
	[[nodiscard]] bool     ContainsChar(char chr) const;
	[[nodiscard]] bool     ContainsAnyChar(const String8& list) const;
	[[nodiscard]] bool     ContainsAllChar(const String8& list) const;
	[[nodiscard]] bool     EndsWith(const String8& str) const;
	[[nodiscard]] bool     StartsWith(const String8& str) const;
	[[nodiscard]] bool     EndsWith(char chr) const;
	[[nodiscard]] bool     StartsWith(char chr) const;

	[[nodiscard]] String8 DirectoryWithoutFilename() const;
	[[nodiscard]] String8 FilenameWithoutDirectory() const;
	[[nodiscard]] String8 FilenameWithoutExtension() const;
	[[nodiscard]] String8 ExtensionWithoutFilename() const;
	[[nodiscard]] String8 FixFilenameSlash() const;
	[[nodiscard]] String8 FixDirectorySlash() const;

	bool Printf(const char* format, va_list args);

	bool           Printf(const char* format, ...) KYTY_FORMAT_PRINTF(2, 3);
	static String8 FromPrintf(const char* format, ...) KYTY_FORMAT_PRINTF(1, 2);

	[[nodiscard]] StringList8 Split(const String8& sep, SplitType type = SplitType::SplitNoEmptyParts) const;
	[[nodiscard]] StringList8 Split(char sep, SplitType type = SplitType::SplitNoEmptyParts) const;

	[[nodiscard]] uint32_t ToUint32(int base = 10) const;
	[[nodiscard]] uint64_t ToUint64(int base = 10) const;
	[[nodiscard]] int32_t  ToInt32(int base = 10) const;
	[[nodiscard]] int64_t  ToInt64(int base = 10) const;
	[[nodiscard]] double   ToDouble() const;
	[[nodiscard]] float    ToFloat() const;

	[[nodiscard]] uint32_t Hash() const;

	[[nodiscard]] const char* c_str() const // NOLINT(readability-identifier-naming)
	{
		EXIT_IF(m_data == nullptr);
		return m_data->GetDataConst();
	}

	[[nodiscard]] String8 SortChars() const;

	using iterator       = char*;
	using const_iterator = const char*;

	iterator                     begin() { return GetData(); }                    // NOLINT(readability-identifier-naming)
	iterator                     end() { return GetData() + Size(); }             // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator begin() const { return GetDataConst(); }         // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator end() const { return GetDataConst() + Size(); }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cbegin() const { return GetDataConst(); }        // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cend() const { return GetDataConst() + Size(); } // NOLINT(readability-identifier-naming)

private:
	DataType* m_data;
};

class StringList8: public Vector<String8>
{
public:
	using Vector<String8>::Vector;

	[[nodiscard]] bool    Contains(const String8& str) const;
	[[nodiscard]] String8 Concat(const String8& str) const;
	[[nodiscard]] String8 Concat(char chr) const;

	[[nodiscard]] bool Equal(const StringList8& str) const;
	bool               operator==(const StringList8& str) const { return this->Equal(str); }
	bool               operator!=(const StringList8& str) const { return !(*this == str); }
};

} // namespace Kyty::Core

namespace Kyty {
using String8 = Core::String8;
} // namespace Kyty

#endif /* CORE_KYTYSTRING8_H_ */
