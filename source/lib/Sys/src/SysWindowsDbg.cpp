#include "Kyty/Sys/SysWindowsDbg.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Sys/SysDbg.h"

// IWYU pragma: no_include <basetsd.h>
// IWYU pragma: no_include <memoryapi.h>
// IWYU pragma: no_include <minwindef.h>
// IWYU pragma: no_include <processthreadsapi.h>
// IWYU pragma: no_include <errhandlingapi.h>
// IWYU pragma: no_include <excpt.h>
// IWYU pragma: no_include <winbase.h>

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#include <windows.h> // IWYU pragma: keep
#if KYTY_COMPILER == KYTY_COMPILER_MSVC
#include <intrin.h>
#endif
#include <psapi.h> // IWYU pragma: keep

namespace Kyty {

#if KYTY_BITNESS == 32
static thread_local sys_dbg_stack_info_t g_stack = {0};
// uintptr_t g_stack_addr = 0;
// bool g_need_break = false;
#endif

#define KYTY_FRAME_SKIP 1

struct FrameS
{
	struct FrameS* next;
	void*          ret_addr;
};

constexpr DWORD READABLE =
    (static_cast<DWORD>(PAGE_EXECUTE_READ) | static_cast<DWORD>(PAGE_EXECUTE_READWRITE) | static_cast<DWORD>(PAGE_EXECUTE_WRITECOPY) |
     static_cast<DWORD>(PAGE_READONLY) | static_cast<DWORD>(PAGE_READWRITE) | static_cast<DWORD>(PAGE_WRITECOPY));
constexpr DWORD PROTECTED = (static_cast<DWORD>(PAGE_GUARD) | static_cast<DWORD>(PAGE_NOCACHE) | static_cast<DWORD>(PAGE_NOACCESS));

exception_filter_func_t g_exception_filter_func = nullptr;

bool sys_mem_read_allowed(void* ptr)
{
	MEMORY_BASIC_INFORMATION mbi;

	size_t s = VirtualQuery(ptr, &mbi, sizeof(mbi));

	if (s == 0)
	{
		EXIT_IF(s == 0);
	}

	return ((mbi.Protect & PROTECTED) == 0u) && ((mbi.State & static_cast<DWORD>(MEM_COMMIT)) != 0u) &&
	       ((mbi.AllocationProtect & READABLE) != 0u);
}

// LONG WINAPI
// VectoredHandlerSkip(struct _EXCEPTION_POINTERS *ExceptionInfo)
//{
//	PCONTEXT Context;
//
//	g_need_break = true;
//
//	Context = ExceptionInfo->ContextRecord;
//#ifdef _AMD64_
//	Context->Rip++;
//#else
//	Context->Eip++;
//#endif
//	return EXCEPTION_CONTINUE_EXECUTION;
//}

#if KYTY_BITNESS == 32
static void stackwalk(void* ebp, void** stack, int* depth, uintptr_t stack_addr, size_t stack_size)
{
	frame_t* frame = (frame_t*)ebp;

	int d = *depth;

	int i;

	// printf("1\n");
	for (i = 0; i < KYTY_FRAME_SKIP; i++)
	{
		//		if (uintptr_t(frame) <= 0xffff || (uintptr_t(frame) & 0xf0000000)
		//				|| frame->ret_addr == 0
		//				|| (uintptr_t(frame->ret_addr) & 0xf0000000)
		//				|| (uintptr_t(frame->next) & 0xf0000000)) break;
		//		if (!sys_mem_read_allowed(&frame->next)) break;
		if (!(uintptr_t(frame) >= stack_addr && uintptr_t(frame) < stack_addr + stack_size)) break;

		frame = frame->next;
	}
	// printf("2\n");
	for (i = 0; i < d; i++)
	{
		//#ifdef _MSC_VER
		//		__try
		//		{
		//#endif
		// FILE *f = fopen("_sw", "wt");
		// printf("%d, %08x\n", i, (uint32_t)frame);
		// fflush(stdout);
		// fclose(f);
		//		printf("%d, %08x, %08x, %08x\n", i, (uint32_t)frame,
		//				(uint32_t)frame->ret_addr, (uint32_t)frame->next);

		//		if (uintptr_t(frame) <= 0xffff || (uintptr_t(frame) & 0xf0000000)
		//				|| frame->ret_addr == 0
		//				|| (uintptr_t(frame->ret_addr) & 0xf0000000)
		//				|| (uintptr_t(frame->next) & 0xf0000000)) break;
		//		if (uintptr_t(frame) == 0 || frame->ret_addr == 0	) break;

		//		if (!sys_mem_read_allowed(&frame->next) || !sys_mem_read_allowed(&frame->ret_addr)) break;
		if (!(uintptr_t(frame) >= stack_addr && uintptr_t(frame) < stack_addr + stack_size)) break;

		// if (g_need_break) break;

		stack[i] = frame->ret_addr;

		frame = frame->next;
		//#ifdef _MSC_VER
		//		} __except(EXCEPTION_EXECUTE_HANDLER)
		//		{
		//			break;
		//		}
		//#endif
	}
	// printf("3\n");

	*depth = i;
}
#endif

#if KYTY_BITNESS == 32
void sys_stack_walk(void** stack, int* depth)
{
#if KYTY_COMPILER == KYTY_COMPILER_MSVC
	void* ebp = (size_t*)_AddressOfReturnAddress() - 1;
#else
	void* ebp = __builtin_frame_address(0);
#endif

	// g_need_break = false;

	//#ifndef _MSC_VER
	// PVOID p = AddVectoredExceptionHandler(1000, VectoredHandlerSkip);
	// if (!g_stack_addr) g_stack_addr = (uintptr_t)&depth;
	///#endif

	if (g_stack.total_size == 0)
	{
		sys_stack_usage(g_stack);
	}

	stackwalk(ebp, stack, depth, g_stack.addr, g_stack.total_size);

	//#ifndef _MSC_VER
	// RemoveVectoredExceptionHandler(p);
	//#endif
}
#else
//#include <unwind.h>
// struct unwind_info_t
//{
//	void **stack;
//	int depth;
//	int max_depth;
//};
//_Unwind_Reason_Code trace_fcn(_Unwind_Context *ctx, void *d)
//{
//	unwind_info_t *info = (unwind_info_t*)d;
//    //printf("\t#%d: program counter at %08x\n", *depth, _Unwind_GetIP(ctx));
//    //(*depth)++;
//	if (info->depth < info->max_depth)
//	{
//		void *ptr = (void*)_Unwind_GetIP(ctx);
//		info->stack[info->depth] = ptr;
//		info->depth++;
//	}
//    return _URC_NO_REASON;
//}
int WalkStack(int z_stack_depth, void** z_stack_trace)
{
	CONTEXT context;
	//	KNONVOLATILE_CONTEXT_POINTERS NvContext;
	PRUNTIME_FUNCTION runtime_function  = nullptr;
	PVOID             handler_data      = nullptr;
	ULONG64           establisher_frame = 0;
	ULONG64           image_base        = 0;

	RtlCaptureContext(&context);

	int frame = 0;

	while (true)
	{
		if (frame >= z_stack_depth)
		{
			break;
		}

		z_stack_trace[frame] = reinterpret_cast<void*>(context.Rip);

		frame++;

		runtime_function = RtlLookupFunctionEntry(context.Rip, &image_base, nullptr);

		if (runtime_function == nullptr)
		{
			break;
		}

		// RtlZeroMemory(&NvContext, sizeof(KNONVOLATILE_CONTEXT_POINTERS));
		RtlVirtualUnwind(0, image_base, context.Rip, runtime_function, &context, &handler_data, &establisher_frame, nullptr /*&NvContext*/);

		if (context.Rip == 0u)
		{
			break;
		}
	}

	return frame;
}

void sys_stack_walk(void** stack, int* depth)
{
	//	USHORT n = CaptureStackBackTrace(KYTY_FRAME_SKIP, *depth, stack, 0);
	int n  = WalkStack(*depth, stack);
	*depth = n;
	//	unwind_info_t info = {stack, 0, *depth};
	//	 _Unwind_Backtrace(&trace_fcn, &info);
	//	 *depth = info.depth;
}
#endif

void sys_stack_usage_print(sys_dbg_stack_info_t& stack)
{
	printf("stack: (0x%" PRIx64 ", %" PRIu64 ") + (0x%" PRIx64 ", %" PRIu64 ") + (0x%" PRIx64 ", %" PRIu64 ")\n",
	       static_cast<uint64_t>(stack.reserved_addr), static_cast<uint64_t>(stack.reserved_size), static_cast<uint64_t>(stack.guard_addr),
	       static_cast<uint64_t>(stack.guard_size), static_cast<uint64_t>(stack.commited_addr), static_cast<uint64_t>(stack.commited_size));
}

void sys_stack_usage(sys_dbg_stack_info_t& s)
{
	MEMORY_BASIC_INFORMATION mbi {};
	[[maybe_unused]] size_t  ss = VirtualQuery(&mbi, &mbi, sizeof(mbi));
	EXIT_IF(ss == 0);
	PVOID reserved = mbi.AllocationBase;
	ss             = VirtualQuery(reserved, &mbi, sizeof(mbi));
	EXIT_IF(ss == 0);
	size_t reserved_size = mbi.RegionSize;
	ss                   = VirtualQuery(static_cast<char*>(reserved) + reserved_size, &mbi, sizeof(mbi));
	EXIT_IF(ss == 0);
	void*  guard_page      = mbi.BaseAddress;
	size_t guard_page_size = mbi.RegionSize;
	ss                     = VirtualQuery(static_cast<char*>(guard_page) + guard_page_size, &mbi, sizeof(mbi));
	EXIT_IF(ss == 0);
	void*  commited      = mbi.BaseAddress;
	size_t commited_size = mbi.RegionSize;
	s.reserved_addr      = reinterpret_cast<uintptr_t>(reserved);
	s.reserved_size      = reserved_size;
	s.guard_addr         = reinterpret_cast<uintptr_t>(guard_page);
	s.guard_size         = guard_page_size;
	s.commited_addr      = reinterpret_cast<uintptr_t>(commited);
	s.commited_size      = commited_size;

	s.addr       = s.reserved_addr;
	s.total_size = s.reserved_size + s.guard_size + s.commited_size;
}

void sys_get_code_info(uintptr_t* addr, size_t* size)
{
	MODULEINFO info {};
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &info, sizeof(MODULEINFO));
	*addr = reinterpret_cast<uintptr_t>(info.lpBaseOfDll);
	*size = static_cast<size_t>(info.SizeOfImage);
}

static LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exception)
{
	g_exception_filter_func(exception->ExceptionRecord->ExceptionAddress);
	return EXCEPTION_EXECUTE_HANDLER;
}

void sys_set_exception_filter(exception_filter_func_t func)
{
	g_exception_filter_func = func;
	SetUnhandledExceptionFilter(ExceptionFilter);
}

} // namespace Kyty

#endif
