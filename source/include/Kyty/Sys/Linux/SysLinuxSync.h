#ifndef SYS_LINUX_INCLUDE_KYTY_SYSSYNC_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSSYNC_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include <pthread.h>
#include <unistd.h>

namespace Kyty {

class SysCS
{
public:
	SysCS() = default;

	void Init()
	{
		EXIT_IF(m_check_ptr != nullptr);

		m_check_ptr = this;

		pthread_mutexattr_t attr {};
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&m_mutex, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	void Delete()
	{
		EXIT_IF(m_check_ptr != this);

		m_check_ptr = nullptr;
		pthread_mutex_destroy(&m_mutex);
	}

	~SysCS() { EXIT_IF(m_check_ptr != nullptr); }

	void Enter()
	{
		EXIT_IF(m_check_ptr != this);

		pthread_mutex_lock(&m_mutex);
	}

	bool TryEnter()
	{
		EXIT_IF(m_check_ptr != this);

		return pthread_mutex_trylock(&m_mutex) == 0;
	}

	void Leave()
	{
		EXIT_IF(m_check_ptr != this);

		pthread_mutex_unlock(&m_mutex);
	}

	KYTY_CLASS_NO_COPY(SysCS);

private:
	SysCS*          m_check_ptr = nullptr;
	pthread_mutex_t m_mutex {};
};

inline void sys_sleep(uint32_t ms)
{
	struct timespec ts
	{
	};
	ts.tv_sec  = ms / 1000;
	ts.tv_nsec = static_cast<int64_t>(ms % 1000) * 1000000;
	nanosleep(&ts, nullptr);
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSSYNC_H_ */
