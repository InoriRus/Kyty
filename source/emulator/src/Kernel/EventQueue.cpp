#include "Emulator/Kernel/EventQueue.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/LinkList.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Timer.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel::EventQueue {

LIB_NAME("libkernel", "libkernel");

class KernelEqueuePrivate
{
public:
	KernelEqueuePrivate() = default;
	virtual ~KernelEqueuePrivate();

	KYTY_CLASS_NO_COPY(KernelEqueuePrivate);

	[[nodiscard]] const String& GetName() const { return m_name; }
	void                        SetName(const String& m_name) { this->m_name = m_name; }

	void AddEvent(const KernelEqueueEvent& event);
	bool TriggerEvent(uintptr_t ident, int16_t filter, void* trigger_data);
	bool DeleteEvent(uintptr_t ident, int16_t filter);

	int GetTriggeredEvents(KernelEvent* ev, int num);
	int WaitForEvents(KernelEvent* ev, int num, uint32_t micros);

private:
	Core::List<KernelEqueueEvent> m_events;
	Core::Mutex                   m_mutex;
	Core::CondVar                 m_cond_var;
	String                        m_name;
};

KernelEqueuePrivate::~KernelEqueuePrivate()
{
	Core::LockGuard lock(m_mutex);

	FOR_LIST(index, m_events)
	{
		auto& event = m_events[index];

		if (event.filter.delete_event_func != nullptr)
		{
			event.filter.delete_event_func(this, &event);
		}
	}
}

int KernelEqueuePrivate::GetTriggeredEvents(KernelEvent* ev, int num)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(num < 1);

	int ret = 0;

	FOR_LIST(index, m_events)
	{
		auto& event = m_events[index];

		if (event.triggered)
		{
			ev[ret++] = event.event;

			if (event.filter.reset_func != nullptr)
			{
				event.filter.reset_func(&event);
			}

			if (ret >= num)
			{
				break;
			}
		}
	}

	return ret;
}

int KernelEqueuePrivate::WaitForEvents(KernelEvent* ev, int num, uint32_t micros)
{
	Core::LockGuard lock(m_mutex);

	EXIT_IF(num < 1);

	uint32_t    elapsed = 0;
	Core::Timer t;
	t.Start();

	for (;;)
	{
		int ret = GetTriggeredEvents(ev, num);

		if (ret > 0 || (elapsed >= micros && micros != 0))
		{
			return ret;
		}

		if (micros == 0)
		{
			m_cond_var.Wait(&m_mutex);
		} else
		{
			m_cond_var.WaitFor(&m_mutex, micros - elapsed);
		}

		elapsed = static_cast<uint32_t>(t.GetTimeS() * 1000000.0);
	}

	return 0;
}

void KernelEqueuePrivate::AddEvent(const KernelEqueueEvent& event)
{
	Core::LockGuard lock(m_mutex);

	if (auto index = m_events.Find(event.event.ident, event.event.filter,
	                               [](auto e, auto ident, auto filter) { return e.event.ident == ident && e.event.filter == filter; });
	    m_events.IndexValid(index))
	{
		m_events[index] = event;
	} else
	{
		m_events.Add(event);
	}

	if (event.triggered)
	{
		m_cond_var.Signal();
	}
}

bool KernelEqueuePrivate::TriggerEvent(uintptr_t ident, int16_t filter, void* trigger_data)
{
	Core::LockGuard lock(m_mutex);

	if (auto index = m_events.Find(ident, filter,
	                               [](auto e, auto ident, auto filter) { return e.event.ident == ident && e.event.filter == filter; });
	    m_events.IndexValid(index))
	{
		auto& event = m_events[index];

		if (event.filter.trigger_func != nullptr)
		{
			event.filter.trigger_func(&event, trigger_data);
		} else
		{
			event.triggered = true;
		}

		m_cond_var.Signal();

		return true;
	}

	return false;
}

bool KernelEqueuePrivate::DeleteEvent(uintptr_t ident, int16_t filter)
{
	Core::LockGuard lock(m_mutex);

	if (auto index = m_events.Find(ident, filter,
	                               [](auto e, auto ident, auto filter) { return e.event.ident == ident && e.event.filter == filter; });
	    m_events.IndexValid(index))
	{
		auto& event = m_events[index];

		if (event.filter.delete_event_func != nullptr)
		{
			event.filter.delete_event_func(this, &event);
		}

		m_events.Remove(index);

		return true;
	}

	return false;
}

int KYTY_SYSV_ABI KernelCreateEqueue(KernelEqueue* eq, const char* name)
{
	PRINT_NAME();

	if (eq == nullptr || name == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	*eq = new KernelEqueuePrivate;

	(*eq)->SetName(String::FromUtf8(name));

	printf("\tEqueue create: %s\n", name);

	return OK;
}

int KYTY_SYSV_ABI KernelAddEvent(KernelEqueue eq, const KernelEqueueEvent& event)
{
	if (eq == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	eq->AddEvent(event);

	return OK;
}

int KYTY_SYSV_ABI KernelTriggerEvent(KernelEqueue eq, uintptr_t ident, int16_t filter, void* trigger_data)
{
	if (eq == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	if (!eq->TriggerEvent(ident, filter, trigger_data))
	{
		return KERNEL_ERROR_ENOENT;
	}

	return OK;
}

int KYTY_SYSV_ABI KernelDeleteEvent(KernelEqueue eq, uintptr_t ident, int16_t filter)
{
	if (eq == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	if (!eq->DeleteEvent(ident, filter))
	{
		return KERNEL_ERROR_ENOENT;
	}

	return OK;
}

int KYTY_SYSV_ABI KernelDeleteEqueue(KernelEqueue eq)
{
	PRINT_NAME();

	if (eq == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	printf("\tEqueue delete: %s\n", eq->GetName().C_Str());

	delete eq;

	return OK;
}

int KYTY_SYSV_ABI KernelWaitEqueue(KernelEqueue eq, KernelEvent* ev, int num, int* out, const KernelUseconds* timo)
{
	// PRINT_NAME();

	if (eq == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	if (ev == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	if (num < 1)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(out == nullptr);

	// printf("\tEqueue wait: %s\n", eq->GetName().C_Str());

	if (timo == nullptr)
	{
		*out = eq->WaitForEvents(ev, num, 0);
	}

	if (timo != nullptr)
	{
		if (*timo == 0)
		{
			*out = eq->GetTriggeredEvents(ev, num);
		} else
		{
			*out = eq->WaitForEvents(ev, num, *timo);
		}
	}

	if (*out == 0)
	{
		// printf("\ttimedout\n");
		return KERNEL_ERROR_ETIMEDOUT;
	}

	// printf("\treceived %u events\n", *out);

	return OK;
}

intptr_t KYTY_SYSV_ABI KernelGetEventData(const KernelEvent* ev)
{
	PRINT_NAME();

	if (ev != nullptr)
	{
		return ev->data;
	}

	return 0;
}

intptr_t KYTY_SYSV_ABI KernelGetEventFflags(const KernelEvent* ev)
{
	PRINT_NAME();

	if (ev != nullptr)
	{
		return ev->fflags;
	}

	return 0;
}

int KYTY_SYSV_ABI KernelGetEventFilter(const KernelEvent* ev)
{
	PRINT_NAME();

	if (ev != nullptr)
	{
		return ev->filter;
	}

	return 0;
}

uintptr_t KYTY_SYSV_ABI KernelGetEventId(const KernelEvent* ev)
{
	PRINT_NAME();

	if (ev != nullptr)
	{
		return ev->ident;
	}

	return 0;
}

void* KYTY_SYSV_ABI KernelGetEventUserData(const KernelEvent* ev)
{
	PRINT_NAME();

	if (ev != nullptr)
	{
		return ev->udata;
	}

	return nullptr;
}

int KYTY_SYSV_ABI KernelGetEventError(const KernelEvent* /*ev*/)
{
	PRINT_NAME();

	KYTY_NOT_IMPLEMENTED;

	return 0;
}

} // namespace Kyty::Libs::LibKernel::EventQueue

#endif // KYTY_EMU_ENABLED
