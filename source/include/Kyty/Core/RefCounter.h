#ifndef INCLUDE_KYTY_CORE_REFCOUNTER_H_
#define INCLUDE_KYTY_CORE_REFCOUNTER_H_

#include "Kyty/Core/Common.h"

namespace Kyty::Core {

template <class MutexPolicy>
class RefCounter
{
public:
	RefCounter() = default;

	virtual ~RefCounter() = default;

	void Release() const
	{
		Lock();
		if (DecRef() == 0)
		{
			Unlock();
			delete this;
		} else
		{
			Unlock();
		}
	}

	template <class PtrType>
	void CopyPtr(PtrType** dst, PtrType* src) const
	{
		Lock();
		*dst = src;
		AddRef();
		Unlock();
	}

	template <class PtrType>
	void CopyOnWrite(PtrType** data) const
	{
		Lock();
		if (Refs() > 1)
		{
			if (DecRef() == 0)
			{
				Unlock();
				delete this;
			} else
			{
				Unlock();
			}

			*data = new PtrType(**data);
		} else
		{
			Unlock();
		}
	}

	KYTY_CLASS_NO_COPY(RefCounter);

private:
	uint32_t AddRef() const { return ++m_refs; }

	uint32_t Refs() const { return m_refs; }

	uint32_t DecRef() const
	{
		EXIT_IF(m_refs == 0);

		uint32_t ret = --m_refs;

		return ret;
	}

	void Lock() const { m_mutex.Lock(); }

	void Unlock() const { m_mutex.Unlock(); }

	mutable uint32_t    m_refs = {1};
	mutable MutexPolicy m_mutex;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_REFCOUNTER_H_ */
