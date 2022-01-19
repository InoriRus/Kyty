#ifndef INCLUDE_KYTY_CORE_THREADS_H_
#define INCLUDE_KYTY_CORE_THREADS_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Subsystems.h"

namespace Kyty::Core {

KYTY_SUBSYSTEM_DEFINE(Threads);

using thread_func_t = void (*)(void*);

struct ThreadPrivate;
struct MutexPrivate;
struct CondVarPrivate;
class String;

class Thread
{
public:
	Thread(thread_func_t func, void* arg);
	virtual ~Thread();

	void Join();
	void Detach();

	// Once a thread has finished, the id may be reused by another thread.
	[[nodiscard]] String GetId() const;

	// The id is unique and can't be reused by another thread.
	[[nodiscard]] int GetUniqueId() const;

	static void Sleep(uint32_t millis);
	static void SleepMicro(uint32_t micros);
	static void SleepNano(uint64_t nanos);
	static bool IsMainThread();

	// Get current thread id
	// Once a thread has finished, the id may be reused by another thread.
	static String GetThreadId();

	// Get current thread id
	// The id is unique and can't be reused by another thread.
	static int GetThreadIdUnique();

	KYTY_CLASS_NO_COPY(Thread);

private:
	ThreadPrivate* m_thread;
};

class Mutex
{
public:
	Mutex();
	virtual ~Mutex();

	void Lock();
	void Unlock();
	bool TryLock();

	friend class CondVar;

	KYTY_CLASS_NO_COPY(Mutex);

private:
	MutexPrivate* m_mutex;
};

class DummyMutex final
{
public:
	DummyMutex()  = default;
	~DummyMutex() = default;

	void Lock() {}
	void Unlock() {}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	bool TryLock() { return false; }

	friend class CondVar;

	KYTY_CLASS_NO_COPY(DummyMutex);

private:
};

class CondVar
{
public:
	CondVar();
	virtual ~CondVar();

	void Wait(Mutex* mutex);
	void WaitFor(Mutex* mutex, uint32_t micros);
	void Signal();
	void SignalAll();

	KYTY_CLASS_NO_COPY(CondVar);

private:
	CondVarPrivate* m_cond_var;
};

class LockGuard
{
public:
	using mutex_type = Mutex;

	explicit LockGuard(mutex_type& m): m_mutex(m) { m_mutex.Lock(); }

	~LockGuard() { m_mutex.Unlock(); }

	KYTY_CLASS_NO_COPY(LockGuard);

private:
	mutex_type& m_mutex;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_THREADS_H_ */
