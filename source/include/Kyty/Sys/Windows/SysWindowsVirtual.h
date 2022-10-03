#ifndef INCLUDE_KYTY_SYS_WINDOWS_SYSWINDOWSVIRTUAL_H_
#define INCLUDE_KYTY_SYS_WINDOWS_SYSWINDOWSVIRTUAL_H_

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/VirtualMemory.h"

namespace Kyty::Core {

void sys_get_system_info(SystemInfo* info);

void sys_virtual_init();

uint64_t sys_virtual_alloc(uint64_t address, uint64_t size, VirtualMemory::Mode mode);
uint64_t sys_virtual_alloc_aligned(uint64_t address, uint64_t size, VirtualMemory::Mode mode, uint64_t alignment);
bool     sys_virtual_alloc_fixed(uint64_t address, uint64_t size, VirtualMemory::Mode mode);
bool     sys_virtual_free(uint64_t address);
bool     sys_virtual_protect(uint64_t address, uint64_t size, VirtualMemory::Mode mode, VirtualMemory::Mode* old_mode = nullptr);
bool     sys_virtual_flush_instruction_cache(uint64_t address, uint64_t size);
bool     sys_virtual_patch_replace(uint64_t vaddr, uint64_t value);

} // namespace Kyty::Core

#endif

#endif /* INCLUDE_KYTY_SYS_WINDOWS_SYSWINDOWSVIRTUAL_H_ */
