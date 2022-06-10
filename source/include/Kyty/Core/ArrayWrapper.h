#ifndef INCLUDE_KYTY_CORE_ARRAYWRAPPER_H_
#define INCLUDE_KYTY_CORE_ARRAYWRAPPER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"

#include <initializer_list> // IWYU pragma: export

namespace Kyty::Core {

template <class Type, int Num>
class Array final
{
public:
	Array()  = default;
	~Array() = default;

	KYTY_CLASS_DEFAULT_COPY(Array);

	Array(std::initializer_list<Type> list) noexcept: m_ptr {}
	{
		int index = 0;
		for (const Type& e: list)
		{
			(*this)[index++] = e;
		}
	}

	const Type& operator[](int index) const
	{
		EXIT_IF(index < 0 || index >= Num);

		return m_ptr[index];
	}

	Type& operator[](int index)
	{
		EXIT_IF(index < 0 || index >= Num);

		return m_ptr[index];
	}

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type const*() const { return &m_ptr[0]; }

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type*() { return &m_ptr[0]; }

	[[nodiscard]] int Size() const { return Num; }

	Type* GetPtr() { return m_ptr; }

	[[nodiscard]] const Type* GetPtr() const { return m_ptr; }

	[[nodiscard]] size_t ByteSize() const { return sizeof(Type) * Num; }

	//	void SetZero()
	//	{
	//		memset(m_ptr, 0, sizeof(Type) * num);
	//	}
	//
	//	void Memset(int fill)
	//	{
	//		memset(m_ptr, fill, sizeof(Type) * num);
	//	}

	using iterator       = Type*;
	using const_iterator = const Type*;

	iterator                     begin() { return &m_ptr[0]; }        // NOLINT(readability-identifier-naming)
	iterator                     end() { return &m_ptr[Num]; }        // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator begin() const { return &m_ptr[0]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator end() const { return &m_ptr[Num]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cbegin() const { return &m_ptr[0]; } // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cend() const { return &m_ptr[Num]; } // NOLINT(readability-identifier-naming)

private:
	Type m_ptr[Num];
};

template <class Type, int Num1, int Num2>
class Array2 final
{
public:
	Array2()  = default;
	~Array2() = default;

	KYTY_CLASS_DEFAULT_COPY(Array2);

	Array2(std::initializer_list<Array<Type, Num2>> list) noexcept: m_ptr {}
	{
		int index = 0;
		for (const Array<Type, Num2>& e: list)
		{
			(*this)[index++] = e;
		}
	}

	const Array<Type, Num2>& operator[](int index) const
	{
		EXIT_IF(index < 0 || index >= Num1);

		return m_ptr[index];
	}

	Array<Type, Num2>& operator[](int index)
	{
		EXIT_IF(index < 0 || index >= Num1);

		return m_ptr[index];
	}

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type const*() const { return &m_ptr[0][0]; }

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type*() { return &m_ptr[0][0]; }

	[[nodiscard]] int Size() const { return Num1; }

	Array<Type, Num2>* GetPtr() { return m_ptr; }

	[[nodiscard]] const Array<Type, Num2>* GetPtr() const { return m_ptr; }

	[[nodiscard]] size_t ByteSize() const { return sizeof(Type) * Num1 * Num2; }

	//	void SetZero()
	//	{
	//		memset(m_ptr, 0, sizeof(Type) * num1 * num2);
	//	}
	//
	//	void Memset(int fill)
	//	{
	//		memset(m_ptr, fill, sizeof(Type) * num1 * num2);
	//	}

	using iterator       = Array<Type, Num2>*;
	using const_iterator = const Array<Type, Num2>*;

	iterator                     begin() { return &m_ptr[0]; }         // NOLINT(readability-identifier-naming)
	iterator                     end() { return &m_ptr[Num1]; }        // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator begin() const { return &m_ptr[0]; }   // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator end() const { return &m_ptr[Num1]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cbegin() const { return &m_ptr[0]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cend() const { return &m_ptr[Num1]; } // NOLINT(readability-identifier-naming)

private:
	Array<Type, Num2> m_ptr[Num1];
};

template <class Type, int Num1, int Num2, int Num3>
class Array3
{
public:
	Array3()  = default;
	~Array3() = default;

	KYTY_CLASS_DEFAULT_COPY(Array3);

	Array3(std::initializer_list<Array2<Type, Num2, Num3>> list) noexcept: m_ptr {}
	{
		int index = 0;
		for (const Array2<Type, Num2, Num3>& e: list)
		{
			(*this)[index++] = e;
		}
	}

	const Array2<Type, Num2, Num3>& operator[](int index) const
	{
		EXIT_IF(index < 0 || index >= Num1);

		return m_ptr[index];
	}

	Array2<Type, Num2, Num3>& operator[](int index)
	{
		EXIT_IF(index < 0 || index >= Num1);

		return m_ptr[index];
	}

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type const*() const { return &m_ptr[0][0][0]; }

	// NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
	operator Type*() { return &m_ptr[0][0][0]; }

	[[nodiscard]] int Size() const { return Num1; }

	Array2<Type, Num2, Num3>* GetPtr() { return m_ptr; }

	[[nodiscard]] const Array2<Type, Num2, Num3>* GetPtr() const { return m_ptr; }

	[[nodiscard]] size_t ByteSize() const { return sizeof(Type) * Num1 * Num2 * Num3; }

	//	void SetZero()
	//	{
	//		memset(m_ptr, 0, sizeof(Type) * num1 * num2 * num3);
	//	}
	//
	//	void Memset(int fill)
	//	{
	//		memset(m_ptr, fill, sizeof(Type) * num1 * num2 * num3);
	//	}

	using iterator       = Array2<Type, Num2, Num3>*;
	using const_iterator = const Array2<Type, Num2, Num3>*;

	iterator                     begin() { return &m_ptr[0]; }         // NOLINT(readability-identifier-naming)
	iterator                     end() { return &m_ptr[Num1]; }        // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator begin() const { return &m_ptr[0]; }   // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator end() const { return &m_ptr[Num1]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cbegin() const { return &m_ptr[0]; }  // NOLINT(readability-identifier-naming)
	[[nodiscard]] const_iterator cend() const { return &m_ptr[Num1]; } // NOLINT(readability-identifier-naming)

private:
	Array2<Type, Num2, Num3> m_ptr[Num1];
};

} // namespace Kyty::Core

//#define KYTY_ARRAY_NUM(a) ((int)(sizeof((a)) / sizeof((a)[0])))

#endif /* INCLUDE_KYTY_CORE_ARRAYWRAPPER_H_ */
