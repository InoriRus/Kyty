#ifndef INCLUDE_KYTY_CORE_MSPACE_H_
#define INCLUDE_KYTY_CORE_MSPACE_H_

#include "Kyty/Core/Common.h"

namespace Kyty::Core {

using mspace_t              = void*;
using mspace_dbg_callback_t = void (*)(mspace_t, size_t, size_t);

struct MSpaceSize
{
	size_t max_system_size;
	size_t current_system_size;
	size_t max_inuse_size;
	size_t current_inuse_size;
};

mspace_t MSpaceCreate(const char* name, void* base, size_t capacity, bool thread_safe, mspace_dbg_callback_t dbg_callback);
bool     MSpaceDestroy(mspace_t msp);
void*    MSpaceMalloc(mspace_t msp, size_t size);
bool     MSpaceFree(mspace_t msp, void* ptr);
void*    MSpaceRealloc(mspace_t msp, void* ptr, size_t size);

void*  MSpaceCalloc(mspace_t msp, size_t nelem, size_t size);
void*  MSpaceAlignedAlloc(mspace_t msp, size_t alignment, size_t size);
void*  MSpaceMemalign(mspace_t msp, size_t boundary, size_t size);
void*  MSpaceReallocalign(mspace_t msp, void* ptr, size_t boundary, size_t size);
bool   MSpacePosixMemalign(mspace_t msp, void** ptr, size_t boundary, size_t size);
size_t MSpaceMallocUsableSize(void* ptr);
bool   MSpaceMallocStats(mspace_t msp, MSpaceSize* mmsize);
bool   MSpaceMallocStatsFast(mspace_t msp, MSpaceSize* mmsize);
bool   MSpaceIsHeapEmpty(mspace_t msp);

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_MSPACE_H_ */
