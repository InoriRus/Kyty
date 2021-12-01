#ifndef INCLUDE_KYTY_CORE_MEMORYALLOC_H_
#define INCLUDE_KYTY_CORE_MEMORYALLOC_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"

namespace Kyty::Core {

// SUBSYSTEM_DEFINE(Memory);
void core_memory_init();

struct MemStats
{
	int      state;
	size_t   total_allocated;
	uint32_t blocks_num;
};

void* mem_alloc(size_t size);
void* mem_realloc(void* ptr, size_t size);
void  mem_free(void* ptr);
void  mem_print(int from_state);
void  mem_get_stat(MemStats* s);
void  mem_set_max_size(size_t size);
int   mem_new_state();
bool  mem_tracker_enabled();
void  mem_tracker_enable();
void  mem_tracker_disable();
bool  mem_check(const void* ptr);

#define KYTY_MEM_CHECK(ptr) EXIT_IF(!mem_check(ptr))

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_MEMORYALLOC_H_ */
