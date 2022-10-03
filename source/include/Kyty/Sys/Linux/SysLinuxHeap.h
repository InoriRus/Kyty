#ifndef SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Sys/Linux/SysLinuxSync.h"

namespace Kyty {

using sys_heap_id_t = SysCS*;

inline sys_heap_id_t sys_heap_create()
{
	return nullptr;
}

inline sys_heap_id_t sys_heap_deafult()
{
	return nullptr;
}

inline void* sys_heap_alloc(sys_heap_id_t /*heap_id*/, size_t size)
{
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void* m = malloc(size);

	EXIT_IF(m == nullptr);

	return m;
}

inline void* sys_heap_realloc(sys_heap_id_t /*heap_id*/, void* p, size_t size)
{
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void* m = p != nullptr ? realloc(p, size) : malloc(size);

	if (m == nullptr)
	{
		EXIT_IF(m == nullptr);
	}

	return m;
}

inline void sys_heap_free(sys_heap_id_t /*heap_id*/, void* p)
{
	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	free(p);
}

inline sys_heap_id_t sys_heap_create_s()
{
	auto* cs = new SysCS;
	cs->Init();
	return cs;
}

inline void* sys_heap_alloc_s(sys_heap_id_t heap_id, size_t size)
{
	heap_id->Enter();

	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void* m = malloc(size);

	heap_id->Leave();

	EXIT_IF(m == nullptr);

	return m;
}

inline void* sys_heap_realloc_s(sys_heap_id_t heap_id, void* p, size_t size)
{
	heap_id->Enter();

	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	void* m = p != nullptr ? realloc(p, size) : malloc(size);

	heap_id->Leave();

	EXIT_IF(m == nullptr);

	return m;
}

inline void sys_heap_free_s(sys_heap_id_t heap_id, void* p)
{
	heap_id->Enter();

	// NOLINTNEXTLINE(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	free(p);

	heap_id->Leave();
}

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSHEAP_H_ */
