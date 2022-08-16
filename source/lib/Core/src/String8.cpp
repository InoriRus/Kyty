#include "Kyty/Core/String8.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Sys/SysStdio.h"
#include "Kyty/Sys/SysStdlib.h"

#include <cctype>

namespace Kyty::Core {

KYTY_HASH_DEFINE_CALC(String8)
{
	return key->Hash();
}

KYTY_HASH_DEFINE_EQUALS(String8)
{
	return *key_a == *key_b;
}

static size_t strlen(const char* str)
{
	const char* eos = str;
	while (*eos++ != 0u)
	{
	}
	return (eos - str - 1);
}

static void copy_on_write(String8::DataType** data)
{
	(*data)->CopyOnWrite(data);
}

static bool compare_equal(const String8::DataType* data1, const String8::DataType* data2, uint32_t from1, uint32_t from2, uint32_t size)
{
	const char* p1 = data1->GetData() + from1;
	const char* p2 = data2->GetData() + from2;

	for (uint32_t i = 0; i < size; i++)
	{
		if (*(p1) != *(p2))
		{
			return false;
		}
		p1++;
		p2++;
	}

	return true;
}

static bool compare_equal(const String8::DataType* data1, char data2, uint32_t from1)
{
	const char* p1 = data1->GetData() + from1;

	return (*(p1) == data2);
}

namespace Char {
bool IsSpace(char c)
{
	return std::isspace(c) != 0;
}
} // namespace Char

String8::String8(): m_data(new DataType(1, false))
{
	(*m_data)[0] = '\0';
}

String8::String8(const String8& src): m_data(nullptr)
{
	//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
	src.m_data->CopyPtr(&m_data, src.m_data);
}

String8::String8(String8&& src) noexcept: m_data(src.m_data)
{
	src.m_data = nullptr;
}

String8& String8::operator=(String8&& src) noexcept
{
	if (m_data != src.m_data)
	{
		if (m_data != nullptr)
		{
			m_data->Release();
			m_data = nullptr;
		}

		m_data     = src.m_data;
		src.m_data = nullptr;
	}
	return *this;
}

String8::String8(char ch, uint32_t repeat): m_data(new DataType(repeat + 1, false))
{
	for (uint32_t i = 0; i < repeat; i++)
	{
		(*m_data)[i] = ch;
	}
	(*m_data)[repeat] = '\0';
}

String8::String8(const char* str): String8(str, static_cast<uint32_t>(strlen(str))) {}

uint32_t String8::Hash() const
{
	EXIT_IF(m_data == nullptr);

	return m_data->Hash();
}

String8::String8(const char* array, uint32_t size): m_data(new DataType(size + 1, false))
{
	std::memcpy(m_data->GetData(), array, sizeof(char) * size);
	(*m_data)[size] = '\0';
}

String8::~String8()
{
	if (m_data != nullptr)
	{
		//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
		m_data->Release();
		m_data = nullptr;
	}
}

uint32_t String8::Size() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = m_data->Size();

	EXIT_IF(size == 0);
	//	if (size == 0)
	//	{
	//		return 0;
	//	}

	return size - 1;
}

bool String8::IsEmpty() const
{
	EXIT_IF(m_data == nullptr);

	return (m_data->Size() == 0 || m_data->At(0) == 0);
}

bool String8::IsInvalid() const
{
	return (m_data == nullptr);
}

void String8::Clear()
{
	//	data->Lock();
	//	if (data->DecRef() == 0)
	//	{
	//		data->Unlock();
	//		delete data;
	//	} else
	//	{
	//		data->Unlock();
	//	}
	if (m_data != nullptr)
	{
		m_data->Release();
		m_data = nullptr;
	}

	m_data       = new DataType(1, false);
	(*m_data)[0] = '\0';
}

char& String8::operator[](uint32_t index)
{
	EXIT_IF(m_data == nullptr);
	EXIT_IF(index >= m_data->Size());

	copy_on_write(&m_data);
	return (*m_data)[index];
}

const char& String8::operator[](uint32_t index) const
{
	EXIT_IF(m_data == nullptr);
	EXIT_IF(index >= m_data->Size());

	return m_data->At(index);
}

const char& String8::At(uint32_t index) const
{
	EXIT_IF(m_data == nullptr);
	if (index >= m_data->Size())
	{
		EXIT_IF(index >= m_data->Size());
	}

	return m_data->At(index);
}

char* String8::GetData()
{
	EXIT_IF(m_data == nullptr);
	copy_on_write(&m_data);
	return m_data->GetData();
}

const char* String8::GetData() const
{
	EXIT_IF(m_data == nullptr);
	return m_data->GetDataConst();
}

const char* String8::GetDataConst() const
{
	EXIT_IF(m_data == nullptr);
	return m_data->GetDataConst();
}

// const char* String8::c_str() const
//{
//	return GetDataConst();
// }

String8& String8::operator=(const String8& src) // NOLINT(bugprone-unhandled-self-assignment, cert-oop54-cpp)
{
	if (m_data != src.m_data)
	{
		if (m_data != nullptr)
		{
			m_data->Release();
			m_data = nullptr;
		}

		src.m_data->CopyPtr(&m_data, src.m_data);
	}
	return *this;
}

String8& String8::operator=(char ch)
{
	(*this) = String8(ch);
	return *this;
}

String8& String8::operator=(const char* utf8_str)
{
	(*this) = String8(utf8_str);
	return *this;
}

String8& String8::operator+=(const String8& src)
{
	EXIT_IF(m_data == nullptr);

	if (this == &src)
	{
		const String8 src2(src); // NOLINT(performance-unnecessary-copy-initialization)
		copy_on_write(&m_data);
		m_data->RemoveAt(m_data->Size() - 1);
		m_data->Add(src2.GetData(), src2.Size() + 1);
	} else
	{
		copy_on_write(&m_data);
		m_data->RemoveAt(m_data->Size() - 1);
		m_data->Add(src.GetData(), src.Size() + 1);
	}
	return *this;
}

String8& String8::operator+=(char ch)
{
	(*this) += String8(ch);
	return *this;
}

String8& String8::operator+=(const char* utf8_str)
{
	(*this) += String8(utf8_str);
	return *this;
}

String8 operator+(const String8& str1, const String8& str2)
{
	String8 s(str1);
	s += str2;
	return s;
}

String8 operator+(const char* utf8_str1, const String8& str2)
{
	String8 str1(utf8_str1);
	return str1 + str2;
}

String8 operator+(const String8& str1, const char* utf8_str2)
{
	return str1 + String8(utf8_str2);
}

String8 operator+(char ch, const String8& str2)
{
	return String8(ch) + str2;
}

String8 operator+(const String8& str1, char ch)
{
	return str1 + String8(ch);
}

bool String8::Equal(const String8& src) const
{
	EXIT_IF(m_data == nullptr);

	if (m_data == src.m_data)
	{
		return true;
	}

	uint32_t size = this->Size();

	if (size != src.Size())
	{
		return false;
	}

	return compare_equal(m_data, src.m_data, 0, 0, size);
}

bool String8::Equal(char ch) const
{
	return this->Equal(String8(ch));
}

bool String8::Equal(const char* utf8_str) const
{
	return this->Equal(String8(utf8_str));
}

String8 String8::Mid(uint32_t first, uint32_t count) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (first >= size)
	{
		return {};
	}

	if (first + count > size)
	{
		count = size - first;
	}

	if (first == 0 && count == size)
	{
		return *this;
	}

	return String8(m_data->GetDataConst() + first, count);
}

String8 String8::Mid(uint32_t first) const
{
	return Mid(first, Size() - first);
}

String8 String8::Left(uint32_t count) const
{
	return Mid(0, count);
}

String8 String8::Right(uint32_t count) const
{
	uint32_t size = Size();
	if (count >= size)
	{
		return *this;
	}
	return Mid(size - count, count);
}

String8 String8::TrimRight() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(size - i - 1)))
		{
			return Mid(0, size - i);
		}
	}
	return {};
}

String8 String8::TrimLeft() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(i)))
		{
			return Mid(i, size - i);
		}
	}
	return {};
}

String8 String8::Trim() const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size     = Size();
	uint32_t left_pos = size;
	uint32_t count    = 0;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!Char::IsSpace(m_data->At(i)))
		{
			left_pos = i;
			break;
		}
	}

	for (uint32_t i = 0; i < size - left_pos; i++)
	{
		if (!Char::IsSpace(m_data->At(size - i - 1)))
		{
			count = size - left_pos - i;
			break;
		}
	}

	return Mid(left_pos, count);
}

String8 String8::Simplify() const
{
	EXIT_IF(m_data == nullptr);

	String8  r;
	uint32_t size       = Size();
	bool     prev_space = true;

	for (uint32_t i = 0; i < size; i++)
	{
		char c = m_data->At(i);
		if (Char::IsSpace(c))
		{
			if (prev_space)
			{
				continue;
			}

			prev_space = true;
		} else
		{
			prev_space = false;
		}

		r += c;
	}
	return r.TrimRight();
}

String8 String8::ReplaceChar(char old_char, char new_char) const
{
	EXIT_IF(m_data == nullptr);

	String8 r(*this);

	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (m_data->At(i) == old_char)
		{
			r[i] = new_char;
		}
	}

	return r;
}

String8 String8::RemoveAt(uint32_t index, uint32_t count) const
{
	EXIT_IF(m_data == nullptr);

	if (index >= Size())
	{
		return *this;
	}
	String8 r(*this);
	copy_on_write(&r.m_data);
	r.m_data->RemoveAt(index, count);
	if (r.m_data->Size() == 0 || r.At(r.m_data->Size() - 1) != 0)
	{
		r.m_data->Add('\0');
	}
	return r;
}

String8 String8::RemoveChar(char ch) const
{
	EXIT_IF(m_data == nullptr);

	String8 r(*this);
	bool    cow = false;
	for (uint32_t i = 0; i < r.Size();)
	{
		if ((r.At(i) == ch))
		{
			if (!cow)
			{
				copy_on_write(&r.m_data);
				cow = true;
			}

			r.m_data->RemoveAt(i);
		} else
		{
			i++;
		}
	}
	return r;
}

String8 String8::InsertAt(uint32_t index, const String8& str) const
{
	return Mid(0, index) + str + Mid(index, Size() - index);
}

uint32_t String8::FindIndex(const String8& str, uint32_t from) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (from >= size)
	{
		return STRING8_INVALID_INDEX;
	}

	uint32_t str_size = str.Size();

	if (str_size == 0)
	{
		return from;
	}

	for (uint32_t i = from; i + str_size <= size; i++)
	{
		//	NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
		if (compare_equal(m_data, str.m_data, i, 0, str_size))
		{
			return i;
		}
	}

	return STRING8_INVALID_INDEX;
}

uint32_t String8::FindLastIndex(const String8& str, uint32_t from) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (size == 0)
	{
		return STRING8_INVALID_INDEX;
	}

	if (from >= size)
	{
		from = size - 1;
	}

	uint32_t str_size = str.Size();

	if (str_size == 0)
	{
		return from;
	}

	if (from + str_size > size)
	{
		from = size - str_size;
	}

	for (uint32_t i = from; i <= from; i--)
	{
		if (compare_equal(m_data, str.m_data, i, 0, str_size))
		{
			return i;
		}
	}

	return STRING8_INVALID_INDEX;
}

bool String8::IndexValid(uint32_t index) const
{
	return index < Size();
}

uint32_t String8::FindIndex(char chr, uint32_t from) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (from >= size)
	{
		return STRING8_INVALID_INDEX;
	}

	for (uint32_t i = from; i + 1 <= size; i++)
	{
		if (compare_equal(m_data, chr, i))
		{
			return i;
		}
	}

	return STRING8_INVALID_INDEX;
}

uint32_t String8::FindLastIndex(char chr, uint32_t from) const
{
	EXIT_IF(m_data == nullptr);

	uint32_t size = Size();

	if (size == 0)
	{
		return STRING8_INVALID_INDEX;
	}

	if (from >= size)
	{
		from = size - 1;
	}

	if (from + 1 > size)
	{
		from = size - 1;
	}

	for (uint32_t i = from; i <= from; i--)
	{
		if (compare_equal(m_data, chr, i))
		{
			return i;
		}
	}

	return STRING8_INVALID_INDEX;
}

bool String8::Printf(const char* format, va_list args)
{
	uint32_t len = sys_vscprintf(format, args);

	if (len == 0)
	{
		return false;
	}

	char buffer[1024];
	bool use_buffer = (len + 1 <= 1024);

	char* d = (use_buffer ? buffer : new char[len + 1]);
	memset(d, 0, len + 1);

	len = sys_vsnprintf(d, len, format, args);

	*this = d;

	if (!use_buffer)
	{
		DeleteArray(d);
	}

	return !(len == 0);
}

bool String8::Printf(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	bool r = Printf(format, args);

	va_end(args);

	return r;
}

String8 String8::FromPrintf(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	String8               str;
	[[maybe_unused]] bool r = str.Printf(format, args);

	va_end(args);

	EXIT_IF(!r);

	return str;
}

bool String8::ContainsStr(const String8& str) const
{
	if (str.Size() == 0)
	{
		return true;
	}
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(str, 0) != STRING8_INVALID_INDEX;
}

bool String8::ContainsAnyStr(const StringList8& list) const
{
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (ContainsStr(list.At(i)))
		{
			return true;
		}
	}
	return false;
}

bool String8::ContainsAllStr(const StringList8& list) const
{
	bool     ret  = true;
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		ret = ret && ContainsStr(list.At(i));
	}
	return ret;
}

bool String8::ContainsChar(char chr) const
{
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(chr, 0) != STRING8_INVALID_INDEX;
}

bool String8::ContainsAnyChar(const String8& list) const
{
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (ContainsChar(list.At(i)))
		{
			return true;
		}
	}
	return false;
}

bool String8::ContainsAllChar(const String8& list) const
{
	bool     ret  = true;
	uint32_t size = list.Size();
	for (uint32_t i = 0; i < size; i++)
	{
		ret = ret && ContainsChar(list.At(i));
	}
	return ret;
}

bool String8::EndsWith(const String8& str) const
{
	uint32_t str_size = str.Size();
	if (str_size == 0)
	{
		return true;
	}
	uint32_t size = Size();
	if (size == 0)
	{
		return false;
	}
	return FindLastIndex(str, size - 1) == (size - str_size);
}

bool String8::StartsWith(const String8& str) const
{
	if (str.Size() == 0)
	{
		return true;
	}
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(str, 0) == 0;
}

bool String8::EndsWith(char chr) const
{
	uint32_t size = Size();
	if (size == 0)
	{
		return false;
	}
	return FindLastIndex(chr, size - 1) == (size - 1);
}

bool String8::StartsWith(char chr) const
{
	if (Size() == 0)
	{
		return false;
	}
	return FindIndex(chr, 0) == 0;
}

StringList8 String8::Split(const String8& sep, SplitType type) const
{
	StringList8 list;
	uint32_t    start    = 0;
	uint32_t    extra    = (sep.IsEmpty() ? 1 : 0);
	uint32_t    end      = 0;
	uint32_t    sep_size = sep.Size();

	while ((end = FindIndex(sep, start + extra)) != STRING8_INVALID_INDEX)
	{
		if (start != end || type == String8::SplitType::WithEmptyParts)
		{
			list.Add(Mid(start, end - start));
		}

		start = end + sep_size;
	}

	if (start != Size() || type == String8::SplitType::WithEmptyParts)
	{
		list.Add(Mid(start, Size() - start));
	}
	return list;
}

StringList8 String8::Split(char sep, SplitType type) const
{
	StringList8 list;
	uint32_t    start = 0;
	uint32_t    end   = 0;

	while ((end = FindIndex(sep, start)) != STRING8_INVALID_INDEX)
	{
		if (start != end || type == String8::SplitType::WithEmptyParts)
		{
			list.Add(Mid(start, end - start));
		}

		start = end + 1;
	}

	if (start != Size() || type == String8::SplitType::WithEmptyParts)
	{
		list.Add(Mid(start, Size() - start));
	}
	return list;
}

String8 String8::ReplaceStr(const String8& old_str, const String8& new_str) const
{
	String8  str;
	uint32_t start    = 0;
	uint32_t extra    = (old_str.IsEmpty() ? 1 : 0);
	uint32_t end      = 0;
	uint32_t sep_size = old_str.Size();

	while ((end = FindIndex(old_str, start + extra)) != STRING8_INVALID_INDEX)
	{
		if (start != end)
		{
			str += Mid(start, end - start);
		}

		str += new_str;

		start = end + sep_size;
	}

	if (start != Size())
	{
		str += Mid(start, Size() - start);
	}
	return str;
}

String8 String8::RemoveStr(const String8& str) const
{
	String8  ret_str;
	uint32_t start    = 0;
	uint32_t extra    = (str.IsEmpty() ? 1 : 0);
	uint32_t end      = 0;
	uint32_t sep_size = str.Size();

	while ((end = FindIndex(str, start + extra)) != STRING8_INVALID_INDEX)
	{
		if (start != end)
		{
			ret_str += Mid(start, end - start);
		}

		start = end + sep_size;
	}

	if (start != Size())
	{
		ret_str += Mid(start, Size() - start);
	}
	return ret_str;
}

uint32_t String8::ToUint32(int base) const
{
	return sys_strtoui32(c_str(), nullptr, base);
}

uint64_t String8::ToUint64(int base) const
{
	return sys_strtoui64(c_str(), nullptr, base);
}

int32_t String8::ToInt32(int base) const
{
	return sys_strtoi32(c_str(), nullptr, base);
}

int64_t String8::ToInt64(int base) const
{
	return sys_strtoi64(c_str(), nullptr, base);
}

double String8::ToDouble() const
{
	return sys_strtod(c_str(), nullptr);
}

float String8::ToFloat() const
{
	return sys_strtof(c_str(), nullptr);
}

bool StringList8::Contains(const String8& str) const
{
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (At(i).ContainsStr(str))
		{
			return true;
		}
	}
	return false;
}

String8 StringList8::Concat(const String8& str) const
{
	String8  r;
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		r += (i > 0 ? str : String8("")) + At(i);
	}
	return r;
}

String8 StringList8::Concat(char chr) const
{
	String8  r;
	uint32_t size = Size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (i > 0)
		{
			r += chr;
		}
		r += At(i);
	}
	return r;
}

String8 String8::DirectoryWithoutFilename() const
{
	uint32_t index = FindLastIndex("/");

	if (index == STRING8_INVALID_INDEX)
	{
		return {};
	}

	return Left(index + 1);
}

String8 String8::FilenameWithoutDirectory() const
{
	uint32_t index = FindLastIndex("/");

	if (index == STRING8_INVALID_INDEX)
	{
		return *this;
	}

	return Mid(index + 1);
}

String8 String8::FilenameWithoutExtension() const
{
	uint32_t index = FindLastIndex(".");

	if (index == STRING8_INVALID_INDEX)
	{
		return *this;
	}

	return Left(index);
}

String8 String8::ExtensionWithoutFilename() const
{
	uint32_t index = FindLastIndex(".");

	if (index == STRING8_INVALID_INDEX)
	{
		return {};
	}

	return Mid(index);
}

String8 String8::FixFilenameSlash() const
{
	auto str = ReplaceChar('\\', '/');
	return str;
}

String8 String8::FixDirectorySlash() const
{
	auto str = ReplaceChar('\\', '/');
	if (!str.EndsWith('/'))
	{
		str += '/';
	}
	return str;
}

String8 String8::RemoveLast(uint32_t num) const
{
	uint32_t size = Size();

	if (num >= size)
	{
		return {};
	}

	return Left(size - num);
}

String8 String8::RemoveFirst(uint32_t num) const
{
	uint32_t size = Size();

	if (num >= size)
	{
		return {};
	}

	return Right(size - num);
}

bool StringList8::Equal(const StringList8& str) const
{
	uint32_t size = Size();

	if (size != str.Size())
	{
		return false;
	}

	for (uint32_t i = 0; i < size; i++)
	{
		if (!At(i).Equal(str.At(i)))
		{
			return false;
		}
	}
	return true;
}

String8 String8::SafeLua() const
{
	return ReplaceStr("\\", "\\\\").ReplaceStr("\'", "\\'");
}

String8 String8::SafeCsv() const
{
	bool add_space = false;
	if (StartsWith('+') || StartsWith('=') || StartsWith('-'))
	{
		add_space = true;
	}
	if (ContainsChar('\"') || ContainsChar(';') || ContainsChar('+') || ContainsChar('=') || ContainsChar('-'))
	{
		return "\"" + String8(add_space ? " " : "") + ReplaceStr("\"", "\"\"") + "\"";
	}
	return *this;
}

String8 String8::SortChars() const
{
	EXIT_IF(m_data == nullptr);

	if (IsEmpty())
	{
		return {};
	}
	auto         size = m_data->Size() - 1;
	Vector<char> d(size);
	std::memcpy(d.GetData(), m_data->GetDataConst(), size * sizeof(char));
	d.Sort();
	return String8(d.GetDataConst(), size);
}

} // namespace Kyty::Core
