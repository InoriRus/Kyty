#include "Emulator/Kernel/EventFlag.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Timer.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel::EventFlag {

LIB_NAME("libkernel", "libkernel");

class KernelEventFlagPrivate
{
public:
	enum class Result
	{
		Ok,
		AlreadyWaiting,
		TimedOut,
		Canceled,
		Deleted
	};

	enum class ClearMode
	{
		None,
		All,
		Bits
	};

	enum class WaitMode
	{
		And,
		Or
	};

	KernelEventFlagPrivate(const String& name, bool single, bool /*fifo*/, uint64_t bits)
	    : m_name(name), m_single_thread(single), m_bits(bits) {};
	virtual ~KernelEventFlagPrivate();

	KYTY_CLASS_NO_COPY(KernelEventFlagPrivate);

	void   Set(uint64_t bits);
	void   Clear(uint64_t bits);
	void   Cancel(uint64_t bits, int* num_waiting_threads);
	Result Wait(uint64_t bits, WaitMode wait_mode, ClearMode clear_mode, uint64_t* result, uint32_t* ptr_micros);

	Result Poll(uint64_t bits, WaitMode wait_mode, ClearMode clear_mode, uint64_t* result)
	{
		uint32_t micros = 0;
		return Wait(bits, wait_mode, clear_mode, result, &micros);
	}

private:
	enum class Status
	{
		Set,
		Canceled,
		Deleted
	};

	Core::Mutex   m_mutex;
	Core::CondVar m_cond_var;
	Status        m_status          = Status::Set;
	int           m_waiting_threads = 0;
	String        m_name;
	bool          m_single_thread = false;
	uint64_t      m_bits          = 0;
};

KernelEventFlagPrivate::~KernelEventFlagPrivate()
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

	while (m_waiting_threads > 0)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}
}

KernelEventFlagPrivate::Result KernelEventFlagPrivate::Wait(uint64_t bits, WaitMode wait_mode, ClearMode clear_mode, uint64_t* result,
                                                            uint32_t* ptr_micros)
{
	Core::LockGuard lock(m_mutex);

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

	if (m_single_thread && m_waiting_threads > 0)
	{
		return Result::AlreadyWaiting;
	}

	while (!((wait_mode == WaitMode::And && (m_bits & bits) == bits) || (wait_mode == WaitMode::Or && (m_bits & bits) != 0)))
	{
		if ((elapsed >= micros && !infinitely))
		{
			if (result != nullptr)
			{
				*result = m_bits;
			}
			*ptr_micros = 0;
			return Result::TimedOut;
		}

		m_waiting_threads++;

		if (infinitely)
		{
			m_cond_var.Wait(&m_mutex);
		} else
		{
			m_cond_var.WaitFor(&m_mutex, micros - elapsed);
		}

		m_waiting_threads--;

		elapsed = static_cast<uint32_t>(t.GetTimeS() * 1000000.0);

		if (m_status == Status::Canceled)
		{
			if (result != nullptr)
			{
				*result = m_bits;
			}
			if (ptr_micros != nullptr)
			{
				*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
			}
			return Result::Canceled;
		}

		if (m_status == Status::Deleted)
		{
			if (result != nullptr)
			{
				*result = m_bits;
			}
			if (ptr_micros != nullptr)
			{
				*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
			}
			return Result::Deleted;
		}
	}

	if (result != nullptr)
	{
		*result = m_bits;
	}

	if (clear_mode == ClearMode::All)
	{
		m_bits = 0;
	} else if (clear_mode == ClearMode::Bits)
	{
		m_bits &= ~bits;
	}

	if (ptr_micros != nullptr)
	{
		*ptr_micros = (elapsed >= micros ? 0 : micros - elapsed);
	}

	return Result::Ok;
}

void KernelEventFlagPrivate::Set(uint64_t bits)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(m_status == Status::Deleted);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	m_bits |= bits;

	m_cond_var.SignalAll();
}

void KernelEventFlagPrivate::Clear(uint64_t bits)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(m_status == Status::Deleted);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	m_bits &= bits;
}

void KernelEventFlagPrivate::Cancel(uint64_t bits, int* num_waiting_threads)
{
	Core::LockGuard lock(m_mutex);

	EXIT_NOT_IMPLEMENTED(m_status == Status::Deleted);

	while (m_status != Status::Set)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	if (num_waiting_threads != nullptr)
	{
		*num_waiting_threads = m_waiting_threads;
	}

	m_status = Status::Canceled;
	m_bits   = bits;

	m_cond_var.SignalAll();

	while (m_waiting_threads > 0)
	{
		m_mutex.Unlock();
		Core::Thread::SleepMicro(10);
		m_mutex.Lock();
	}

	m_status = Status::Set;
}

int KYTY_SYSV_ABI KernelCreateEventFlag(KernelEventFlag* ef, const char* name, uint32_t attr, uint64_t init_pattern, const void* param)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(param != nullptr);

	if (ef == nullptr || name == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	bool single = true;
	bool fifo   = true;

	switch (attr)
	{
		case 0x10:
		case 0x11:
			single = true;
			fifo   = true;
			break;
		case 0x20:
		case 0x21:
			single = false;
			fifo   = true;
			break;
		case 0x22:
			single = false;
			fifo   = false;
			break;
		default: EXIT("unknown attr: %u\n", attr);
	}

	*ef = new KernelEventFlagPrivate(String::FromUtf8(name), single, fifo, init_pattern);

	printf("\tEventFlag create: %s\n", name);

	return OK;
}

int KYTY_SYSV_ABI KernelDeleteEventFlag(KernelEventFlag ef)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	delete ef;

	return OK;
}

int KYTY_SYSV_ABI KernelWaitEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat,
                                      KernelUseconds* timeout)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	if (bit_pattern == 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	KernelEventFlagPrivate::WaitMode  wait  = KernelEventFlagPrivate::WaitMode::And;
	KernelEventFlagPrivate::ClearMode clear = KernelEventFlagPrivate::ClearMode::None;

	switch (wait_mode & 0xfu)
	{
		case 0x01: wait = KernelEventFlagPrivate::WaitMode::And; break;
		case 0x02: wait = KernelEventFlagPrivate::WaitMode::Or; break;
		default: EXIT("unknown mode: %u\n", wait_mode);
	}

	switch (wait_mode & 0xf0u)
	{
		case 0x00: clear = KernelEventFlagPrivate::ClearMode::None; break;
		case 0x10: clear = KernelEventFlagPrivate::ClearMode::All; break;
		case 0x20: clear = KernelEventFlagPrivate::ClearMode::Bits; break;
		default: EXIT("unknown mode: %u\n", wait_mode);
	}

	auto result = ef->Wait(bit_pattern, wait, clear, result_pat, timeout);

	int ret = OK;

	switch (result)
	{
		case KernelEventFlagPrivate::Result::Ok: ret = OK; break;
		case KernelEventFlagPrivate::Result::AlreadyWaiting: ret = KERNEL_ERROR_EPERM; break;
		case KernelEventFlagPrivate::Result::TimedOut: ret = KERNEL_ERROR_ETIMEDOUT; break;
		case KernelEventFlagPrivate::Result::Canceled: ret = KERNEL_ERROR_ECANCELED; break;
		case KernelEventFlagPrivate::Result::Deleted: ret = KERNEL_ERROR_EACCES; break;
	}

	return ret;
}

int KYTY_SYSV_ABI KernelPollEventFlag(KernelEventFlag ef, uint64_t bit_pattern, uint32_t wait_mode, uint64_t* result_pat)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	if (bit_pattern == 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	KernelEventFlagPrivate::WaitMode  wait  = KernelEventFlagPrivate::WaitMode::And;
	KernelEventFlagPrivate::ClearMode clear = KernelEventFlagPrivate::ClearMode::None;

	switch (wait_mode & 0xfu)
	{
		case 0x01: wait = KernelEventFlagPrivate::WaitMode::And; break;
		case 0x02: wait = KernelEventFlagPrivate::WaitMode::Or; break;
		default: EXIT("unknown mode: %u\n", wait_mode);
	}

	switch (wait_mode & 0xf0u)
	{
		case 0x00: clear = KernelEventFlagPrivate::ClearMode::None; break;
		case 0x10: clear = KernelEventFlagPrivate::ClearMode::All; break;
		case 0x20: clear = KernelEventFlagPrivate::ClearMode::Bits; break;
		default: EXIT("unknown mode: %u\n", wait_mode);
	}

	auto result = ef->Poll(bit_pattern, wait, clear, result_pat);

	int ret = OK;

	switch (result)
	{
		case KernelEventFlagPrivate::Result::Ok: ret = OK; break;
		case KernelEventFlagPrivate::Result::AlreadyWaiting: ret = KERNEL_ERROR_EPERM; break;
		case KernelEventFlagPrivate::Result::TimedOut:
		case KernelEventFlagPrivate::Result::Canceled:
		case KernelEventFlagPrivate::Result::Deleted: ret = KERNEL_ERROR_EBUSY; break;
	}

	return ret;
}

int KYTY_SYSV_ABI KernelSetEventFlag(KernelEventFlag ef, uint64_t bit_pattern)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	ef->Set(bit_pattern);

	return OK;
}

int KYTY_SYSV_ABI KernelClearEventFlag(KernelEventFlag ef, uint64_t bit_pattern)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	ef->Clear(bit_pattern);

	return OK;
}

int KYTY_SYSV_ABI KernelCancelEventFlag(KernelEventFlag ef, uint64_t set_pattern, int* num_wait_threads)
{
	PRINT_NAME();

	if (ef == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	ef->Cancel(set_pattern, num_wait_threads);

	return OK;
}

} // namespace Kyty::Libs::LibKernel::EventFlag

#endif // KYTY_EMU_ENABLED
