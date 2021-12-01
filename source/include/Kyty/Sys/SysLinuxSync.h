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
	SysCS() { check_ptr = 0; }

	void Init()
	{
		EXIT_IF(check_ptr != 0);

		check_ptr = this;

		pthread_mutexattr_t attr;
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&m, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	void Delete()
	{
		EXIT_IF(check_ptr != this);

		check_ptr = 0;
		pthread_mutex_destroy(&m);
	}

	~SysCS() { EXIT_IF(check_ptr != 0); }

	void Enter()
	{
		EXIT_IF(check_ptr != this);

		pthread_mutex_lock(&m);
	}

	bool TryEnter()
	{
		EXIT_IF(check_ptr != this);

		return pthread_mutex_trylock(&m) == 0;
	}

	void Leave()
	{
		EXIT_IF(check_ptr != this);

		pthread_mutex_unlock(&m);
	}

	KYTY_CLASS_NO_COPY(SysCS);

private:
	SysCS*          check_ptr;
	pthread_mutex_t m;
};

inline void sys_sleep(uint32_t ms)
{
	struct timespec ts;
	ts.tv_sec  = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, 0);
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSSYNC_H_ */
