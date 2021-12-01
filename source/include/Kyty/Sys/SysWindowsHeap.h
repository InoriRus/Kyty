#ifndef SYS_WIN32_INCLUDE_KYTY_SYSHEAP_H_
#define SYS_WIN32_INCLUDE_KYTY_SYSHEAP_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#if 0

#include <windows.h>

namespace Kyty {

typedef HANDLE sys_heap_id_t;


inline sys_heap_id_t sys_heap_create()
{
	HANDLE h = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);

	EXIT_IF(h == NULL);

	return h;
}

inline sys_heap_id_t sys_heap_deafult()
{
	HANDLE h = GetProcessHeap();

	EXIT_IF(h == NULL);

	return h;
}

inline void* sys_heap_alloc(sys_heap_id_t heap_id, size_t size)
{
	void *m = HeapAlloc(heap_id, HEAP_NO_SERIALIZE, size);

	EXIT_IF(m == 0);

	return m;
}

inline void* sys_heap_realloc(sys_heap_id_t heap_id, void *p, size_t size)
{
	void *m = p ? HeapReAlloc(heap_id, HEAP_NO_SERIALIZE, p, size)
			: HeapAlloc(heap_id, HEAP_NO_SERIALIZE, size);

	if (m == 0)
	{
	EXIT_IF(m == 0);
	}

	return m;
}

inline void sys_heap_free(sys_heap_id_t heap_id, void *p)
{
	bool r = HeapFree(heap_id, HEAP_NO_SERIALIZE, p);

	if (!r)
	{
		printf("%" PRIx64"\n", uint64_t(p));
		EXIT_IF(!r);
	}
}

inline sys_heap_id_t sys_heap_create_s()
{
	HANDLE h = HeapCreate(0, 0, 0);

	EXIT_IF(h == NULL);

	return h;
}

inline void* sys_heap_alloc_s(sys_heap_id_t heap_id, size_t size)
{
	void *m = HeapAlloc(heap_id, 0, size);

	EXIT_IF(m == 0);

	return m;
}

inline void* sys_heap_realloc_s(sys_heap_id_t heap_id, void *p, size_t size)
{
	void *m = HeapReAlloc(heap_id, 0, p, size);

	EXIT_IF(m == 0);

	return m;
}

inline void sys_heap_free_s(sys_heap_id_t heap_id, void *p)
{
	bool r = HeapFree(heap_id, 0, p);

	EXIT_IF(!r);
}

} // namespace Kyty

#else

#include "Kyty/Sys/SysSync.h"

namespace Kyty {

using sys_heap_id_t = SysCS *;


inline sys_heap_id_t sys_heap_create()
{
	return nullptr;
}

inline sys_heap_id_t sys_heap_deafult()
{
	return nullptr;
}

inline void* sys_heap_alloc(sys_heap_id_t  /*heap_id*/, size_t size)
{
	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void *m = malloc(size);

	EXIT_IF(m == nullptr);

	return m;
}

inline void* sys_heap_realloc(sys_heap_id_t  /*heap_id*/, void *p, size_t size)
{
	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void *m = p != nullptr ? realloc(p, size) : malloc(size);

	if (m == nullptr)
	{
	EXIT_IF(m == nullptr);
	}

	return m;
}

inline void sys_heap_free(sys_heap_id_t  /*heap_id*/, void *p)
{
	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	free(p);
}

inline sys_heap_id_t sys_heap_create_s()
{
	auto *cs = new SysCS;
	cs->Init();
	return cs;
}

inline void* sys_heap_alloc_s(sys_heap_id_t heap_id, size_t size)
{
	heap_id->Enter();

	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void *m = malloc(size);

	heap_id->Leave();

	EXIT_IF(m == nullptr);

	return m;
}

inline void* sys_heap_realloc_s(sys_heap_id_t heap_id, void *p, size_t size)
{
	heap_id->Enter();

	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void *m = p != nullptr ? realloc(p, size) : malloc(size);

	heap_id->Leave();

	EXIT_IF(m == nullptr);

	return m;
}

inline void sys_heap_free_s(sys_heap_id_t heap_id, void *p)
{
	heap_id->Enter();

	//NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	free(p);

	heap_id->Leave();

}

} // namespace Kyty

#endif

#endif

#endif /* SYS_WIN32_INCLUDE_KYTY_SYSHEAP_H_ */
