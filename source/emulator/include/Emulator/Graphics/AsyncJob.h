#ifndef EMULATOR_INCLUDE_EMULATOR_GRAPHICS_ASYNCJOB_H_
#define EMULATOR_INCLUDE_EMULATOR_GRAPHICS_ASYNCJOB_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Threads.h"

#include "Emulator/Common.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::Graphics {

class AsyncJob
{
public:
	using func_t = std::function<void(void*)>;
	explicit AsyncJob(const char* name): m_name(name), m_thread(new Core::Thread(ThreadRun, this)) {}
	virtual ~AsyncJob()
	{
		EXIT_IF(m_thread == nullptr);
		Core::LockGuard lock(m_mutex);
		m_need_exit = true;
		m_cond_var1.Signal();
		m_thread->Join();
		delete m_thread;
	}
	KYTY_CLASS_NO_COPY(AsyncJob);

	void Execute(func_t func, void* arg = nullptr)
	{
		Core::LockGuard lock(m_mutex);
		while (m_func != nullptr)
		{
			m_cond_var2.Wait(&m_mutex);
		}
		m_func = std::move(func);
		m_arg  = arg;
		m_cond_var1.Signal();
	}

	void Wait()
	{
		Core::LockGuard lock(m_mutex);
		while (m_func != nullptr)
		{
			m_cond_var2.Wait(&m_mutex);
		}
	}

private:
	Core::Mutex   m_mutex;
	Core::CondVar m_cond_var1;
	Core::CondVar m_cond_var2;
	const char*   m_name      = nullptr;
	func_t        m_func      = nullptr;
	void*         m_arg       = nullptr;
	bool          m_need_exit = false;
	Core::Thread* m_thread    = nullptr;

	static void ThreadRun(void* data)
	{
		// printf("Start AsyncJob Thread: 0x%" PRIx64 "\n", static_cast<uint64_t>(GetCurrentThreadId()));
		auto* aj = static_cast<AsyncJob*>(data);

		if (aj->m_name != nullptr)
		{
			KYTY_PROFILER_THREAD(aj->m_name);
		}

		for (;;)
		{
			Core::LockGuard lock(aj->m_mutex);
			while (aj->m_func == nullptr && !aj->m_need_exit)
			{
				aj->m_cond_var1.Wait(&aj->m_mutex);
			}
			if (aj->m_need_exit)
			{
				break;
			}
			aj->m_func(aj->m_arg);
			aj->m_func = nullptr;
			aj->m_cond_var2.Signal();
		}
	}
};

} // namespace Kyty::Libs::Graphics

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_GRAPHICS_ASYNCJOB_H_ */
