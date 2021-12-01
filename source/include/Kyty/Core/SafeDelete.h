#ifndef INCLUDE_KYTY_CORE_SAFEDELETE_H_
#define INCLUDE_KYTY_CORE_SAFEDELETE_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"

#include <cstring>

namespace Kyty {

#define KYTY_CORE_SAFE_DELETE_DEADBEEF (reinterpret_cast<void*>((uintptr_t)0x01))

#define DeleteProtected(p) /* NOLINT(cppcoreguidelines-macro-usage) */                                                                     \
	{                                                                                                                                      \
		if (!(p))                                                                                                                          \
		{                                                                                                                                  \
			EXIT("delete null\n");                                                                                                         \
		}                                                                                                                                  \
		void* dddd = KYTY_CORE_SAFE_DELETE_DEADBEEF;                                                                                       \
		if (std::memcmp(&(p), &dddd, sizeof(void*)) == 0)                                                                                  \
		{                                                                                                                                  \
			EXIT("already deleted\n");                                                                                                     \
		}                                                                                                                                  \
		delete (p);                                                                                                                        \
		std::memcpy(&(p), &dddd, sizeof(void*));                                                                                           \
	}

#define DeleteProtectedArray(p) /* NOLINT(cppcoreguidelines-macro-usage) */                                                                \
	{                                                                                                                                      \
		if (!(p))                                                                                                                          \
		{                                                                                                                                  \
			EXIT("delete null\n");                                                                                                         \
		}                                                                                                                                  \
		void* dddd = KYTY_CORE_SAFE_DELETE_DEADBEEF;                                                                                       \
		if (std::memcmp(&(p), &dddd, sizeof(void*)) == 0)                                                                                  \
		{                                                                                                                                  \
			EXIT("already deleted\n");                                                                                                     \
		}                                                                                                                                  \
		delete[](p);                                                                                                                       \
		std::memcpy(&(p), &dddd, sizeof(void*));                                                                                           \
	}

template <class T>
void Delete(T*& p)
{
	if (!p)
	{
		EXIT("delete null\n");
	}
	if (p == static_cast<T*>(KYTY_CORE_SAFE_DELETE_DEADBEEF))
	{
		EXIT("already deleted\n");
	}
	delete p;
	p = static_cast<T*>(KYTY_CORE_SAFE_DELETE_DEADBEEF);
}

template <class T>
void DeleteArray(T*& p)
{
	if (!p)
	{
		EXIT("delete null\n");
	}
	if (p == static_cast<T*>(KYTY_CORE_SAFE_DELETE_DEADBEEF))
	{
		EXIT("already deleted\n");
	}
	delete[] p;
	p = static_cast<T*>(KYTY_CORE_SAFE_DELETE_DEADBEEF);
}

} // namespace Kyty

#endif /* INCLUDE_KYTY_CORE_SAFEDELETE_H_ */
