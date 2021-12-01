#ifndef SYS_LINUX_INCLUDE_KYTY_SYSDBG_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSDBG_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

namespace Kyty {

struct sys_dbg_stack_info_t
{
	uintptr_t code_addr;
	uintptr_t addr;
	uintptr_t commited_addr;
	size_t    commited_size;
	size_t    total_size;
	size_t    code_size;
};

using exception_filter_func_t = void (*)(void* addr);

void sys_stack_walk(void** stack, int* depth);
void sys_stack_usage(sys_dbg_stack_info_t& s);
void sys_stack_usage_print(sys_dbg_stack_info_t& stack);
void sys_get_code_info(uintptr_t* addr, size_t* size);
void sys_set_exception_filter(exception_filter_func_t func);

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSDBG_H_ */
