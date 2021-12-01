#include "Kyty/Core/Threads.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/String.h"

//#define THREADS_SDL

#ifdef THREADS_SDL
#include "SDL.h"
#else
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#endif

namespace Kyty::Core {

#ifdef THREADS_SDL
typedef uint64_t thread_id_t;
#else
using thread_id_t = std::thread::id;
#endif

struct Thread::ThreadPrivate
{
	ThreadPrivate(thread_func_t func, void* arg)
	    : finished(false), auto_delete(false),
#ifdef THREADS_SDL
	      m_func(func), m_arg(arg)
	{
		sdl = SDL_CreateThread(thread_run, "sdl_thread", this);
	}
#else
	      m_thread(func, arg)
	{
	}
#endif

	bool finished;
	bool auto_delete;
#ifdef THREADS_SDL
	SDL_Thread*   sdl;
	thread_func_t m_func;
	void*         m_arg;

	static int thread_run(void* data)
	{
		ThreadPrivate* t = (ThreadPrivate*)data;
		t->m_func(t->m_arg);
		return 0;
	}

#else
	std::thread                 m_thread;
#endif
};

struct Mutex::MutexPrivate
{
#ifdef THREADS_SDL
	SDL_mutex* sdl;
#else
	std::recursive_mutex        m_mutex;
#endif
};

struct CondVar::CondVarPrivate
{
#ifdef THREADS_SDL
	SDL_cond* sdl;
#else
	std::condition_variable_any m_cv;
#endif
};

static thread_id_t      g_main_thread;
static int              g_main_thread_int;
static std::atomic<int> g_thread_counter = 0;

KYTY_SUBSYSTEM_INIT(Threads)
{
#ifdef THREADS_SDL
	g_main_thread = SDL_ThreadID();
#else
	g_main_thread     = std::this_thread::get_id();
	g_main_thread_int = Thread::GetThreadIdUnique();
#endif
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Threads) {}

KYTY_SUBSYSTEM_DESTROY(Threads) {}

Thread::Thread(thread_func_t func, void* arg): m_thread(new ThreadPrivate(func, arg)) // @suppress("Symbol is not resolved")
{
}

Thread::~Thread()
{
	EXIT_IF(!m_thread->finished && !m_thread->auto_delete);

	Delete(m_thread);
}

void Thread::Join()
{
	EXIT_IF(m_thread->finished || m_thread->auto_delete);

#ifdef THREADS_SDL
	int status = -1;
	SDL_WaitThread(m_thread->sdl, &status);
	EXIT_IF(status != 0);
#else
	m_thread->m_thread.join();
#endif

	m_thread->finished = true;
}

void Thread::Detach()
{
	EXIT_IF(m_thread->finished || m_thread->auto_delete);

	m_thread->auto_delete = true;
#ifdef THREADS_SDL
	SDL_DetachThread(m_thread->sdl);
#else
	m_thread->m_thread.detach();
#endif
}

void Thread::Sleep(uint32_t millis)
{
#ifdef THREADS_SDL
	SDL_Delay(millis);
#else
	std::this_thread::sleep_for(std::chrono::milliseconds(millis));
#endif
}

void Thread::SleepMicro(uint32_t micros)
{
#ifdef THREADS_SDL
	SDL_Delay(micros / 1000);
#else
	std::this_thread::sleep_for(std::chrono::microseconds(micros));
#endif
}

void Thread::SleepNano(uint64_t nanos)
{
#ifdef THREADS_SDL
	SDL_Delay(nanos / 1000000);
#else
	std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
#endif
}

bool Thread::IsMainThread()
{
#ifdef THREADS_SDL
	return g_main_thread == thread_id_t(SDL_ThreadID());
#else
	return g_main_thread == std::this_thread::get_id();
#endif
}

String Thread::GetId() const
{
#ifdef THREADS_SDL
	return String::FromPrintf("%" PRIu64, (uint64_t)SDL_GetThreadID(m_thread->sdl));
#else
	std::stringstream ss;
	ss << m_thread->m_thread.get_id();
	return String::FromUtf8(ss.str().c_str());
#endif
}

String Thread::GetThreadId()
{
#ifdef THREADS_SDL
	return String::FromPrintf("%" PRIu64, (uint64_t)SDL_ThreadID());
#else
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return String::FromUtf8(ss.str().c_str());
#endif
}

Mutex::Mutex(): m_mutex(new MutexPrivate)
{
#ifdef THREADS_SDL
	m_mutex->sdl = SDL_CreateMutex();
	EXIT_IF(!m_mutex->sdl);
#endif
}

Mutex::~Mutex()
{
#ifdef THREADS_SDL
	SDL_DestroyMutex(m_mutex->sdl);
#endif
	Delete(m_mutex);
}

void Mutex::Lock()
{
#ifdef THREADS_SDL
	SDL_LockMutex(m_mutex->sdl);
#else
	m_mutex->m_mutex.lock();
#endif
}

void Mutex::Unlock()
{
#ifdef THREADS_SDL
	SDL_UnlockMutex(m_mutex->sdl);
#else
	m_mutex->m_mutex.unlock();
#endif
}

bool Mutex::TryLock()
{
#ifdef THREADS_SDL
	int status = SDL_TryLockMutex(m_mutex->sdl);
	if (status == 0)
	{
		return true;
	}
	return false;
#else
	return m_mutex->m_mutex.try_lock();
#endif
}

CondVar::CondVar(): m_cond_var(new CondVarPrivate)
{
#ifdef THREADS_SDL
	m_cond_var->sdl = SDL_CreateCond();
	EXIT_IF(!m_cond_var->sdl);
#endif
}

CondVar::~CondVar()
{
#ifdef THREADS_SDL
	SDL_DestroyCond(m_cond_var->sdl);
#endif
	Delete(m_cond_var);
}

void CondVar::Wait(Mutex* mutex)
{
#ifdef THREADS_SDL
	SDL_CondWait(m_cond_var->sdl, mutex->m_mutex->sdl);
#else
	std::unique_lock<std::recursive_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
	m_cond_var->m_cv.wait(cpp_lock);
	cpp_lock.release();
#endif
}

void CondVar::WaitFor(Mutex* mutex, uint32_t micros)
{
#ifdef THREADS_SDL
	SDL_CondWaitTimeout(m_cond_var->sdl, mutex->m_mutex->sdl, (micros < 1000 ? 1 : micros / 1000));
#else
	std::unique_lock<std::recursive_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
	m_cond_var->m_cv.wait_for(cpp_lock, std::chrono::microseconds(micros));
	cpp_lock.release();
#endif
}

void CondVar::Signal()
{
#ifdef THREADS_SDL
	SDL_CondSignal(m_cond_var->sdl);
#else
	m_cond_var->m_cv.notify_one();
#endif
}

void CondVar::SignalAll()
{
#ifdef THREADS_SDL
	SDL_CondBroadcast(m_cond_var->sdl);
#else
	m_cond_var->m_cv.notify_all();
#endif
}

int Thread::GetThreadIdUnique()
{
	static thread_local int tid = ++g_thread_counter;
	return tid;
}

} // namespace Kyty::Core
