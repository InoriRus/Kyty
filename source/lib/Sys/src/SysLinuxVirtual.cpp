#include "Kyty/Core/Common.h"

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/VirtualMemory.h"
#include "Kyty/Sys/SysVirtual.h"

#include "cpuinfo.h"

#include <cerrno>
#include <map>
#include <pthread.h>
#include <sys/mman.h>

// IWYU pragma: no_include <asm/mman-common.h>
// IWYU pragma: no_include <asm/mman.h>
// IWYU pragma: no_include <bits/pthread_types.h>
// IWYU pragma: no_include <linux/mman.h>

#if defined(MAP_FIXED_NOREPLACE) && KYTY_PLATFORM == KYTY_PLATFORM_LINUX
#define KYTY_FIXED_NOREPLACE
#endif

namespace Kyty::Core {

static pthread_mutex_t              g_virtual_mutex {};
static std::map<uintptr_t, size_t>* g_allocs   = nullptr;
static std::map<uintptr_t, int>*    g_protects = nullptr;

void sys_get_system_info(SystemInfo* info)
{
	EXIT_IF(info == nullptr);

	const auto* p = cpuinfo_get_package(0);

	EXIT_IF(p == nullptr);

	info->ProcessorName = String::FromUtf8(p->name);
}

void sys_virtual_init()
{
	pthread_mutexattr_t attr {};

	pthread_mutexattr_init(&attr);
#if KYTY_PLATFORM == KYTY_PLATFORM_LINUX
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_FAST_NP);
#else
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
	pthread_mutex_init(&g_virtual_mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	g_allocs   = new std::map<uintptr_t, size_t>;
	g_protects = new std::map<uintptr_t, int>;

	cpuinfo_initialize();
}

static int get_protection_flag(VirtualMemory::Mode mode)
{
	int protect = PROT_NONE;
	switch (mode)
	{
		case VirtualMemory::Mode::Read: protect = PROT_READ; break;
		case VirtualMemory::Mode::Write:
		case VirtualMemory::Mode::ReadWrite: protect = PROT_READ | PROT_WRITE; break; // NOLINT
		case VirtualMemory::Mode::Execute: protect = PROT_EXEC; break;
		case VirtualMemory::Mode::ExecuteRead: protect = PROT_EXEC | PROT_READ; break; // NOLINT
		case VirtualMemory::Mode::ExecuteWrite:
		case VirtualMemory::Mode::ExecuteReadWrite: protect = PROT_EXEC | PROT_WRITE | PROT_READ; break; // NOLINT
		case VirtualMemory::Mode::NoAccess:
		default: protect = PROT_NONE; break;
	}
	return protect;
}

static VirtualMemory::Mode get_protection_flag(int mode)
{
	switch (mode)
	{
		case PROT_NONE: return VirtualMemory::Mode::NoAccess;
		case PROT_READ: return VirtualMemory::Mode::Read;
		case PROT_WRITE: return VirtualMemory::Mode::Write;
		case PROT_READ | PROT_WRITE: return VirtualMemory::Mode::ReadWrite; // NOLINT
		case PROT_EXEC: return VirtualMemory::Mode::Execute;
		case PROT_EXEC | PROT_WRITE: return VirtualMemory::Mode::ExecuteWrite;                 // NOLINT
		case PROT_EXEC | PROT_READ: return VirtualMemory::Mode::ExecuteRead;                   // NOLINT
		case PROT_EXEC | PROT_WRITE | PROT_READ: return VirtualMemory::Mode::ExecuteReadWrite; // NOLINT
		default: return VirtualMemory::Mode::NoAccess;
	}
}

uint64_t sys_virtual_alloc(uint64_t address, uint64_t size, VirtualMemory::Mode mode)
{
	EXIT_IF(g_allocs == nullptr);

	auto addr = static_cast<uintptr_t>(address);

	int protect = get_protection_flag(mode);

	void* ptr = mmap(reinterpret_cast<void*>(addr), size, protect, MAP_PRIVATE | MAP_ANON, -1, 0); // NOLINT

	auto ret_addr = reinterpret_cast<uintptr_t>(ptr);

	if (ptr != MAP_FAILED)
	{
		pthread_mutex_lock(&g_virtual_mutex);
		(*g_allocs)[ret_addr] = size;
		uintptr_t page_start  = ret_addr >> 12u;
		uintptr_t page_end    = (ret_addr + size - 1) >> 12u;
		for (uintptr_t page = page_start; page <= page_end; page++)
		{
			(*g_protects)[page] = protect;
		}
		pthread_mutex_unlock(&g_virtual_mutex);
	}

	return ret_addr;
}

static uintptr_t align_up(uintptr_t addr, uint64_t alignment)
{
	return (addr + alignment - 1) & ~(alignment - 1);
}

uint64_t sys_virtual_alloc_aligned(uint64_t address, uint64_t size, VirtualMemory::Mode mode, uint64_t alignment)
{
	if (alignment == 0)
	{
		return 0;
	}

	EXIT_IF(g_allocs == nullptr);

	auto addr    = static_cast<uintptr_t>(address);
	int  protect = get_protection_flag(mode);

	void* ptr = mmap(reinterpret_cast<void*>(addr), size, protect, MAP_PRIVATE | MAP_ANON, -1, 0); // NOLINT

	auto ret_addr = reinterpret_cast<uintptr_t>(ptr);

	if (ptr != MAP_FAILED && ((ret_addr & (alignment - 1)) != 0))
	{
		munmap(ptr, size);

		ptr      = mmap(reinterpret_cast<void*>(addr), size + alignment, protect, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0); // NOLINT
		ret_addr = reinterpret_cast<uintptr_t>(ptr);
		if (ptr != MAP_FAILED)
		{
			munmap(ptr, size + alignment);
			auto aligned_addr = align_up(ret_addr, alignment);
#ifdef KYTY_FIXED_NOREPLACE
			// NOLINTNEXTLINE
			ptr      = mmap(reinterpret_cast<void*>(aligned_addr), size, protect, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANON, -1, 0);
#else
			// NOLINTNEXTLINE
			ptr = mmap(reinterpret_cast<void*>(aligned_addr), size, protect, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
			ret_addr = reinterpret_cast<uintptr_t>(ptr);
			if (ptr == MAP_FAILED)
			{
				[[maybe_unused]] int err = errno;
				// printf("mmap failed: %d\n", err);
			}
			if (ptr != MAP_FAILED && ((ret_addr & (alignment - 1)) != 0))
			{
				munmap(ptr, size);
				ret_addr = 0;
				ptr      = MAP_FAILED;
			}
		}
	}

	if (ptr == MAP_FAILED)
	{
		return sys_virtual_alloc_aligned(address, size, mode, alignment << 1u);
	}

	pthread_mutex_lock(&g_virtual_mutex);
	(*g_allocs)[ret_addr] = size;
	uintptr_t page_start  = ret_addr >> 12u;
	uintptr_t page_end    = (ret_addr + size - 1) >> 12u;
	for (uintptr_t page = page_start; page <= page_end; page++)
	{
		(*g_protects)[page] = protect;
	}
	pthread_mutex_unlock(&g_virtual_mutex);

	return ret_addr;
}

[[maybe_unused]] static bool is_mmaped(void* ptr, size_t length)
{
	FILE* file = fopen("/proc/self/maps", "r");
	char  line[1024];
	bool  ret  = false;
	auto  addr = reinterpret_cast<uintptr_t>(ptr);

	[[maybe_unused]] int result = 0;

	while (feof(file) == 0)
	{
		if (fgets(line, 1024, file) == nullptr)
		{
			break;
		}
		uint64_t start = 0;
		uint64_t end   = 0;
		// NOLINTNEXTLINE(cert-err34-c)
		if (sscanf(line, "%" SCNx64 "-%" SCNx64, &start, &end) != 2)
		{
			continue;
		}
		if (addr >= start && addr + length <= end)
		{
			ret = true;
			break;
		}
	}
	result = fclose(file);
	return ret;
}

bool sys_virtual_alloc_fixed(uint64_t address, uint64_t size, VirtualMemory::Mode mode)
{
	EXIT_IF(g_allocs == nullptr);

	auto addr    = static_cast<uintptr_t>(address);
	int  protect = get_protection_flag(mode);

#ifdef KYTY_FIXED_NOREPLACE
	// NOLINTNEXTLINE
	void* ptr = mmap(reinterpret_cast<void*>(addr), size, protect, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANON, -1, 0);
#else
	// NOLINTNEXTLINE
	void* ptr = (is_mmaped(reinterpret_cast<void*>(addr), size)
	                 ? MAP_FAILED
	                 : mmap(reinterpret_cast<void*>(addr), size, protect, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0));
#endif

	auto ret_addr = reinterpret_cast<uintptr_t>(ptr);

	if (ptr != MAP_FAILED && ret_addr != addr)
	{
		munmap(ptr, size);
		ret_addr = 0;
		ptr      = MAP_FAILED;
	}

	if (ptr != MAP_FAILED)
	{
		pthread_mutex_lock(&g_virtual_mutex);
		(*g_allocs)[ret_addr] = size;
		uintptr_t page_start  = ret_addr >> 12u;
		uintptr_t page_end    = (ret_addr + size - 1) >> 12u;
		for (uintptr_t page = page_start; page <= page_end; page++)
		{
			(*g_protects)[page] = protect;
		}
		pthread_mutex_unlock(&g_virtual_mutex);

		return true;
	}

	return false;
}

bool sys_virtual_free(uint64_t address)
{
	EXIT_IF(g_allocs == nullptr);
	size_t size = 0;

	auto addr = static_cast<uintptr_t>(address & ~static_cast<uint64_t>(0xfffu));

	pthread_mutex_lock(&g_virtual_mutex);
	if (auto s = g_allocs->find(addr); s != g_allocs->end())
	{
		size = s->second;
		g_allocs->erase(s);
	}
	pthread_mutex_unlock(&g_virtual_mutex);

	if (size == 0)
	{
		return false;
	}

	if (munmap(reinterpret_cast<void*>(addr), size) == 0)
	{
		uintptr_t page_start = addr >> 12u;
		uintptr_t page_end   = (addr + size - 1) >> 12u;
		pthread_mutex_lock(&g_virtual_mutex);
		for (uintptr_t page = page_start; page <= page_end; page++)
		{
			if (auto s = g_protects->find(page); s != g_protects->end())
			{
				g_protects->erase(s);
			}
		}
		pthread_mutex_unlock(&g_virtual_mutex);
		return true;
	}

	return false;
}

bool sys_virtual_protect(uint64_t address, uint64_t size, VirtualMemory::Mode mode, VirtualMemory::Mode* old_mode)
{
	auto addr = static_cast<uintptr_t>(address);

	pthread_mutex_lock(&g_virtual_mutex);
	if (old_mode != nullptr)
	{
		if (auto s = g_protects->find(addr >> 12u); s != g_protects->end())
		{
			*old_mode = get_protection_flag(s->second);
		} else
		{
			*old_mode = VirtualMemory::Mode::NoAccess;
		}
	}
	pthread_mutex_unlock(&g_virtual_mutex);

	uintptr_t page_start = addr >> 12u;
	uintptr_t page_end   = (addr + size - 1) >> 12u;
	if (mprotect(reinterpret_cast<void*>(page_start << 12u), (page_end - page_start + 1) << 12u, get_protection_flag(mode)) == 0)
	{
		pthread_mutex_lock(&g_virtual_mutex);
		for (uintptr_t page = page_start; page <= page_end; page++)
		{
			(*g_protects)[page] = get_protection_flag(mode);
		}
		pthread_mutex_unlock(&g_virtual_mutex);
		return true;
	}

	return false;
}

bool sys_virtual_flush_instruction_cache(uint64_t /*address*/, uint64_t /*size*/)
{
	return true;
}

bool sys_virtual_patch_replace(uint64_t vaddr, uint64_t value)
{
	VirtualMemory::Mode old_mode {};
	sys_virtual_protect(vaddr, 8, VirtualMemory::Mode::ReadWrite, &old_mode);

	auto* ptr = reinterpret_cast<uint64_t*>(vaddr);

	bool ret = (*ptr != value);

	*ptr = value;

	sys_virtual_protect(vaddr, 8, old_mode);

	if (VirtualMemory::IsExecute(old_mode))
	{
		sys_virtual_flush_instruction_cache(vaddr, 8);
	}

	return ret;
}

} // namespace Kyty::Core

#endif
