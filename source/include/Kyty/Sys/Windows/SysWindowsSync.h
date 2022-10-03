#ifndef SYS_WIN32_INCLUDE_KYTY_SYSSYNC_H_
#define SYS_WIN32_INCLUDE_KYTY_SYSSYNC_H_

// IWYU pragma: private

#include "Kyty/Core/DbgAssert.h"

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#include <windows.h>

namespace Kyty {

constexpr DWORD SYS_CS_SPIN_COUNT = 16;

class SysCS
{
public:
	SysCS() = default;

	void Init()
	{
		EXIT_IF(m_check_ptr != nullptr);

		m_check_ptr = this;
		InitializeCriticalSectionAndSpinCount(&m_cs, SYS_CS_SPIN_COUNT);
	}

	void Delete()
	{
		EXIT_IF(m_check_ptr != this);

		m_check_ptr = nullptr;
		DeleteCriticalSection(&m_cs);
	}

	~SysCS() { EXIT_IF(m_check_ptr != nullptr); }

	void Enter()
	{
		EXIT_IF(m_check_ptr != this);

		EnterCriticalSection(&m_cs);
	}

	bool TryEnter()
	{
		EXIT_IF(m_check_ptr != this);

		return TryEnterCriticalSection(&m_cs) != 0;
	}

	void Leave()
	{
		EXIT_IF(m_check_ptr != this);

		LeaveCriticalSection(&m_cs);
	}

	KYTY_CLASS_NO_COPY(SysCS);

private:
	SysCS*           m_check_ptr = nullptr;
	CRITICAL_SECTION m_cs {};
};

inline void sys_sleep(uint32_t ms)
{
	Sleep(ms);
}

} // namespace Kyty

#endif

#endif /* SYS_WIN32_INCLUDE_KYTY_SYSSYNC_H_ */
