#include "Kyty/Core/VirtualMemory.h"

#include "Kyty/Sys/SysVirtual.h"

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#define KYTY_HAS_EXCEPTIONS
#endif

#ifdef KYTY_HAS_EXCEPTIONS
#include <windows.h> // IWYU pragma: keep
#endif

// IWYU pragma: no_include <basetsd.h>
// IWYU pragma: no_include <errhandlingapi.h>
// IWYU pragma: no_include <excpt.h>
// IWYU pragma: no_include <minwinbase.h>
// IWYU pragma: no_include <minwindef.h>
// IWYU pragma: no_include <wtypes.h>

namespace Kyty::Core {

SystemInfo GetSystemInfo()
{
	SystemInfo ret {};

	sys_get_system_info(&ret);

	return ret;
}

namespace VirtualMemory {

#ifdef KYTY_HAS_EXCEPTIONS

struct JmpRax
{
	template <class Handler>
	void SetFunc(Handler func)
	{
		*reinterpret_cast<Handler*>(&code[2]) = func;
	}

	// mov rax, 0x1122334455667788
	// jmp rax
	uint8_t code[16] = {0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xFF, 0xE0};
};

class ExceptionHandlerPrivate
{
public:
#pragma pack(1)

	struct UnwindInfo
	{
		uint8_t Version : 3;
		uint8_t Flags   : 5;
		uint8_t SizeOfProlog;
		uint8_t CountOfCodes;
		uint8_t FrameRegister : 4;
		uint8_t FrameOffset   : 4;
		ULONG   ExceptionHandler;

		ExceptionHandlerPrivate* ExceptionData;
	};

	struct HandlerInfo
	{
		JmpRax           code;
		RUNTIME_FUNCTION function_table = {};
		UnwindInfo       unwind_info    = {};
	};

#pragma pack()

	static EXCEPTION_DISPOSITION Handler(PEXCEPTION_RECORD   exception_record, ULONG64 /*EstablisherFrame*/, PCONTEXT /*ContextRecord*/,
	                                     PDISPATCHER_CONTEXT dispatcher_context)
	{
		ExceptionHandler::ExceptionInfo info {};

		info.exception_address = reinterpret_cast<uint64_t>(exception_record->ExceptionAddress);

		if (exception_record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
		{
			info.type = ExceptionHandler::ExceptionType::AccessViolation;
			switch (exception_record->ExceptionInformation[0])
			{
				case 0: info.access_violation_type = ExceptionHandler::AccessViolationType::Read; break;
				case 1: info.access_violation_type = ExceptionHandler::AccessViolationType::Write; break;
				case 8: info.access_violation_type = ExceptionHandler::AccessViolationType::Execute; break;
				default: info.access_violation_type = ExceptionHandler::AccessViolationType::Unknown; break;
			}
			info.access_violation_vaddr = exception_record->ExceptionInformation[1];
		}

		info.rbp                = dispatcher_context->ContextRecord->Rbp;
		info.exception_win_code = exception_record->ExceptionCode;

		auto* p = *static_cast<ExceptionHandlerPrivate**>(dispatcher_context->HandlerData);
		p->func(&info);

		return ExceptionContinueExecution;
	}

	void InitHandler()
	{
		auto* h           = new (reinterpret_cast<void*>(handler_addr)) HandlerInfo;
		auto* code        = &h->code;
		auto* unwind_info = &h->unwind_info;

		function_table = &h->function_table;

		function_table->BeginAddress = 0;
		function_table->EndAddress   = image_size;
		function_table->UnwindData   = reinterpret_cast<uintptr_t>(unwind_info) - base_address;

		unwind_info->Version          = 1;
		unwind_info->Flags            = UNW_FLAG_EHANDLER;
		unwind_info->SizeOfProlog     = 0;
		unwind_info->CountOfCodes     = 0;
		unwind_info->FrameRegister    = 0;
		unwind_info->FrameOffset      = 0;
		unwind_info->ExceptionHandler = reinterpret_cast<uintptr_t>(code) - base_address;
		unwind_info->ExceptionData    = this;

		code->SetFunc(Handler);

		FlushInstructionCache(reinterpret_cast<uint64_t>(code), sizeof(h->code));
	}

	uint64_t          base_address   = 0;
	uint64_t          handler_addr   = 0;
	uint64_t          image_size     = 0;
	PRUNTIME_FUNCTION function_table = nullptr;

	ExceptionHandler::handler_func_t func = nullptr;

	static ExceptionHandler::handler_func_t g_vec_func;
};

ExceptionHandler::handler_func_t ExceptionHandlerPrivate::g_vec_func = nullptr;

#else
class ExceptionHandlerPrivate
{
};
#endif

ExceptionHandler::ExceptionHandler(): m_p(new ExceptionHandlerPrivate) {}

ExceptionHandler::~ExceptionHandler()
{
#ifdef KYTY_HAS_EXCEPTIONS
	Uninstall();
#endif
	delete m_p;
}

uint64_t ExceptionHandler::GetSize()
{
#ifdef KYTY_HAS_EXCEPTIONS
	return (sizeof(ExceptionHandlerPrivate::HandlerInfo) & ~(static_cast<uint64_t>(0x1000) - 1)) + 0x1000;
#else
	return 0x1000;
#endif
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static, misc-unused-parameters)
bool ExceptionHandler::Install(uint64_t base_address, uint64_t handler_addr, uint64_t image_size, handler_func_t func)
{
#ifdef KYTY_HAS_EXCEPTIONS
	if (m_p->function_table == nullptr)
	{
		m_p->base_address = base_address;
		m_p->handler_addr = handler_addr;
		m_p->image_size   = image_size;
		m_p->func         = func;

		m_p->InitHandler();

		if (RtlAddFunctionTable(m_p->function_table, 1, base_address) == FALSE)
		{
			printf("RtlAddFunctionTable() failed: 0x%08" PRIx32 "\n", static_cast<uint32_t>(GetLastError()));
			return false;
		}

		return true;
	}

	return false;
#else
	return true;
#endif
}

#ifdef KYTY_HAS_EXCEPTIONS
static LONG WINAPI ExceptionFilter(PEXCEPTION_POINTERS exception)
{
	PEXCEPTION_RECORD exception_record = exception->ExceptionRecord;

	ExceptionHandler::ExceptionInfo info {};

	info.exception_address = reinterpret_cast<uint64_t>(exception_record->ExceptionAddress);

	// printf("exception_record->ExceptionCode = %u\n", static_cast<uint32_t>(exception_record->ExceptionCode));

	if (exception_record->ExceptionCode == DBG_PRINTEXCEPTION_C || exception_record->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C)
	{
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if (exception_record->ExceptionCode == 0x406D1388)
	{
		// Set a thread name
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if (exception_record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		info.type = ExceptionHandler::ExceptionType::AccessViolation;
		switch (exception_record->ExceptionInformation[0])
		{
			case 0: info.access_violation_type = ExceptionHandler::AccessViolationType::Read; break;
			case 1: info.access_violation_type = ExceptionHandler::AccessViolationType::Write; break;
			case 8: info.access_violation_type = ExceptionHandler::AccessViolationType::Execute; break;
			default: info.access_violation_type = ExceptionHandler::AccessViolationType::Unknown; break;
		}
		info.access_violation_vaddr = exception_record->ExceptionInformation[1];
	}

	info.rbp                = exception->ContextRecord->Rbp;
	info.exception_win_code = exception_record->ExceptionCode;

	ExceptionHandlerPrivate::g_vec_func(&info);

	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

// NOLINTNEXTLINE(readability-convert-member-functions-to-static, misc-unused-parameters)
bool ExceptionHandler::InstallVectored(handler_func_t func)
{
#ifdef KYTY_HAS_EXCEPTIONS
	if (ExceptionHandlerPrivate::g_vec_func == nullptr)
	{
		ExceptionHandlerPrivate::g_vec_func = func;

		if (AddVectoredExceptionHandler(1, ExceptionFilter) == nullptr)
		{
			printf("AddVectoredExceptionHandler() failed\n");
			return false;
		}

		return true;
	}
	return false;
#else
	return true;
#endif
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static, misc-unused-parameters)
bool ExceptionHandler::Uninstall()
{
#ifdef KYTY_HAS_EXCEPTIONS
	if (m_p->function_table != nullptr)
	{
		if (RtlDeleteFunctionTable(m_p->function_table) == FALSE)
		{
			printf("RtlDeleteFunctionTable() failed: 0x%08" PRIx32 "\n", static_cast<uint32_t>(GetLastError()));
			return false;
		}
		m_p->function_table = nullptr;
		return true;
	}

	return false;
#else
	return true;
#endif
}

void Init()
{
	sys_virtual_init();
}

uint64_t Alloc(uint64_t address, uint64_t size, Mode mode)
{
	return sys_virtual_alloc(address, size, mode);
}

uint64_t AllocAligned(uint64_t address, uint64_t size, Mode mode, uint64_t alignment)
{
	return sys_virtual_alloc_aligned(address, size, mode, alignment);
}

bool AllocFixed(uint64_t address, uint64_t size, Mode mode)
{
	return sys_virtual_alloc_fixed(address, size, mode);
}

bool Free(uint64_t address)
{
	return sys_virtual_free(address);
}

bool Protect(uint64_t address, uint64_t size, Mode mode, Mode* old_mode)
{
	return sys_virtual_protect(address, size, mode, old_mode);
}

bool FlushInstructionCache(uint64_t address, uint64_t size)
{
	return sys_virtual_flush_instruction_cache(address, size);
}

bool PatchReplace(uint64_t vaddr, uint64_t value)
{
	return sys_virtual_patch_replace(vaddr, value);
}

} // namespace VirtualMemory

} // namespace Kyty::Core
