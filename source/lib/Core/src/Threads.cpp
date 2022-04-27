#include "Kyty/Core/Threads.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Debug.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

//#define KYTY_DEBUG_LOCKS
//#define KYTY_DEBUG_LOCKS_TIMED

namespace Kyty::Core {

using thread_id_t = std::thread::id;

#if defined(KYTY_DEBUG_LOCKS) || defined(KYTY_DEBUG_LOCKS_TIMED)
constexpr auto DBG_TRY_SECONDS = std::chrono::seconds(15);
#endif

struct MutexPrivate
{
#if defined(KYTY_DEBUG_LOCKS) || defined(KYTY_DEBUG_LOCKS_TIMED)
	std::recursive_timed_mutex m_mutex;
#else
	std::recursive_mutex m_mutex;
#endif
};

struct CondVarPrivate
{
	std::condition_variable_any m_cv;
};

class WaitForGraph
{
public:
	using T     = int;
	using R     = MutexPrivate*;
	using Cycle = Vector<int>;

	enum class Link
	{
		None,
		Own,
		Wait,
		CondVar
	};

	WaitForGraph()          = default;
	virtual ~WaitForGraph() = default;

	KYTY_CLASS_NO_COPY(WaitForGraph);

	void          DbgDump(const Vector<Cycle>& list);
	Vector<Cycle> DetectDeadlocks();

	int  Insert(T t, R r, Link link);
	void Update(int index, Link link);
	void DeleteByIndex(int index);
	void Delete(T t, R r, Link link);
	void Delete(T t);
	void Delete(R r);

private:
	void FindCycles(Cycle* c, Vector<Cycle>* list);

	static constexpr int MAX_EDGES = 1024;

	struct Edge
	{
		T          t    = 0;
		R          r    = nullptr;
		Link       link = Link::None;
		uint64_t   time = 0;
		DebugStack stack;
	};

	std::recursive_mutex m_mutex;
	Edge                 m_edges[MAX_EDGES];
	int                  m_edges_num = 0;
	uint64_t             m_time      = 0;
	bool                 m_disabled  = false;
};

static std::atomic<WaitForGraph*> g_wait_for_graph = nullptr;

struct ThreadPrivate
{
	ThreadPrivate(thread_func_t f, void* a): func(f), arg(a), m_thread(&Run, this) {}

	static void Run(ThreadPrivate* t)
	{
		t->unique_id = Thread::GetThreadIdUnique();
		t->started   = true;
		t->func(t->arg);
		if (t->auto_delete)
		{
#ifdef KYTY_DEBUG_LOCKS
			if (g_wait_for_graph != nullptr)
			{
				g_wait_for_graph.load()->Delete(t->unique_id);
			}
#endif
		}
	}

	thread_func_t    func;
	void*            arg;
	std::atomic_bool finished    = false;
	std::atomic_bool auto_delete = false;
	std::atomic_bool started     = false;
	int              unique_id   = 0;
	std::thread      m_thread;
};

static thread_id_t      g_main_thread;
static int              g_main_thread_int;
static std::atomic<int> g_thread_counter = 0;

KYTY_SUBSYSTEM_INIT(Threads)
{
	g_main_thread     = std::this_thread::get_id();
	g_main_thread_int = Thread::GetThreadIdUnique();
	g_wait_for_graph  = new WaitForGraph;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Threads) {}

KYTY_SUBSYSTEM_DESTROY(Threads) {}

void WaitForGraph::DbgDump(const Vector<Cycle>& list)
{
	std::lock_guard lock(m_mutex);

	m_disabled = true;

	Vector<int> indices;

	for (const auto& c: list)
	{
		printf("cycle: ");
		for (int n: c)
		{
			printf("%d ", n);
			indices.Add(n);
		}
		printf("\n");
	}

	for (int index = 0; index < m_edges_num; index++)
	{
		auto& e = m_edges[index];
		if (e.link != Link::None && (indices.IsEmpty() || indices.Contains(index)))
		{
			printf("\n[%d] thread = %d, mutex = %" PRIx64 ", link = %s, time = %" PRIu64 "\n\n", index, e.t,
			       reinterpret_cast<uint64_t>(e.r), Core::EnumName(e.link).C_Str(), e.time);
			e.stack.Print(0);
		}
	}

	m_disabled = false;
}

Vector<WaitForGraph::Cycle> WaitForGraph::DetectDeadlocks()
{
	std::lock_guard lock(m_mutex);

	m_disabled = true;

	Cycle         c;
	Vector<Cycle> list;

	FindCycles(&c, &list);

	m_disabled = false;

	return list;
}

void WaitForGraph::FindCycles(Cycle* c, Vector<Cycle>* list)
{
	uint32_t size = c->Size();
	int      last = -1;
	if (size > 0)
	{
		last = c->At(size - 1);
		for (uint32_t i = 0; i < size - 1; i++)
		{
			if (c->At(i) == last)
			{
				list->Add(*c);
				return;
			}
		}
	}

	Cycle       nc = *c;
	const auto* l  = (last >= 0 ? &m_edges[last] : nullptr);
	for (int index = 0; index < m_edges_num; index++)
	{
		const auto& e = m_edges[index];
		if (e.link != Link::None && (last < 0 || (l->link == Link::Own && l->r == e.r && e.link == Link::Wait) ||
		                             (l->link == Link::Wait && l->t == e.t && e.link == Link::Own)))
		{
			nc.Add(index);
			FindCycles(&nc, list);
			nc.RemoveAt(size);
		}
	}
}

int WaitForGraph::Insert(T t, R r, Link link)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return -1;
	}

	Edge* edge  = nullptr;
	int   index = 0;
	for (index = 0; index < m_edges_num; index++)
	{
		auto& e = m_edges[index];
		if (e.link == Link::None)
		{
			edge = &e;
			break;
		}
	}

	if (edge == nullptr)
	{
		if (m_edges_num < MAX_EDGES)
		{
			edge = &m_edges[index];
			m_edges_num++;
		} else
		{
			return -1;
		}
	}

	edge->t    = t;
	edge->r    = r;
	edge->link = link;
	edge->time = ++m_time;
	DebugStack::Trace(&edge->stack);

	return index;
}

void WaitForGraph::Update(int index, Link link)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return;
	}

	if (index >= 0 && index < MAX_EDGES)
	{
		m_edges[index].link = link;
		m_edges[index].time = ++m_time;
		DebugStack::Trace(&m_edges[index].stack);
	}
}

void WaitForGraph::DeleteByIndex(int index)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return;
	}

	if (index >= 0 && index < MAX_EDGES)
	{
		m_edges[index].link = Link::None;
	}
}

void WaitForGraph::Delete(T t, R r, Link l)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return;
	}

	uint64_t max_time  = 0;
	int      max_index = -1;
	for (int index = 0; index < m_edges_num; index++)
	{
		auto& e = m_edges[index];
		if (e.t == t && e.r == r && e.link == l && e.time > max_time)
		{
			max_time  = e.time;
			max_index = index;
		}
	}

	if (max_index >= 0)
	{
		m_edges[max_index].link = Link::None;
		if (max_index == m_edges_num - 1)
		{
			m_edges_num--;
		}
	}
}

void WaitForGraph::Delete(T t)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return;
	}

	for (int index = 0; index < m_edges_num; index++)
	{
		auto& e = m_edges[index];
		if (e.t == t && e.link != Link::None)
		{
			e.link = Link::None;
		}
	}
}

void WaitForGraph::Delete(R r)
{
	std::lock_guard lock(m_mutex);

	if (m_disabled)
	{
		return;
	}

	for (int index = 0; index < m_edges_num; index++)
	{
		auto& e = m_edges[index];
		if (e.r == r && e.link != Link::None)
		{
			e.link = Link::None;
		}
	}
}

Thread::Thread(thread_func_t func, void* arg): m_thread(new ThreadPrivate(func, arg))
{
	while (!m_thread->started)
	{
		Core::Thread::SleepMicro(1000);
	}
}

Thread::~Thread()
{
	EXIT_IF(!m_thread->finished && !m_thread->auto_delete);

	if (m_thread->finished)
	{
#ifdef KYTY_DEBUG_LOCKS
		if (g_wait_for_graph != nullptr)
		{
			g_wait_for_graph.load()->Delete(m_thread->unique_id);
		}
#endif
	}

	Delete(m_thread);
}

void Thread::Join()
{
	EXIT_IF(m_thread->finished || m_thread->auto_delete);

	m_thread->m_thread.join();

	m_thread->finished = true;
}

void Thread::Detach()
{
	EXIT_IF(m_thread->finished || m_thread->auto_delete);

	m_thread->auto_delete = true;
	m_thread->m_thread.detach();
}

void Thread::Sleep(uint32_t millis)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(millis));
}

void Thread::SleepMicro(uint32_t micros)
{
	std::this_thread::sleep_for(std::chrono::microseconds(micros));
}

void Thread::SleepNano(uint64_t nanos)
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(nanos));
}

bool Thread::IsMainThread()
{
	return g_main_thread == std::this_thread::get_id();
}

String Thread::GetId() const
{
	std::stringstream ss;
	ss << m_thread->m_thread.get_id();
	return String::FromUtf8(ss.str().c_str());
}

int Thread::GetUniqueId() const
{
	return m_thread->unique_id;
}

String Thread::GetThreadId()
{
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return String::FromUtf8(ss.str().c_str());
}

Mutex::Mutex(): m_mutex(new MutexPrivate) {}

Mutex::~Mutex()
{
#ifdef KYTY_DEBUG_LOCKS
	if (g_wait_for_graph != nullptr)
	{
		g_wait_for_graph.load()->Delete(m_mutex);
	}
#endif
	Delete(m_mutex);
}

void Mutex::Lock()
{
#ifdef KYTY_DEBUG_LOCKS
	if (g_wait_for_graph != nullptr)
	{
		int  index  = g_wait_for_graph.load()->Insert(Thread::GetThreadIdUnique(), m_mutex, WaitForGraph::Link::Wait);
		bool locked = false;
		do
		{
			locked = m_mutex->m_mutex.try_lock_for(DBG_TRY_SECONDS);

			if (!locked)
			{
				if (auto list = g_wait_for_graph.load()->DetectDeadlocks(); !list.IsEmpty())
				{
					g_wait_for_graph.load()->DbgDump(list);
					EXIT("deadlock!");
				}
			}
		} while (!locked);
		g_wait_for_graph.load()->Update(index, WaitForGraph::Link::Own);
	} else
	{
		m_mutex->m_mutex.lock();
	}
#else
#ifdef KYTY_DEBUG_LOCKS_TIMED
	bool                 locked = false;
	do
	{
		locked = m_mutex->m_mutex.try_lock_for(DBG_TRY_SECONDS);

		if (!locked)
		{
			EXIT("lock timeout!");
		}
	} while (!locked);
#else
	m_mutex->m_mutex.lock();
#endif
#endif
}

void Mutex::Unlock()
{
#ifdef KYTY_DEBUG_LOCKS
	if (g_wait_for_graph != nullptr)
	{
		g_wait_for_graph.load()->Delete(Thread::GetThreadIdUnique(), m_mutex, WaitForGraph::Link::Own);
	}
	m_mutex->m_mutex.unlock();
#else
	m_mutex->m_mutex.unlock();
#endif
}

bool Mutex::TryLock()
{
#ifdef KYTY_DEBUG_LOCKS
	if (m_mutex->m_mutex.try_lock())
	{
		if (g_wait_for_graph != nullptr)
		{
			g_wait_for_graph.load()->Insert(Thread::GetThreadIdUnique(), m_mutex, WaitForGraph::Link::Own);
		}
		return true;
	}
	return false;
#else
	return m_mutex->m_mutex.try_lock();
#endif
}

CondVar::CondVar(): m_cond_var(new CondVarPrivate) {}

CondVar::~CondVar()
{
	Delete(m_cond_var);
}

void CondVar::Wait(Mutex* mutex)
{
#if defined(KYTY_DEBUG_LOCKS) || defined(KYTY_DEBUG_LOCKS_TIMED)
	std::unique_lock<std::recursive_timed_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
#else
	std::unique_lock<std::recursive_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
#endif
#ifdef KYTY_DEBUG_LOCKS
	if (g_wait_for_graph != nullptr)
	{
		g_wait_for_graph.load()->Delete(Thread::GetThreadIdUnique(), mutex->m_mutex, WaitForGraph::Link::Own);
		int index = g_wait_for_graph.load()->Insert(Thread::GetThreadIdUnique(), mutex->m_mutex, WaitForGraph::Link::CondVar);
		m_cond_var->m_cv.wait(cpp_lock);
		g_wait_for_graph.load()->DeleteByIndex(index);
		g_wait_for_graph.load()->Insert(Thread::GetThreadIdUnique(), mutex->m_mutex, WaitForGraph::Link::Own);
	} else
	{
		m_cond_var->m_cv.wait(cpp_lock);
	}
#else
	m_cond_var->m_cv.wait(cpp_lock);
#endif
	cpp_lock.release();
}

void CondVar::WaitFor(Mutex* mutex, uint32_t micros)
{
#if defined(KYTY_DEBUG_LOCKS) || defined(KYTY_DEBUG_LOCKS_TIMED)
	std::unique_lock<std::recursive_timed_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
#else
	std::unique_lock<std::recursive_mutex> cpp_lock(mutex->m_mutex->m_mutex, std::adopt_lock_t());
#endif
	m_cond_var->m_cv.wait_for(cpp_lock, std::chrono::microseconds(micros));
	cpp_lock.release();
}

void CondVar::Signal()
{
	m_cond_var->m_cv.notify_one();
}

void CondVar::SignalAll()
{
	m_cond_var->m_cv.notify_all();
}

int Thread::GetThreadIdUnique()
{
	static thread_local int tid = ++g_thread_counter;
	return tid;
}

} // namespace Kyty::Core
