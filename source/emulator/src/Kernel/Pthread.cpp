#include "Emulator/Kernel/Pthread.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Singleton.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Timer.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/RuntimeLinker.h"
#include "Emulator/Loader/Timer.h"

#include <atomic>
#include <cerrno>
#include <ctime>

#ifdef KYTY_EMU_ENABLED

#include <pthread.h>
#include <pthread_time.h>

namespace Kyty::Libs {

namespace LibKernel {

LIB_NAME("libkernel", "libkernel");

constexpr int KEYS_MAX              = 256;
constexpr int DESTRUCTOR_ITERATIONS = 4;

struct PthreadMutexPrivate
{
	uint8_t         reserved[256];
	String          name;
	pthread_mutex_t p;
};

struct PthreadMutexattrPrivate
{
	uint8_t             reserved[64];
	pthread_mutexattr_t p;
	int                 pprotocol;
};

struct PthreadAttrPrivate
{
	uint8_t        reserved[64];
	KernelCpumask  affinity;
	size_t         guard_size;
	int            policy;
	bool           detached;
	pthread_attr_t p;
};

struct PthreadPrivate
{
	uint8_t              reserved[4096];
	String               name;
	pthread_t            p;
	PthreadAttr          attr;
	pthread_entry_func_t entry;
	void*                arg;
	int                  unique_id;
	std::atomic_bool     started;
	std::atomic_bool     detached;
	std::atomic_bool     almost_done;
	std::atomic_bool     free;
};

struct PthreadRwlockPrivate
{
	uint8_t          reserved[256];
	String           name;
	pthread_rwlock_t p;
};

struct PthreadRwlockattrPrivate
{
	uint8_t              reserved[64];
	int                  type;
	pthread_rwlockattr_t p;
};

struct PthreadCondattrPrivate
{
	uint8_t            reserved[64];
	pthread_condattr_t p;
};

struct PthreadCondPrivate
{
	uint8_t        reserved[256];
	String         name;
	pthread_cond_t p;
};

struct PthreadStaticObject
{
	enum class Type
	{
		Mutex,
		Cond,
		Rwlock
	};

	Type             type;
	uint64_t         vaddr;
	Loader::Program* program;
};

class PthreadStaticObjects
{
public:
	PthreadStaticObjects() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~PthreadStaticObjects() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(PthreadStaticObjects);

	void* CreateObject(void* addr, PthreadStaticObject::Type type);
	void  DeleteObjects(Loader::Program* program);

private:
	Vector<PthreadStaticObject*> m_objects;
	Core::Mutex                  m_mutex;
};

class PthreadKeys
{
public:
	PthreadKeys() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~PthreadKeys() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(PthreadKeys);

	bool Create(int* key, pthread_key_destructor_func_t destructor);
	bool Delete(int key);
	void Destruct(int thread_id);
	bool Set(int key, int thread_id, void* data);
	bool Get(int key, int thread_id, void** data);

private:
	struct Map
	{
		int   thread_id = -1;
		void* data      = nullptr;
	};

	struct Key
	{
		bool                          used       = false;
		pthread_key_destructor_func_t destructor = nullptr;
		Vector<Map>                   specific_values;
	};

	Core::Mutex m_mutex;
	Key         m_keys[KEYS_MAX];
};

class PthreadPool
{
public:
	PthreadPool() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~PthreadPool() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(PthreadPool);

	Pthread Create();

	void FreeDetachedThreads();

private:
	Vector<Pthread> m_threads;
	Core::Mutex     m_mutex;
};

class PThreadContext
{
public:
	PThreadContext() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~PThreadContext() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(PThreadContext);

	PthreadAttr*          GetDefaultAttr() { return &m_default_attr; }
	void                  SetDefaultAttr(PthreadAttr attr) { m_default_attr = attr; }
	PthreadCondattr*      GetDefaultCondattr() { return &m_default_condattr; }
	void                  SetDefaultCondattr(PthreadCondattr attr) { m_default_condattr = attr; }
	PthreadMutexattr*     GetDefaultMutexattr() { return &m_default_mutexattr; }
	void                  SetDefaultMutexattr(PthreadMutexattr attr) { m_default_mutexattr = attr; }
	PthreadRwlockattr*    GetDefaultRwlockattr() { return &m_default_rwlockattr; }
	void                  SetDefaultRwlockattr(PthreadRwlockattr attr) { m_default_rwlockattr = attr; }
	PthreadPool*          GetPthreadPool() { return m_pthread_pool; }
	void                  SetPthreadPool(PthreadPool* pool) { m_pthread_pool = pool; }
	PthreadStaticObjects* GetPthreadStaticObjects() { return m_pthread_static_objects; }
	void                  SetPthreadStaticObjects(PthreadStaticObjects* objs) { m_pthread_static_objects = objs; }
	PthreadKeys*          GetPthreadKeys() { return m_pthread_keys; }
	void                  SetPthreadKeys(PthreadKeys* keys) { m_pthread_keys = keys; }

	[[nodiscard]] thread_dtors_func_t GetThreadDtors() const { return m_thread_dtors; }
	void                              SetThreadDtors(thread_dtors_func_t dtors) { m_thread_dtors = dtors; }

private:
	// Core::Mutex           m_mutex;
	PthreadMutexattr      m_default_mutexattr      = nullptr;
	PthreadRwlockattr     m_default_rwlockattr     = nullptr;
	PthreadCondattr       m_default_condattr       = nullptr;
	PthreadAttr           m_default_attr           = nullptr;
	PthreadPool*          m_pthread_pool           = nullptr;
	PthreadStaticObjects* m_pthread_static_objects = nullptr;
	PthreadKeys*          m_pthread_keys           = nullptr;

	std::atomic<thread_dtors_func_t> m_thread_dtors = nullptr;
};

thread_local Pthread g_pthread_self    = nullptr;
PThreadContext*      g_pthread_context = nullptr;

static void FreeDetachedThreads(void* /*arg*/)
{
	PRINT_NAME_ENABLE(false);

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_pool = g_pthread_context->GetPthreadPool();

	EXIT_IF(pthread_pool == nullptr);

	while (true)
	{
		Core::Thread::Sleep(10000);
		pthread_pool->FreeDetachedThreads();
	}
}

void PthreadDeleteStaticObjects(Loader::Program* program)
{
	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	pthread_static_objects->DeleteObjects(program);
}

void PthreadInitSelfForMainThread()
{
	EXIT_IF(g_pthread_self != nullptr);

	g_pthread_self = new PthreadPrivate {};
	PthreadAttrInit(&g_pthread_self->attr);
	g_pthread_self->p           = pthread_self();
	g_pthread_self->name        = "MainThread";
	g_pthread_self->unique_id   = Core::Thread::GetThreadIdUnique();
	g_pthread_self->free        = false;
	g_pthread_self->detached    = false;
	g_pthread_self->almost_done = false;
	g_pthread_self->entry       = nullptr;
	g_pthread_self->arg         = nullptr;
}

KYTY_SUBSYSTEM_INIT(Pthread)
{
	PRINT_NAME_ENABLE(false);

	EXIT_IF(g_pthread_context != nullptr);

	g_pthread_context = new PThreadContext;

	g_pthread_context->SetPthreadStaticObjects(new PthreadStaticObjects);
	g_pthread_context->SetPthreadPool(new PthreadPool);
	g_pthread_context->SetPthreadKeys(new PthreadKeys);

	PthreadMutexattr  default_mutexattr  = nullptr;
	PthreadRwlockattr default_rwlockattr = nullptr;
	PthreadCondattr   default_condattr   = nullptr;
	PthreadAttr       default_attr       = nullptr;

	PthreadAttrInit(&default_attr);
	PthreadMutexattrInit(&default_mutexattr);
	PthreadRwlockattrInit(&default_rwlockattr);
	PthreadCondattrInit(&default_condattr);

	g_pthread_context->SetDefaultMutexattr(default_mutexattr);
	g_pthread_context->SetDefaultRwlockattr(default_rwlockattr);
	g_pthread_context->SetDefaultCondattr(default_condattr);
	g_pthread_context->SetDefaultAttr(default_attr);

	PRINT_NAME_ENABLE(true);

	Core::Thread thread(FreeDetachedThreads, nullptr);
	thread.Detach();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Pthread) {}

KYTY_SUBSYSTEM_DESTROY(Pthread) {}

static int pthread_attr_copy(PthreadAttr* dst, const PthreadAttr* src)
{
	if (dst == nullptr || *dst == nullptr || src == nullptr || *src == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	KernelCpumask    mask          = 0;
	int              state         = 0;
	size_t           guard_size    = 0;
	int              inherit_sched = 0;
	KernelSchedParam param         = {};
	int              policy        = 0;
	void*            stack_addr    = nullptr;
	size_t           stack_size    = 0;

	int result = 0;

	result = (result == 0 ? PthreadAttrGetaffinity(src, &mask) : result);
	result = (result == 0 ? PthreadAttrGetdetachstate(src, &state) : result);
	result = (result == 0 ? PthreadAttrGetguardsize(src, &guard_size) : result);
	result = (result == 0 ? PthreadAttrGetinheritsched(src, &inherit_sched) : result);
	result = (result == 0 ? PthreadAttrGetschedparam(src, &param) : result);
	result = (result == 0 ? PthreadAttrGetschedpolicy(src, &policy) : result);
	result = (result == 0 ? PthreadAttrGetstackaddr(src, &stack_addr) : result);
	result = (result == 0 ? PthreadAttrGetstacksize(src, &stack_size) : result);

	result = (result == 0 ? PthreadAttrSetaffinity(dst, mask) : result);
	result = (result == 0 ? PthreadAttrSetdetachstate(dst, state) : result);
	result = (result == 0 ? PthreadAttrSetguardsize(dst, guard_size) : result);
	result = (result == 0 ? PthreadAttrSetinheritsched(dst, inherit_sched) : result);
	result = (result == 0 ? PthreadAttrSetschedparam(dst, &param) : result);
	result = (result == 0 ? PthreadAttrSetschedpolicy(dst, policy) : result);
	if (stack_addr != nullptr)
	{
		result = (result == 0 ? PthreadAttrSetstackaddr(dst, stack_addr) : result);
	}
	if (stack_size != 0)
	{
		result = (result == 0 ? PthreadAttrSetstacksize(dst, stack_size) : result);
	}

	return result;
}

static void pthread_attr_dbg_print(const PthreadAttr* src)
{
	KernelCpumask    mask          = 0;
	int              state         = 0;
	size_t           guard_size    = 0;
	int              inherit_sched = 0;
	KernelSchedParam param         = {};
	int              policy        = 0;
	void*            stack_addr    = nullptr;
	size_t           stack_size    = 0;

	PthreadAttrGetaffinity(src, &mask);
	PthreadAttrGetdetachstate(src, &state);
	PthreadAttrGetguardsize(src, &guard_size);
	PthreadAttrGetinheritsched(src, &inherit_sched);
	PthreadAttrGetschedparam(src, &param);
	PthreadAttrGetschedpolicy(src, &policy);
	PthreadAttrGetstackaddr(src, &stack_addr);
	PthreadAttrGetstacksize(src, &stack_size);

	printf("\tcpu_mask       = 0x%" PRIx64 "\n", mask);
	printf("\tdetach_state   = %d\n", state);
	printf("\tguard_size     = %" PRIu64 "\n", guard_size);
	printf("\tinherit_sched  = %d\n", inherit_sched);
	printf("\tsched_priority = %d\n", param.sched_priority);
	printf("\tpolicy         = %d\n", policy);
	printf("\tstack_addr     = 0x%016" PRIx64 "\n", reinterpret_cast<uint64_t>(stack_addr));
	printf("\tstack_size    = %" PRIu64 "\n", reinterpret_cast<uint64_t>(stack_size));
}

static void usec_to_timespec(struct timespec* ts, KernelUseconds usec)
{
	ts->tv_sec  = usec / 1000000;
	ts->tv_nsec = static_cast<decltype(ts->tv_nsec)>((usec % 1000000) * 1000);
}

static void sec_to_timeval(KernelTimeval* ts, double sec)
{
	ts->tv_sec  = static_cast<int64_t>(sec);
	ts->tv_usec = static_cast<int64_t>((sec - static_cast<double>(ts->tv_sec)) * 1000000.0);
}

static void sec_to_timespec(KernelTimespec* ts, double sec)
{
	ts->tv_sec  = static_cast<int64_t>(sec);
	ts->tv_nsec = static_cast<int64_t>((sec - static_cast<double>(ts->tv_sec)) * 1000000000.0);
}

void* PthreadStaticObjects::CreateObject(void* addr, PthreadStaticObject::Type type)
{
	Core::LockGuard lock(m_mutex);

	if (addr == nullptr || *static_cast<void**>(addr) != nullptr)
	{
		return addr;
	}

	auto* rt      = Core::Singleton<Loader::RuntimeLinker>::Instance();
	auto  vaddr   = reinterpret_cast<uint64_t>(addr);
	auto* program = rt->FindProgramByAddr(vaddr);

	EXIT_NOT_IMPLEMENTED(program == nullptr);

	auto* obj    = new PthreadStaticObject;
	obj->program = program;
	obj->type    = type;
	obj->vaddr   = vaddr;

	String name = String::FromPrintf("Static%016" PRIx64, vaddr);

	int result = OK;
	switch (type)
	{
		case PthreadStaticObject::Type::Mutex: result = PthreadMutexInit(static_cast<PthreadMutex*>(addr), nullptr, name.C_Str()); break;
		case PthreadStaticObject::Type::Cond: result = PthreadCondInit(static_cast<PthreadCond*>(addr), nullptr, name.C_Str()); break;
		case PthreadStaticObject::Type::Rwlock: result = PthreadRwlockInit(static_cast<PthreadRwlock*>(addr), nullptr, name.C_Str()); break;
		default: EXIT("unknown type: %d\n", static_cast<int>(type));
	}

	EXIT_NOT_IMPLEMENTED(result != OK);

	auto index = m_objects.Find(nullptr);

	if (m_objects.IndexValid(index))
	{
		m_objects[index] = obj;
	} else
	{
		m_objects.Add(obj);
	}

	return addr;
}

void PthreadStaticObjects::DeleteObjects(Loader::Program* program)
{
	Core::LockGuard lock(m_mutex);

	for (auto& obj: m_objects)
	{
		if (obj != nullptr && obj->program == program)
		{
			int result = OK;
			switch (obj->type)
			{
				case PthreadStaticObject::Type::Mutex: result = PthreadMutexDestroy(reinterpret_cast<PthreadMutex*>(obj->vaddr)); break;
				case PthreadStaticObject::Type::Cond: result = PthreadCondDestroy(reinterpret_cast<PthreadCond*>(obj->vaddr)); break;
				case PthreadStaticObject::Type::Rwlock: result = PthreadRwlockDestroy(reinterpret_cast<PthreadRwlock*>(obj->vaddr)); break;
				default: EXIT("unknown type: %d\n", static_cast<int>(obj->type));
			}

			EXIT_NOT_IMPLEMENTED(result != OK);

			delete obj;
			obj = nullptr;
		}
	}
}

Pthread PthreadPool::Create()
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_threads)
	{
		if (p->free)
		{
			p->free = false;
			return p;
		}
	}

	auto* ret = new PthreadPrivate {};

	ret->free        = false;
	ret->detached    = false;
	ret->almost_done = false;
	ret->attr        = nullptr;

	m_threads.Add(ret);

	return ret;
}

void PthreadPool::FreeDetachedThreads()
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_threads)
	{
		if (p->detached && p->almost_done && !p->free)
		{
			PthreadJoin(p, nullptr);
		}
	}
}

bool PthreadKeys::Create(int* key, pthread_key_destructor_func_t destructor)
{
	EXIT_IF(key == nullptr);

	Core::LockGuard lock(m_mutex);

	for (int index = 0; index < KEYS_MAX; index++)
	{
		if (!m_keys[index].used)
		{
			*key                     = index;
			m_keys[index].used       = true;
			m_keys[index].destructor = destructor;
			m_keys[index].specific_values.Clear();
			return true;
		}
	}

	return false;
}

bool PthreadKeys::Delete(int key)
{
	Core::LockGuard lock(m_mutex);

	if (key < 0 || key >= KEYS_MAX || !m_keys[key].used)
	{
		return false;
	}

	m_keys[key].used       = false;
	m_keys[key].destructor = nullptr;
	m_keys[key].specific_values.Clear();

	return true;
}

void PthreadKeys::Destruct(int thread_id)
{
	Core::LockGuard lock(m_mutex);

	struct CallInfo
	{
		pthread_key_destructor_func_t destructor;
		void*                         data;
	};

	for (int iter = 0; iter < DESTRUCTOR_ITERATIONS; iter++)
	{
		Vector<CallInfo> delete_list;

		for (auto& key: m_keys)
		{
			if (key.used && key.destructor != nullptr)
			{
				for (auto& v: key.specific_values)
				{
					if (v.thread_id == thread_id && v.data != nullptr)
					{
						delete_list.Add(CallInfo({key.destructor, v.data}));
					}
				}
			}
		}

		if (delete_list.IsEmpty())
		{
			return;
		}

		for (auto& d: delete_list)
		{
			d.destructor(d.data);
		}
	}
}

bool PthreadKeys::Set(int key, int thread_id, void* data)
{
	Core::LockGuard lock(m_mutex);

	if (key < 0 || key >= KEYS_MAX || !m_keys[key].used)
	{
		return false;
	}

	for (auto& v: m_keys[key].specific_values)
	{
		if (v.thread_id == thread_id)
		{
			v.data = data;
			return true;
		}
	}

	m_keys[key].specific_values.Add(Map({thread_id, data}));

	return true;
}

bool PthreadKeys::Get(int key, int thread_id, void** data)
{
	EXIT_IF(data == nullptr);

	Core::LockGuard lock(m_mutex);

	if (key < 0 || key >= KEYS_MAX || !m_keys[key].used)
	{
		return false;
	}

	for (auto& v: m_keys[key].specific_values)
	{
		if (v.thread_id == thread_id)
		{
			*data = v.data;
			return true;
		}
	}

	*data = nullptr;

	return true;
}

int KYTY_SYSV_ABI PthreadMutexattrInit(PthreadMutexattr* attr)
{
	// PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attr == nullptr);

	*attr = new PthreadMutexattrPrivate {};

	int result = pthread_mutexattr_init(&(*attr)->p);

	result = (result == 0 ? PthreadMutexattrSettype(attr, 1) : result);
	result = (result == 0 ? PthreadMutexattrSetprotocol(attr, 0) : result);

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexattrDestroy(PthreadMutexattr* attr)
{
	// PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attr == nullptr || *attr == nullptr);

	int result = pthread_mutexattr_destroy(&(*attr)->p);

	delete *attr;
	*attr = nullptr;

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexattrSettype(PthreadMutexattr* attr, int type)
{
	// PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attr == nullptr || *attr == nullptr);

	int ptype = PTHREAD_MUTEX_DEFAULT;
	switch (type)
	{
		case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
		case 2: ptype = PTHREAD_MUTEX_RECURSIVE; break;
		case 3:
		case 4: ptype = PTHREAD_MUTEX_NORMAL; break;
		default: EXIT("invalid type: %d\n", type);
	}

	int result = pthread_mutexattr_settype(&(*attr)->p, ptype);

	if (result == 0)
	{
		return OK;
	}

	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadMutexattrSetprotocol([[maybe_unused]] PthreadMutexattr* attr, int protocol)
{
	// PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attr == nullptr || *attr == nullptr);

	[[maybe_unused]] int pprotocol = PTHREAD_PRIO_NONE;
	switch (protocol)
	{
		case 0: pprotocol = PTHREAD_PRIO_NONE; break;
		case 1: pprotocol = PTHREAD_PRIO_INHERIT; break;
		case 2: pprotocol = PTHREAD_PRIO_PROTECT; break;
		default: EXIT("invalid protocol: %d\n", protocol);
	}

	// protocol doesn't work in winpthreads
	int result         = 0; // pthread_mutexattr_setprotocol(&(*attr)->p, pprotocol);
	(*attr)->pprotocol = pprotocol;

	if (result == 0)
	{
		return OK;
	}

	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadMutexInit(PthreadMutex* mutex, const PthreadMutexattr* attr, const char* name)
{
	if (name != nullptr)
	{
		PRINT_NAME();
	}

	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
	if (mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	if (attr == nullptr)
	{
		EXIT_IF(g_pthread_context == nullptr);

		attr = g_pthread_context->GetDefaultMutexattr();
	}

	*mutex = new PthreadMutexPrivate {};

	(*mutex)->name = name;

	int result = pthread_mutex_init(&(*mutex)->p, &(*attr)->p);

	if (name != nullptr)
	{
		printf("\tmutex init: %s, %d\n", (*mutex)->name.C_Str(), result);
	}

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL: return KERNEL_ERROR_EINVAL;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexDestroy(PthreadMutex* mutex)
{
	PRINT_NAME();

	if (mutex == nullptr || *mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_mutex_destroy(&(*mutex)->p);

	printf("\tmutex destroy: %s, %d\n", (*mutex)->name.C_Str(), result);

	delete *mutex;
	*mutex = nullptr;

	switch (result)
	{
		case 0: return OK;
		case EBUSY: return KERNEL_ERROR_EBUSY;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexLock(PthreadMutex* mutex)
{
	// PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	mutex = static_cast<PthreadMutex*>(pthread_static_objects->CreateObject(mutex, PthreadStaticObject::Type::Mutex));

	if (mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*mutex == nullptr);

	int result = pthread_mutex_lock(&(*mutex)->p);

	// printf("\tmutex lock: %s, %d\n", (*mutex)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL: return KERNEL_ERROR_EINVAL;
		case EDEADLK: return KERNEL_ERROR_EDEADLK;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexTrylock(PthreadMutex* mutex)
{
	// PRINT_NAME();

	if (mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*mutex == nullptr);

	int result = pthread_mutex_trylock(&(*mutex)->p);

	// printf("\tmutex trylock: %s, %d\n", (*mutex)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EBUSY: return KERNEL_ERROR_EBUSY;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadMutexUnlock(PthreadMutex* mutex)
{
	// PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	mutex = static_cast<PthreadMutex*>(pthread_static_objects->CreateObject(mutex, PthreadStaticObject::Type::Mutex));

	if (mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*mutex == nullptr);

	int result = pthread_mutex_unlock(&(*mutex)->p);

	// printf("\tmutex unlock: %s, %d\n", (*mutex)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;

		case EINVAL: return KERNEL_ERROR_EINVAL;
		case EPERM: return KERNEL_ERROR_EPERM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadAttrInit(PthreadAttr* attr)
{
	PRINT_NAME();

	*attr = new PthreadAttrPrivate {};

	int result = pthread_attr_init(&(*attr)->p);

	(*attr)->affinity   = 0x7f;
	(*attr)->guard_size = 0x1000;

	KernelSchedParam param;
	param.sched_priority = 700;

	result = (result == 0 ? PthreadAttrSetinheritsched(attr, 4) : result);
	result = (result == 0 ? PthreadAttrSetschedparam(attr, &param) : result);
	result = (result == 0 ? PthreadAttrSetschedpolicy(attr, 1) : result);
	result = (result == 0 ? PthreadAttrSetdetachstate(attr, 0) : result);

	if (PRINT_NAME_ENABLED)
	{
		pthread_attr_dbg_print(attr);
	}

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadAttrDestroy(PthreadAttr* attr)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(attr == nullptr || *attr == nullptr);

	int result = pthread_attr_destroy(&(*attr)->p);

	delete *attr;
	*attr = nullptr;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGet(Pthread thread, PthreadAttr* attr)
{
	PRINT_NAME();

	if (thread == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	return pthread_attr_copy(attr, &thread->attr);
}

int KYTY_SYSV_ABI PthreadAttrGetaffinity(const PthreadAttr* attr, KernelCpumask* mask)
{
	PRINT_NAME();

	if (mask == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	*mask = (*attr)->affinity;

	return OK;
}

int KYTY_SYSV_ABI PthreadAttrGetdetachstate(const PthreadAttr* attr, int* state)
{
	PRINT_NAME();

	if (state == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	// int result = pthread_attr_getdetachstate(&(*attr)->p, state);
	int result = 0;

	*state = ((*attr)->detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);

	switch (*state)
	{
		case PTHREAD_CREATE_JOINABLE: *state = 0; break;
		case PTHREAD_CREATE_DETACHED: *state = 1; break;
		default: EXIT("unknown state: %d\n", *state);
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetguardsize(const PthreadAttr* attr, size_t* guard_size)
{
	PRINT_NAME();

	if (guard_size == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	*guard_size = (*attr)->guard_size;

	return OK;
}

int KYTY_SYSV_ABI PthreadAttrGetinheritsched(const PthreadAttr* attr, int* inherit_sched)
{
	PRINT_NAME();

	if (inherit_sched == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_attr_getinheritsched(&(*attr)->p, inherit_sched);

	switch (*inherit_sched)
	{
		case PTHREAD_EXPLICIT_SCHED: *inherit_sched = 0; break;
		case PTHREAD_INHERIT_SCHED: *inherit_sched = 4; break;
		default: EXIT("unknown inherit_sched: %d\n", *inherit_sched);
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetschedparam(const PthreadAttr* attr, KernelSchedParam* param)
{
	PRINT_NAME();

	if (param == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_attr_getschedparam(&(*attr)->p, param);

	if (param->sched_priority <= -2)
	{
		param->sched_priority = 767;
	} else if (param->sched_priority >= +2)
	{
		param->sched_priority = 256;
	} else
	{
		param->sched_priority = 700;
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetschedpolicy(const PthreadAttr* attr, int* policy)
{
	PRINT_NAME();

	if (policy == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_attr_getschedpolicy(&(*attr)->p, policy);

	switch (*policy)
	{
		case SCHED_OTHER: *policy = (*attr)->policy; break;
		case SCHED_FIFO: *policy = 1; break;
		case SCHED_RR: *policy = 3; break;
		default: EXIT("unknown policy: %d\n", *policy);
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetstack(const PthreadAttr* __restrict attr, void** __restrict stack_addr, size_t* __restrict stack_size)
{
	PRINT_NAME();

	if (stack_size == nullptr || stack_addr == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result1 = pthread_attr_getstackaddr(&(*attr)->p, stack_addr);
	int result2 = pthread_attr_getstacksize(&(*attr)->p, stack_size);

	if (result1 == 0 && result2 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetstackaddr(const PthreadAttr* attr, void** stack_addr)
{
	PRINT_NAME();

	if (stack_addr == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result1 = pthread_attr_getstackaddr(&(*attr)->p, stack_addr);

	if (result1 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrGetstacksize(const PthreadAttr* attr, size_t* stack_size)
{
	PRINT_NAME();

	if (stack_size == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result2 = pthread_attr_getstacksize(&(*attr)->p, stack_size);

	if (result2 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetaffinity(PthreadAttr* attr, KernelCpumask mask)
{
	PRINT_NAME();

	if (attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	(*attr)->affinity = mask;

	return OK;
}

int KYTY_SYSV_ABI PthreadAttrSetdetachstate(PthreadAttr* attr, int state)
{
	PRINT_NAME();

	if (attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int pstate = PTHREAD_CREATE_JOINABLE;
	switch (state)
	{
		case 0: pstate = PTHREAD_CREATE_JOINABLE; break;
		case 1: pstate = PTHREAD_CREATE_DETACHED; break;
		default: EXIT("unknown state: %d\n", state);
	}

	// int result = pthread_attr_setdetachstate(&(*attr)->p, pstate);
	int result = 0;

	(*attr)->detached = (pstate == PTHREAD_CREATE_DETACHED);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetguardsize(PthreadAttr* attr, size_t guard_size)
{
	PRINT_NAME();

	if (attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	(*attr)->guard_size = guard_size;

	return OK;
}

int KYTY_SYSV_ABI PthreadAttrSetinheritsched(PthreadAttr* attr, int inherit_sched)
{
	PRINT_NAME();

	if (attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int pinherit_sched = PTHREAD_INHERIT_SCHED;
	switch (inherit_sched)
	{
		case 0: pinherit_sched = PTHREAD_EXPLICIT_SCHED; break;
		case 4: pinherit_sched = PTHREAD_INHERIT_SCHED; break;
		default: EXIT("unknown inherit_sched: %d\n", inherit_sched);
	}

	int result = pthread_attr_setinheritsched(&(*attr)->p, pinherit_sched);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetschedparam(PthreadAttr* attr, const KernelSchedParam* param)
{
	PRINT_NAME();

	if (param == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	KernelSchedParam pparam {};
	if (param->sched_priority <= 478)
	{
		pparam.sched_priority = +2;
	} else if (param->sched_priority >= 733)
	{
		pparam.sched_priority = -2;
	} else
	{
		pparam.sched_priority = 0;
	}

	int result = pthread_attr_setschedparam(&(*attr)->p, &pparam);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetschedpolicy(PthreadAttr* attr, int policy)
{
	PRINT_NAME();

	if (attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	// winpthreads supports only SCHED_OTHER policy
	int ppolicy = SCHED_OTHER;

	(*attr)->policy = policy;

	int result = pthread_attr_setschedpolicy(&(*attr)->p, ppolicy);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetstack(PthreadAttr* attr, void* addr, size_t size)
{
	PRINT_NAME();

	if (addr == nullptr || size == 0 || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result1 = pthread_attr_setstackaddr(&(*attr)->p, addr);
	int result2 = pthread_attr_setstacksize(&(*attr)->p, size);

	if (result1 == 0 && result2 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetstackaddr(PthreadAttr* attr, void* addr)
{
	PRINT_NAME();

	if (addr == nullptr || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result1 = pthread_attr_setstackaddr(&(*attr)->p, addr);

	if (result1 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadAttrSetstacksize(PthreadAttr* attr, size_t stack_size)
{
	PRINT_NAME();

	if (stack_size == 0 || attr == nullptr || *attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result2 = pthread_attr_setstacksize(&(*attr)->p, stack_size);

	if (result2 == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadRwlockDestroy(PthreadRwlock* rwlock)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_destroy(&(*rwlock)->p);

	printf("\trwlock destroy: %s, %d\n", (*rwlock)->name.C_Str(), result);

	delete *rwlock;
	*rwlock = nullptr;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadRwlockInit(PthreadRwlock* rwlock, const PthreadRwlockattr* attr, const char* name)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	if (attr == nullptr)
	{
		EXIT_IF(g_pthread_context == nullptr);

		attr = g_pthread_context->GetDefaultRwlockattr();
	}

	*rwlock = new PthreadRwlockPrivate {};

	(*rwlock)->name = name;

	int result = pthread_rwlock_init(&(*rwlock)->p, &(*attr)->p);

	printf("\trwlock init: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL: return KERNEL_ERROR_EINVAL;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockRdlock(PthreadRwlock* rwlock)
{
	PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	rwlock = static_cast<PthreadRwlock*>(pthread_static_objects->CreateObject(rwlock, PthreadStaticObject::Type::Rwlock));

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_rdlock(&(*rwlock)->p);

	// printf("\trwlock rdlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockTimedrdlock(PthreadRwlock* rwlock, KernelUseconds usec)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	timespec t {};
	usec_to_timespec(&t, usec);

	int result = pthread_rwlock_timedrdlock(&(*rwlock)->p, &t);

	// printf("\trwlock timedrdlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case ETIMEDOUT: return KERNEL_ERROR_ETIMEDOUT;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockTimedwrlock(PthreadRwlock* rwlock, KernelUseconds usec)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	timespec t {};
	usec_to_timespec(&t, usec);

	int result = pthread_rwlock_timedwrlock(&(*rwlock)->p, &t);

	// printf("\trwlock timedwrlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case ETIMEDOUT: return KERNEL_ERROR_ETIMEDOUT;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockTryrdlock(PthreadRwlock* rwlock)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_tryrdlock(&(*rwlock)->p);

	// printf("\trwlock tryrdlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EBUSY: return KERNEL_ERROR_EBUSY;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockTrywrlock(PthreadRwlock* rwlock)
{
	PRINT_NAME();

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_trywrlock(&(*rwlock)->p);

	// printf("\trwlock trywrlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EBUSY: return KERNEL_ERROR_EBUSY;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockUnlock(PthreadRwlock* rwlock)
{
	// PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	rwlock = static_cast<PthreadRwlock*>(pthread_static_objects->CreateObject(rwlock, PthreadStaticObject::Type::Rwlock));

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_unlock(&(*rwlock)->p);

	// printf("\trwlock unlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;

		case EINVAL: return KERNEL_ERROR_EINVAL;
		case EPERM: return KERNEL_ERROR_EPERM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockWrlock(PthreadRwlock* rwlock)
{
	// PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	rwlock = static_cast<PthreadRwlock*>(pthread_static_objects->CreateObject(rwlock, PthreadStaticObject::Type::Rwlock));

	if (rwlock == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*rwlock == nullptr);

	int result = pthread_rwlock_wrlock(&(*rwlock)->p);

	// printf("\trwlock wrlock: %s, %d\n", (*rwlock)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockattrDestroy(PthreadRwlockattr* attr)
{
	PRINT_NAME();

	if (attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*attr == nullptr);

	int result = pthread_rwlockattr_destroy(&(*attr)->p);

	delete *attr;
	*attr = nullptr;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadRwlockattrInit(PthreadRwlockattr* attr)
{
	PRINT_NAME();

	*attr = new PthreadRwlockattrPrivate {};

	int result = pthread_rwlockattr_init(&(*attr)->p);

	result = (result == 0 ? PthreadRwlockattrSettype(attr, 1) : result);

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadRwlockattrGettype(PthreadRwlockattr* attr, int* type)
{
	PRINT_NAME();

	if (type == nullptr || attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	*type = (*attr)->type;

	return OK;
}

int KYTY_SYSV_ABI PthreadRwlockattrSettype(PthreadRwlockattr* attr, int type)
{
	PRINT_NAME();

	if (attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	(*attr)->type = type;

	return OK;
}

int KYTY_SYSV_ABI PthreadCondattrDestroy(PthreadCondattr* attr)
{
	PRINT_NAME();

	if (attr == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*attr == nullptr);

	int result = pthread_condattr_destroy(&(*attr)->p);

	delete *attr;
	*attr = nullptr;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadCondattrInit(PthreadCondattr* attr)
{
	PRINT_NAME();

	*attr = new PthreadCondattrPrivate {};

	int result = pthread_condattr_init(&(*attr)->p);

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadCondBroadcast(PthreadCond* cond)
{
	PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	cond = static_cast<PthreadCond*>(pthread_static_objects->CreateObject(cond, PthreadStaticObject::Type::Cond));

	if (cond == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);

	int result = pthread_cond_broadcast(&(*cond)->p);

	printf("\tcond broadcast: %s, %d\n", (*cond)->name.C_Str(), result);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadCondDestroy(PthreadCond* cond)
{
	PRINT_NAME();

	if (cond == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);

	int result = pthread_cond_destroy(&(*cond)->p);

	printf("\tcond destroy: %s, %d\n", (*cond)->name.C_Str(), result);

	delete *cond;
	*cond = nullptr;

	switch (result)
	{
		case 0: return OK;
		case EINVAL: return KERNEL_ERROR_EINVAL;
		case EBUSY: return KERNEL_ERROR_EBUSY;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadCondInit(PthreadCond* cond, const PthreadCondattr* attr, const char* name)
{
	PRINT_NAME();

	if (cond == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	if (attr == nullptr)
	{
		EXIT_IF(g_pthread_context == nullptr);

		attr = g_pthread_context->GetDefaultCondattr();
	}

	*cond = new PthreadCondPrivate {};

	(*cond)->name = name;

	int result = pthread_cond_init(&(*cond)->p, &(*attr)->p);

	printf("\tcond init: %s, %d\n", (*cond)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EINVAL: return KERNEL_ERROR_EINVAL;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadCondSignal(PthreadCond* cond)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(cond == nullptr);

	if (cond == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);

	int result = pthread_cond_signal(&(*cond)->p);

	// printf("\tcond signal: %s, %d\n", (*cond)->name.C_Str(), result);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadCondSignalto(PthreadCond* cond, Pthread thread)
{
	PRINT_NAME();

	if (cond == nullptr || thread == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);

	int result = 0;

	KYTY_NOT_IMPLEMENTED;

	// printf("\tcond signalto: %s, %d\n", (*cond)->name.C_Str(), result);

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadCondTimedwait(PthreadCond* cond, PthreadMutex* mutex, KernelUseconds usec)
{
	PRINT_NAME();

	if (cond == nullptr || mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);
	EXIT_NOT_IMPLEMENTED(*mutex == nullptr);

	timespec t {};
	usec_to_timespec(&t, usec);

	int result = pthread_cond_timedwait(&(*cond)->p, &(*mutex)->p, &t);

	// printf("\tcond timedwait: %s, %d\n", (*cond)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case ETIMEDOUT: return KERNEL_ERROR_ETIMEDOUT;
		case EPERM: return KERNEL_ERROR_EPERM;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadCondWait(PthreadCond* cond, PthreadMutex* mutex)
{
	PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_static_objects = g_pthread_context->GetPthreadStaticObjects();

	EXIT_IF(pthread_static_objects == nullptr);

	cond  = static_cast<PthreadCond*>(pthread_static_objects->CreateObject(cond, PthreadStaticObject::Type::Cond));
	mutex = static_cast<PthreadMutex*>(pthread_static_objects->CreateObject(mutex, PthreadStaticObject::Type::Mutex));

	if (cond == nullptr || mutex == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_NOT_IMPLEMENTED(*cond == nullptr);
	EXIT_NOT_IMPLEMENTED(*mutex == nullptr);

	int result = pthread_cond_wait(&(*cond)->p, &(*mutex)->p);

	// printf("\tcond wait: %s, %d\n", (*cond)->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case EPERM: return KERNEL_ERROR_EPERM;
		case EINVAL:
		default: return KERNEL_ERROR_EINVAL;
	}
}

Pthread KYTY_SYSV_ABI PthreadSelf()
{
	// PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(g_pthread_self == nullptr);

	return g_pthread_self;
}

static void cleanup_thread(void* arg)
{
	auto* thread = static_cast<Pthread>(arg);

	EXIT_IF(g_pthread_context == nullptr);

	auto thread_dtors = g_pthread_context->GetThreadDtors();

	if (thread_dtors != nullptr)
	{
		thread_dtors();
	}

	thread->almost_done = true;
}

static void* run_thread(void* arg)
{
	auto* thread = static_cast<Pthread>(arg);
	void* ret    = nullptr;

	thread->unique_id = Core::Thread::GetThreadIdUnique();

	g_pthread_self = thread;

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	pthread_cleanup_push(cleanup_thread, thread);

	thread->started = true;

	ret = thread->entry(thread->arg);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
	pthread_cleanup_pop(1);

	return ret;
}

int KYTY_SYSV_ABI PthreadCreate(Pthread* thread, const PthreadAttr* attr, pthread_entry_func_t entry, void* arg, const char* name)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_IF(g_pthread_context == nullptr);

	auto* pthread_pool = g_pthread_context->GetPthreadPool();

	EXIT_IF(pthread_pool == nullptr);

	if (attr == nullptr)
	{
		attr = g_pthread_context->GetDefaultAttr();
	}

	PRINT_NAME_ENABLE(false);

	*thread = pthread_pool->Create();

	if ((*thread)->attr != nullptr)
	{
		PthreadAttrDestroy(&(*thread)->attr);
	}

	PthreadAttrInit(&(*thread)->attr);

	int result = pthread_attr_copy(&(*thread)->attr, attr);

	if (result == 0)
	{
		EXIT_IF((*thread)->free);

		(*thread)->name        = name;
		(*thread)->entry       = entry;
		(*thread)->arg         = arg;
		(*thread)->almost_done = false;
		(*thread)->detached    = (*attr)->detached;
		(*thread)->started     = false;
		(*thread)->unique_id   = -1;

		result = pthread_create(&(*thread)->p, &(*attr)->p, run_thread, *thread);
	}

	if (result == 0)
	{
		while (!(*thread)->started)
		{
			Core::Thread::SleepMicro(1000);
		}
	}

	printf("\tthread create: %s, id = %d, %d\n", (*thread)->name.C_Str(), (*thread)->unique_id, result);

	pthread_attr_dbg_print(attr);

	PRINT_NAME_ENABLE(true);

	switch (result)
	{
		case 0: return OK;
		case ENOMEM: return KERNEL_ERROR_ENOMEM;
		case EAGAIN: return KERNEL_ERROR_EAGAIN;
		case EDEADLK: return KERNEL_ERROR_EDEADLK;
		case EPERM: return KERNEL_ERROR_EPERM;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadDetach(Pthread thread)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	printf("\tthread detach: %s, %d\n", thread->name.C_Str(), 0);

	thread->detached = true;

	return OK;
}

int KYTY_SYSV_ABI PthreadJoin(Pthread thread, void** value)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_join(thread->p, value);

	if (PRINT_NAME_ENABLED)
	{
		printf("\tthread join: %s, %d\n", thread->name.C_Str(), result);
	}

	int id = thread->unique_id;

	thread->almost_done = false;
	thread->free        = true;

	auto* rt = Core::Singleton<Loader::RuntimeLinker>::Instance();
	rt->DeleteTlss(id);

	g_pthread_context->GetPthreadKeys()->Destruct(id);

	switch (result)
	{
		case 0: return OK;
		case ESRCH: return KERNEL_ERROR_ESRCH;
		case EDEADLK: return KERNEL_ERROR_EDEADLK;
		case EOPNOTSUPP: return KERNEL_ERROR_EOPNOTSUPP;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadCancel(Pthread thread)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	int result = pthread_cancel(thread->p);

	printf("\tthread cancel: %s, %d\n", thread->name.C_Str(), result);

	switch (result)
	{
		case 0: return OK;
		case ESRCH: return KERNEL_ERROR_ESRCH;
		default: return KERNEL_ERROR_EINVAL;
	}
}

int KYTY_SYSV_ABI PthreadSetaffinity(Pthread thread, KernelCpumask mask)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	auto result = PthreadAttrSetaffinity(&thread->attr, mask);

	return result;
}

int KYTY_SYSV_ABI PthreadSetcancelstate(int state, int* old_state)
{
	PRINT_NAME();

	int pstate = PTHREAD_CANCEL_DISABLE;

	switch (state)
	{
		case 0: pstate = PTHREAD_CANCEL_ENABLE; break;
		case 1: pstate = PTHREAD_CANCEL_DISABLE; break;
		default: EXIT("unknown state: %d", state);
	}

	int result = pthread_setcancelstate(pstate, old_state);

	printf("\tthread setcancelstate: %d\n", result);

	switch (*old_state)
	{
		case PTHREAD_CANCEL_ENABLE: *old_state = 0; break;
		case PTHREAD_CANCEL_DISABLE: *old_state = 1; break;
		default: EXIT("unknown old_state: %d", *old_state);
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadSetcanceltype(int type, int* old_type)
{
	PRINT_NAME();

	int ptype = PTHREAD_CANCEL_DEFERRED;

	switch (type)
	{
		case 0: ptype = PTHREAD_CANCEL_DEFERRED; break;
		case 2: ptype = PTHREAD_CANCEL_ASYNCHRONOUS; break;
		default: EXIT("unknown type: %d", type);
	}

	int result = pthread_setcanceltype(ptype, old_type);

	printf("\tthread setcanceltype: %d\n", result);

	switch (*old_type)
	{
		case PTHREAD_CANCEL_DEFERRED: *old_type = 0; break;
		case PTHREAD_CANCEL_ASYNCHRONOUS: *old_type = 2; break;
		default: EXIT("unknown type: %d", *old_type);
	}

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadGetprio(Pthread thread, int* prio)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	EXIT_NOT_IMPLEMENTED(prio == nullptr);

	sched_param param {};
	int         pol = 0;

	int result = pthread_getschedparam(thread->p, &pol, &param);

	if (result == 0)
	{
		if (param.sched_priority <= -2)
		{
			*prio = 767;
		} else if (param.sched_priority >= +2)
		{
			*prio = 256;
		} else
		{
			*prio = 700;
		}

		printf("\t PthreadGetprio: %d, %d\n", thread->unique_id, *prio);

		return OK;
	}

	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI PthreadSetprio(Pthread thread, int prio)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	sched_param param {};
	int         pol = 0;

	int result = pthread_getschedparam(thread->p, &pol, &param);

	if (result == 0)
	{
		if (prio <= 478)
		{
			param.sched_priority = +2;
		} else if (prio >= 733)
		{
			param.sched_priority = -2;
		} else
		{
			param.sched_priority = 0;
		}

		result = pthread_setschedparam(thread->p, pol, &param);

		if (result == 0)
		{
			printf("\t PthreadSetprio: %d, %d\n", thread->unique_id, prio);

			return OK;
		}
	}

	return KERNEL_ERROR_EINVAL;
}

void KYTY_SYSV_ABI PthreadTestcancel()
{
	PRINT_NAME();

	pthread_testcancel();
}

void KYTY_SYSV_ABI PthreadExit(void* value)
{
	PRINT_NAME();

	pthread_exit(value);
}

int KYTY_SYSV_ABI PthreadEqual(Pthread thread1, Pthread thread2)
{
	// PRINT_NAME();

	return (thread1 == thread2 ? 1 : 0);
}

int KYTY_SYSV_ABI PthreadGetname(Pthread thread, char* name)
{
	PRINT_NAME();

	if (thread == nullptr)
	{
		return KERNEL_ERROR_ESRCH;
	}

	if (name == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	strncpy(name, thread->name.C_Str(), 32);
	name[31] = '\0';

	return OK;
}

void KYTY_SYSV_ABI PthreadYield()
{
	PRINT_NAME();

	sched_yield();
}

int KYTY_SYSV_ABI PthreadGetthreadid()
{
	PRINT_NAME();

	return Core::Thread::GetThreadIdUnique();
}

int KYTY_SYSV_ABI KernelClockGetres(KernelClockid clock_id, KernelTimespec* tp)
{
	PRINT_NAME();

	if (tp == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	clockid_t pclock_id = CLOCK_REALTIME;
	switch (clock_id)
	{
		case 0: pclock_id = CLOCK_REALTIME; break;
		case 4: pclock_id = CLOCK_MONOTONIC; break;
		default: EXIT("unknown clock_id: %d", clock_id);
	}

	timespec t {};

	int result = clock_getres(pclock_id, &t);

	tp->tv_sec  = t.tv_sec;
	tp->tv_nsec = t.tv_nsec;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI KernelClockGettime(KernelClockid clock_id, KernelTimespec* tp)
{
	PRINT_NAME();

	if (tp == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	clockid_t pclock_id = CLOCK_REALTIME;
	switch (clock_id)
	{
		case 0: pclock_id = CLOCK_REALTIME; break;
		case 13:
		case 4: pclock_id = CLOCK_MONOTONIC; break;
		default: EXIT("unknown clock_id: %d", clock_id);
	}

	timespec t {};

	int result = clock_gettime(pclock_id, &t);

	tp->tv_sec  = t.tv_sec;
	tp->tv_nsec = t.tv_nsec;

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

int KYTY_SYSV_ABI KernelGettimeofday(KernelTimeval* tp)
{
	PRINT_NAME();

	if (tp == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	// timespec t {};
	// int result = clock_gettime(CLOCK_REALTIME, &t);
	// tp->tv_sec  = t.tv_sec;
	// tp->tv_usec = t.tv_nsec / 1000;

	int  result = 0;
	auto dt     = Core::DateTime::FromSystemUTC();
	sec_to_timeval(tp, dt.ToUnix());

	if (result == 0)
	{
		return OK;
	}
	return KERNEL_ERROR_EINVAL;
}

uint64_t KYTY_SYSV_ABI KernelGetTscFrequency()
{
	return Core::Timer::QueryPerformanceFrequency();
}

uint64_t KYTY_SYSV_ABI KernelReadTsc()
{
	return Core::Timer::QueryPerformanceCounter();
}

uint64_t KYTY_SYSV_ABI KernelGetProcessTime()
{
	return static_cast<uint64_t>(Loader::Timer::GetTimeMs() * 1000.0);
}

uint64_t KYTY_SYSV_ABI KernelGetProcessTimeCounter()
{
	return Loader::Timer::GetCounter();
}

uint64_t KYTY_SYSV_ABI KernelGetProcessTimeCounterFrequency()
{
	return Loader::Timer::GetFrequency();
}

void KYTY_SYSV_ABI KernelSetThreadDtors(thread_dtors_func_t dtors)
{
	PRINT_NAME();

	EXIT_IF(g_pthread_context == nullptr);

	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
	EXIT_NOT_IMPLEMENTED(g_pthread_context->GetThreadDtors() != nullptr);

	g_pthread_context->SetThreadDtors(dtors);
	// g_thread_dtors = dtors;
}

int KYTY_SYSV_ABI KernelUsleep(KernelUseconds microseconds)
{
	PRINT_NAME();
	printf("\tusleep: %u\n", microseconds);
	Core::Timer t;
	t.Start();
	Core::Thread::SleepMicro(microseconds);
	double ts = t.GetTimeS();
	printf("\tactual: %g microseconds\n", ts * 1000000.0);
	return OK;
}

unsigned int KYTY_SYSV_ABI KernelSleep(unsigned int seconds)
{
	PRINT_NAME();
	printf("\tsleep: %u\n", seconds);
	Core::Timer t;
	t.Start();
	Core::Thread::Sleep(seconds);
	double ts = t.GetTimeS();
	printf("\tactual: %g seconds\n", ts);
	return OK;
}

int KYTY_SYSV_ABI KernelNanosleep(const KernelTimespec* rqtp, KernelTimespec* rmtp)
{
	PRINT_NAME();

	if (rqtp == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	if (rqtp->tv_sec < 0 || rqtp->tv_nsec < 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	uint64_t nanos = rqtp->tv_sec * 1000000000 + rqtp->tv_nsec;

	printf("\tnanosleep: %" PRIu64 "\n", nanos);

	Core::Timer t;
	t.Start();
	Core::Thread::SleepNano(nanos);
	double ts = t.GetTimeS();
	printf("\tactual: %g nanoseconds\n", ts * 1000000000.0);

	if (rmtp != nullptr)
	{
		sec_to_timespec(rmtp, ts);
	}

	return OK;
}

int KYTY_SYSV_ABI PthreadKeyCreate(PthreadKey* key, pthread_key_destructor_func_t destructor)
{
	PRINT_NAME();

	if (key == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_IF(g_pthread_context == nullptr || g_pthread_context->GetPthreadKeys() == nullptr);

	if (!g_pthread_context->GetPthreadKeys()->Create(key, destructor))
	{
		return KERNEL_ERROR_EAGAIN;
	}

	printf("\t destructor = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(destructor));
	printf("\t key        = %d\n", *key);

	return OK;
}

int KYTY_SYSV_ABI PthreadKeyDelete(PthreadKey key)
{
	PRINT_NAME();

	printf("\t key = %d\n", key);

	EXIT_IF(g_pthread_context == nullptr || g_pthread_context->GetPthreadKeys() == nullptr);

	if (!g_pthread_context->GetPthreadKeys()->Delete(key))
	{
		return KERNEL_ERROR_EINVAL;
	}

	return OK;
}

int KYTY_SYSV_ABI PthreadSetspecific(PthreadKey key, void* value)
{
	PRINT_NAME();

	int thread_id = Core::Thread::GetThreadIdUnique();

	printf("\t key       = %d\n", key);
	printf("\t thread_id = %d\n", thread_id);
	printf("\t value     = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(value));

	EXIT_IF(g_pthread_context == nullptr || g_pthread_context->GetPthreadKeys() == nullptr);

	if (!g_pthread_context->GetPthreadKeys()->Set(key, thread_id, value))
	{
		return KERNEL_ERROR_EINVAL;
	}

	return OK;
}

void* KYTY_SYSV_ABI PthreadGetspecific(PthreadKey key)
{
	PRINT_NAME();

	int thread_id = Core::Thread::GetThreadIdUnique();

	printf("\t key       = %d\n", key);
	printf("\t thread_id = %d\n", thread_id);

	EXIT_IF(g_pthread_context == nullptr || g_pthread_context->GetPthreadKeys() == nullptr);

	void* value = nullptr;

	if (!g_pthread_context->GetPthreadKeys()->Get(key, thread_id, &value))
	{
		return nullptr;
	}

	printf("\t value     = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(value));

	return value;
}

} // namespace LibKernel

namespace Posix {

LIB_NAME("Posix", "libkernel");

int KYTY_SYSV_ABI pthread_create(LibKernel::Pthread* thread, const LibKernel::PthreadAttr* attr, LibKernel::pthread_entry_func_t entry,
                                 void* arg)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadCreate(thread, attr, entry, arg, ""));
}

int KYTY_SYSV_ABI pthread_join(LibKernel::Pthread thread, void** value)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadJoin(thread, value));
}

int KYTY_SYSV_ABI pthread_cond_broadcast(LibKernel::PthreadCond* cond)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadCondBroadcast(cond));
}

int KYTY_SYSV_ABI pthread_cond_wait(LibKernel::PthreadCond* cond, LibKernel::PthreadMutex* mutex)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadCondWait(cond, mutex));
}

int KYTY_SYSV_ABI pthread_mutex_lock(LibKernel::PthreadMutex* mutex)
{
	// PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexLock(mutex));
}

int KYTY_SYSV_ABI pthread_mutex_trylock(LibKernel::PthreadMutex* mutex)
{
	// PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexTrylock(mutex));
}

int KYTY_SYSV_ABI pthread_mutex_unlock(LibKernel::PthreadMutex* mutex)
{
	// PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexUnlock(mutex));
}

int KYTY_SYSV_ABI pthread_rwlock_rdlock(LibKernel::PthreadRwlock* rwlock)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadRwlockRdlock(rwlock));
}

int KYTY_SYSV_ABI pthread_rwlock_unlock(LibKernel::PthreadRwlock* rwlock)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadRwlockUnlock(rwlock));
}

int KYTY_SYSV_ABI pthread_rwlock_wrlock(LibKernel::PthreadRwlock* rwlock)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadRwlockWrlock(rwlock));
}

int KYTY_SYSV_ABI pthread_key_create(LibKernel::PthreadKey* key, LibKernel::pthread_key_destructor_func_t destructor)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadKeyCreate(key, destructor));
}

int KYTY_SYSV_ABI pthread_key_delete(LibKernel::PthreadKey key)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadKeyDelete(key));
}

int KYTY_SYSV_ABI pthread_setspecific(LibKernel::PthreadKey key, void* value)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadSetspecific(key, value));
}

void* KYTY_SYSV_ABI pthread_getspecific(LibKernel::PthreadKey key)
{
	PRINT_NAME();

	return (LibKernel::PthreadGetspecific(key));
}

int KYTY_SYSV_ABI pthread_mutex_destroy(LibKernel::PthreadMutex* mutex)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexDestroy(mutex));
}

int KYTY_SYSV_ABI pthread_mutex_init(LibKernel::PthreadMutex* mutex, const LibKernel::PthreadMutexattr* attr)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexInit(mutex, attr, nullptr));
}

int KYTY_SYSV_ABI pthread_mutexattr_init(LibKernel::PthreadMutexattr* attr)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexattrInit(attr));
}

int KYTY_SYSV_ABI pthread_mutexattr_settype(LibKernel::PthreadMutexattr* attr, int type)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexattrSettype(attr, type));
}

int KYTY_SYSV_ABI pthread_mutexattr_destroy(LibKernel::PthreadMutexattr* attr)
{
	PRINT_NAME();

	return POSIX_PTHREAD_CALL(LibKernel::PthreadMutexattrDestroy(attr));
}

} // namespace Posix

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
