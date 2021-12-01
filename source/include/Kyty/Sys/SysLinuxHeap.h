#ifndef SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Sys/SysLinuxSync.h"

namespace Kyty {

typedef SysCS* sys_heap_id_t;

inline sys_heap_id_t sys_heap_create()
{
	return 0;
}

inline sys_heap_id_t sys_heap_deafult()
{
	return 0;
}

inline void* sys_heap_alloc(sys_heap_id_t heap_id, size_t size)
{
	void* m = malloc(size);

	EXIT_IF(m == 0);

	return m;
}

inline void* sys_heap_realloc(sys_heap_id_t heap_id, void* p, size_t size)
{
	void* m = p ? realloc(p, size) : malloc(size);

	if (m == 0)
	{
		EXIT_IF(m == 0);
	}

	return m;
}

inline void sys_heap_free(sys_heap_id_t heap_id, void* p)
{
	free(p);
}

inline sys_heap_id_t sys_heap_create_s()
{
	SysCS* cs = new SysCS;
	cs->Init();
	return cs;
}

inline void* sys_heap_alloc_s(sys_heap_id_t heap_id, size_t size)
{
	heap_id->Enter();

	void* m = malloc(size);

	heap_id->Leave();

	EXIT_IF(m == 0);

	return m;
}

inline void* sys_heap_realloc_s(sys_heap_id_t heap_id, void* p, size_t size)
{
	heap_id->Enter();

	void* m = p ? realloc(p, size) : malloc(size);

	heap_id->Leave();

	EXIT_IF(m == 0);

	return m;
}

inline void sys_heap_free_s(sys_heap_id_t heap_id, void* p)
{
	heap_id->Enter();

	free(p);

	heap_id->Leave();
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_ */
