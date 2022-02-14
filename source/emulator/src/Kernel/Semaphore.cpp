#include "Emulator/Kernel/Semaphore.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Timer.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel::Semaphore {

LIB_NAME("libkernel", "libkernel");

class KernelSemaPrivate
{
public:
	enum class Result
	{
		Ok,
		TimedOut,
		Canceled,
		Deleted,
		InvalCount
	};

	KernelSemaPrivate(const String& name, bool /*fifo*/, int init_count, int max_count)
	    : m_name(name), /*m_fifo_order(fifo),*/ m_count(init_count), m_init_count(init_count), m_max_count(max_count) {};
	virtual ~KernelSemaPrivate();

	KYTY_CLASS_NO_COPY(KernelSemaPrivate);

	Result Cancel(int set_count, int* num_waiting_threads);
	Result Signal(int signal_count);
	Result Wait(int need_count, uint32_t* ptr_micros);

	Result Poll(int need_count)
	{
		uint32_t micros = 0;
		return Wait(need_count, &micros);
	}

	[[nodiscard]] const String& GetName() const { return m_name; }

private:
	enum class Status
	{
		Set,
		Canceled,
		Deleted
	};

	Core::Mutex   m_mutex;
	Core::CondVar m_cond_var;
	String        m_name;
	Status        m_status = Status::Set;
	Vector<int>   m_waiting_threads;
	// bool          m_fifo_order;
	int m_count;
	int m_init_count;
	int m_max_count;
};

KernelSemaPrivate::~KernelSemaPrivate()
{
	Core::LockGuard lock(m_mutex);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	m_status = Status::Deleted;

	m_cond_var.SignalAll();

	while (!m_waiting_threads.IsEmpty())
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}
}

KernelSemaPrivate::Result KernelSemaPrivate::Cancel(int set_count, int* num_waiting_threads)
{
	Core::LockGuard lock(m_mutex);

	if (set_count > m_max_count)
	{
		return Result::InvalCount;
	}

	EXIT_NOT_IMPLEMENTED(m_status == Status::Deleted);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	if (num_waiting_threads != nullptr)
	{
		*num_waiting_threads = static_cast<int>(m_waiting_threads.Size());
	}

	m_status = Status::Canceled;

	m_count = (set_count < 0 ? m_init_count : set_count);

	m_cond_var.SignalAll();

	while (!m_waiting_threads.IsEmpty())
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	m_status = Status::Set;

	return Result::Ok;
}

KernelSemaPrivate::Result KernelSemaPrivate::Signal(int signal_count)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(m_status == Status::Deleted);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	if (m_count + signal_count > m_max_count)
	{
		return Result::InvalCount;
	}

	m_count += signal_count;

	m_cond_var.SignalAll();

	return Result::Ok;
}

KernelSemaPrivate::Result KernelSemaPrivate::Wait(int need_count, uint32_t* ptr_micros)
{
	Core::LockGuard lock(m_mutex);

	if (need_count < 1 || need_count > m_max_count)
	{
		return Result::InvalCount;
	}

	uint32_t micros     = 0;
	bool     infinitely = true;
	if (ptr_micros != nullptr)
	{
		micros     = *ptr_micros;
		infinitely = false;
	}

	uint32_t    elapsed = 0;
	Core::Timer t;
	t.Start();

	int id = Core::Thread::GetThreadIdUnique();

	while (!(m_count - need_count >= 0))
	{
		if ((elapsed >= micros && !infinitely))
		{
			*ptr_micros = 0;
			return Result::TimedOut;
		}

		m_waiting_threads.Add(id);

		if (infinitely)
		{
			m_cond_var.Wait(&m_mutex);
		} else
		{
			m_cond_var.WaitFor(&m_mutex, micros - elapsed);
		}

		m_waiting_threads.Remove(id);

		elapsed = static_cast<uint32_t>(t.GetTimeS() * 1000000.0);

		if (m_status == Status::Canceled)
		{
			if (ptr_micros != nullptr)
			{
				*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
			}
			return Result::Canceled;
		}

		if (m_status == Status::Deleted)
		{
			if (ptr_micros != nullptr)
			{
				*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
			}
			return Result::Deleted;
		}
	}

	m_count -= need_count;

	if (ptr_micros != nullptr)
	{
		*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
	}

	return Result::Ok;
}

int KYTY_SYSV_ABI KernelCreateSema(KernelSema* sem, const char* name, uint32_t attr, int init, int max, void* opt)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(sem == nullptr);

	if (name == nullptr || init < 0 || init > max || opt != nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	bool fifo = false;

	switch (attr)
	{
		case 0x01: fifo = true; break;
		case 0x02:
		default: fifo = false; break;
	}

	*sem = new KernelSemaPrivate(String::FromUtf8(name), fifo, init, max);

	printf("\t Semaphore create: %s, %d, %d\n", name, init, max);

	return OK;
}

int KYTY_SYSV_ABI KernelDeleteSema(KernelSema sem)
{
	PRINT_NAME();

	if (sem == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	delete sem;

	return OK;
}

int KYTY_SYSV_ABI KernelWaitSema(KernelSema sem, int need, KernelUseconds* time)
{
	PRINT_NAME();

	if (sem == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	printf("\t Semaphore wait: %s, %d, %d\n", sem->GetName().C_Str(), need, (time != nullptr ? *time : -1));

	auto result = sem->Wait(need, time);

	int ret = OK;

	switch (result)
	{
		case KernelSemaPrivate::Result::Ok: ret = OK; break;
		case KernelSemaPrivate::Result::InvalCount: ret = KERNEL_ERROR_EINVAL; break;
		case KernelSemaPrivate::Result::TimedOut: ret = KERNEL_ERROR_ETIMEDOUT; break;
		case KernelSemaPrivate::Result::Canceled: ret = KERNEL_ERROR_ECANCELED; break;
		case KernelSemaPrivate::Result::Deleted: ret = KERNEL_ERROR_EACCES; break;
	}

	return ret;
}

int KYTY_SYSV_ABI KernelPollSema(KernelSema sem, int need)
{
	PRINT_NAME();

	if (sem == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	printf("\t Semaphore poll: %s, %d\n", sem->GetName().C_Str(), need);

	auto result = sem->Poll(need);

	int ret = OK;

	switch (result)
	{
		case KernelSemaPrivate::Result::Ok: ret = OK; break;
		case KernelSemaPrivate::Result::InvalCount: ret = KERNEL_ERROR_EINVAL; break;
		case KernelSemaPrivate::Result::TimedOut:
		case KernelSemaPrivate::Result::Canceled:
		case KernelSemaPrivate::Result::Deleted: ret = KERNEL_ERROR_EBUSY; break;
	}

	return ret;
}

int KYTY_SYSV_ABI KernelSignalSema(KernelSema sem, int count)
{
	PRINT_NAME();

	if (sem == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	printf("\t Semaphore signal: %s, %d\n", sem->GetName().C_Str(), count);

	auto result = sem->Signal(count);

	int ret = OK;

	switch (result)
	{
		case KernelSemaPrivate::Result::Ok: ret = OK; break;
		case KernelSemaPrivate::Result::InvalCount:
		case KernelSemaPrivate::Result::TimedOut:
		case KernelSemaPrivate::Result::Canceled:
		case KernelSemaPrivate::Result::Deleted: ret = KERNEL_ERROR_EINVAL; break;
	}

	return ret;
}

int KYTY_SYSV_ABI KernelCancelSema(KernelSema sem, int count, int* threads)
{
	PRINT_NAME();

	if (sem == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	auto result = sem->Cancel(count, threads);

	int ret = OK;

	switch (result)
	{
		case KernelSemaPrivate::Result::Ok: ret = OK; break;
		case KernelSemaPrivate::Result::InvalCount:
		case KernelSemaPrivate::Result::TimedOut:
		case KernelSemaPrivate::Result::Canceled:
		case KernelSemaPrivate::Result::Deleted: ret = KERNEL_ERROR_EINVAL; break;
	}

	return ret;
}

} // namespace Kyty::Libs::LibKernel::Semaphore

#endif
