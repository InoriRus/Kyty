#include "Kyty/Core/Common.h"

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Sys/SysDbg.h"

#include <cstdlib>
#include <cstring>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

namespace Kyty {

static thread_local sys_dbg_stack_info_t g_stack = {0};

void sys_stack_walk(void** /*stack*/, int* depth)
{
	*depth = 0;
}

void sys_stack_usage_print(sys_dbg_stack_info_t& stack)
{
	printf("stack: (0x%" PRIx64 ", %" PRIu64 ")\n", static_cast<uint64_t>(stack.commited_addr), static_cast<uint64_t>(stack.commited_size));
	printf("code: (0x%" PRIx64 ", %" PRIu64 ")\n", static_cast<uint64_t>(stack.code_addr), static_cast<uint64_t>(stack.code_size));
}

void sys_stack_usage(sys_dbg_stack_info_t& s)
{
	pid_t pid = getpid();

	// printf("pid = %"I64"d\n", (int64_t)pid);

	[[maybe_unused]] int result = 0;

	char str[1024];
	char str2[1024];
	result = sprintf(str, "/proc/%d/exe", static_cast<int>(pid));

	ssize_t buff_len = 0;
	if ((buff_len = readlink(str, str2, 1023)) == -1)
	{
		return;
	}
	str2[buff_len]   = '\0';
	const char* name = basename(str2);

	result = sprintf(str, "/proc/%d/maps", static_cast<int>(pid));

	memset(&s, 0, sizeof(sys_dbg_stack_info_t));

	FILE* f = fopen(str, "r");

	if (f == nullptr)
	{
		return;
	}

	// printf("&str = %"I64"x\n", (uint64_t)&str);

	uint64_t                  addr                 = 0;
	uint64_t                  endaddr              = 0;
	[[maybe_unused]] uint64_t size                 = 0;
	uint64_t                  offset               = 0;
	uint64_t                  inode                = 0;
	char                      permissions[8]       = {};
	char                      device[8]            = {};
	char                      filename[MAXPATHLEN] = {};

	auto check_addr = reinterpret_cast<uintptr_t>(&f);

	while (true)
	{
		if (feof(f) != 0)
		{
			break;
		}

		if (fgets(str, sizeof(str), f) == nullptr)
		{
			break;
		}

		filename[0]    = 0;
		permissions[0] = 0;
		addr           = 0;
		size           = 0;

		// printf("%s", str);

		// NOLINTNEXTLINE(cert-err34-c)
		result = sscanf(str, "%" SCNx64 "-%" SCNx64 " %s %" SCNx64 " %s %" SCNx64 " %s", &addr, &endaddr, permissions, &offset, device,
		                &inode, filename);

		size = endaddr - addr;

		bool read  = (strchr(permissions, 'r') != nullptr);
		bool write = (strchr(permissions, 'w') != nullptr);
		bool exec  = (strchr(permissions, 'x') != nullptr);

		// printf("%016"I64"x, %"I64"d, %s, %d, %d\n", addr, size, filename, read, write);

		if (read && write && !exec && strncmp(filename, "[stack", 6) == 0)
		{
			// printf("stack: %016"I64"x, %"I64"d\n", addr, size);

			if (check_addr >= addr && check_addr < addr + size)
			{
				s.addr          = addr;
				s.total_size    = size;
				s.commited_addr = addr;
				s.commited_size = size;

				if (s.code_addr != 0)
				{
					break;
				}
			}
		}

		if (read && !write && exec && strstr(filename, name) != nullptr)
		{
			s.code_addr = addr;
			s.code_size = size;

			if (s.addr != 0)
			{
				break;
			}
		}
	}

	result = fclose(f);
}

void sys_get_code_info(uintptr_t* addr, size_t* size)
{
	if (g_stack.code_size == 0)
	{
		sys_stack_usage(g_stack);
	}

	*addr = g_stack.code_addr;
	*size = g_stack.code_size;
}

void sys_set_exception_filter(exception_filter_func_t /*func*/) {}

} // namespace Kyty
#endif
