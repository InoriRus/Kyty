#include "Emulator/Loader/RuntimeLinker.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/MagicEnum.h"
#include "Kyty/Core/Singleton.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Sys/SysDbg.h"

#include "Emulator/Config.h"
#include "Emulator/Graphics/Objects/GpuMemory.h"
#include "Emulator/Kernel/Pthread.h"
#include "Emulator/Loader/Elf.h"
#include "Emulator/Loader/Jit.h"
#include "Emulator/Loader/SymbolDatabase.h"
#include "Emulator/Loader/VirtualMemory.h"
#include "Emulator/Profiler.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel {
void SetProgName(const String& name);
} // namespace Kyty::Libs::LibKernel

namespace Kyty::Loader {

#pragma pack(1)

struct EntryParams
{
	int         argc;
	uint32_t    pad;
	const char* argv[3];
};

#pragma pack()

using atexit_func_t          = KYTY_SYSV_ABI void (*)();
using entry_func_t           = KYTY_SYSV_ABI void (*)(EntryParams* params, atexit_func_t atexit_func);
using module_ini_fini_func_t = KYTY_SYSV_ABI int (*)(size_t args, const void* argp, module_func_t func);

enum class BindType
{
	Unknown,
	Local,
	Global,
	Weak
};

struct RelocationInfo
{
	bool       resolved   = false;
	BindType   bind       = BindType::Unknown;
	SymbolType type       = SymbolType::Unknown;
	uint64_t   value      = 0;
	uint64_t   vaddr      = 0;
	uint64_t   base_vaddr = 0;
	String     name;
	String     dbg_name;
	bool       bind_self = false;
};

// The structure will be passed via the stack
// since the size of an object is larger than 16 bytes
struct RelocateHandlerStack
{
	uint64_t stack[3];
};

constexpr uint64_t SYSTEM_RESERVED  = 0x800000000u;
constexpr uint64_t CODE_BASE_INCR   = 0x010000000u;
constexpr uint64_t INVALID_OFFSET   = 0x040000000u;
constexpr uint64_t CODE_BASE_OFFSET = 0x100000000u;
constexpr uint64_t INVALID_MEMORY   = SYSTEM_RESERVED + INVALID_OFFSET;

constexpr size_t   XSAVE_BUFFER_SIZE = 2688;
constexpr uint64_t XSAVE_CHK_GUARD   = 0xDeadBeef5533CCAAu;

static uint64_t g_desired_base_addr = SYSTEM_RESERVED + CODE_BASE_OFFSET;
static uint64_t g_invalid_memory    = 0;

static Program* g_tls_main_program = nullptr;
alignas(64) static uint8_t g_tls_reg_save_area[XSAVE_BUFFER_SIZE + sizeof(XSAVE_CHK_GUARD)];
static uint8_t g_tls_spinlock = 0;

static KYTY_SYSV_ABI void run_entry(uint64_t addr, EntryParams* params, atexit_func_t atexit_func)
{
	reinterpret_cast<entry_func_t>(addr)(params, atexit_func);
}

static KYTY_SYSV_ABI int run_ini_fini(uint64_t addr, size_t args, const void* argp, module_func_t func)
{
	return reinterpret_cast<module_ini_fini_func_t>(addr)(args, argp, func);
}

static uint64_t get_aligned_size(const Elf64_Phdr* p)
{
	return (p->p_align != 0 ? (p->p_memsz + (p->p_align - 1)) & ~(p->p_align - 1) : p->p_memsz);
}

static void dbg_dump_symbols(const String& folder, Elf64_Sym* symbols, uint64_t size, const char* names)
{
	auto folder_str = folder.FixDirectorySlash();

	Core::File::CreateDirectories(folder_str);

	Core::File f;
	f.Create(folder_str + "symbols.txt");

	for (auto* sym = symbols; reinterpret_cast<uint8_t*>(sym) < reinterpret_cast<uint8_t*>(symbols) + size; sym++)
	{
		f.Printf("----\n");
		f.Printf("st_name = %" PRIu32 ", %s\n", sym->st_name, names + sym->st_name);
		f.Printf("st_info = 0x%02" PRIx8 "\n", sym->st_info);
		f.Printf("st_other = 0x%02" PRIx8 "\n", sym->st_other);
		f.Printf("st_shndx = 0x%04" PRIx16 "\n", sym->st_shndx);
		f.Printf("st_value = 0x%016" PRIx64 "\n", sym->st_value);
		f.Printf("st_size = %" PRIu64 "\n", sym->st_size);
	}

	f.Close();
}

static void dbg_dump_rela(const String& folder, Elf64_Rela* records, uint64_t size, const char* /*names*/, const char* file_name)
{
	auto folder_str = folder.FixDirectorySlash();

	Core::File::CreateDirectories(folder_str);

	Core::File f;
	f.Create(folder_str + file_name);

	for (auto* r = records; reinterpret_cast<uint8_t*>(r) < reinterpret_cast<uint8_t*>(records) + size; r++)
	{
		f.Printf("----\n"
		         "r_offset = 0x%016" PRIx64 "\n"
		         "r_info = 0x%016" PRIx64 "\n"
		         "r_addend = %" PRId64 "\n",
		         r->r_offset, r->r_info, r->r_addend);
	}

	f.Close();
}

static VirtualMemory::Mode get_mode(Elf64_Word flags)
{
	switch (flags)
	{
		case PF_R: return VirtualMemory::Mode::Read;
		case PF_W: return VirtualMemory::Mode::Write;
		case PF_R | PF_W: return VirtualMemory::Mode::ReadWrite;
		case PF_X: return VirtualMemory::Mode::Execute;
		case PF_X | PF_R: return VirtualMemory::Mode::ExecuteRead;
		case PF_X | PF_W: return VirtualMemory::Mode::ExecuteWrite;
		case PF_X | PF_W | PF_R: return VirtualMemory::Mode::ExecuteReadWrite;

		default: return VirtualMemory::Mode::NoAccess;
	}
}

struct FrameS
{
	FrameS*   next;
	uintptr_t ret_addr;
};

static void KYTY_SYSV_ABI stackwalk_x86(uint64_t rbp, void** stack, int* depth, uintptr_t stack_addr, size_t stack_size,
                                        uintptr_t code_addr, size_t code_size)
{
	auto* frame = reinterpret_cast<FrameS*>(rbp);

	int d = *depth;
	int i = 0;

	for (; i < d; i++)
	{
		if (!(reinterpret_cast<uintptr_t>(frame) >= stack_addr && reinterpret_cast<uintptr_t>(frame) < stack_addr + stack_size))
		{
			break;
		}

		if (!(frame->ret_addr >= code_addr && frame->ret_addr < code_addr + code_size))
		{
			break;
		}

		stack[i] = reinterpret_cast<void*>(frame->ret_addr);

		frame = frame->next;
	}

	*depth = i;
}

void KYTY_SYSV_ABI sys_stack_walk_x86(uint64_t rbp, void** stack, int* depth)
{
	stackwalk_x86(rbp, stack, depth, 0, UINT64_MAX, SYSTEM_RESERVED + CODE_BASE_OFFSET,
	              g_desired_base_addr - (SYSTEM_RESERVED + CODE_BASE_OFFSET));
}

static void kyty_exception_handler(const VirtualMemory::ExceptionHandler::ExceptionInfo* info)
{
	printf("kyty_exception_handler: %016" PRIx64 "\n", info->exception_address);

	if (info->type == VirtualMemory::ExceptionHandler::ExceptionType::AccessViolation)
	{
		if (info->access_violation_type == VirtualMemory::ExceptionHandler::AccessViolationType::Write &&
		    Libs::Graphics::GpuMemoryCheckAccessViolation(info->access_violation_vaddr, sizeof(uint64_t)))
		{
			return;
		}

		if (info->rbp != 0)
		{
			Core::Singleton<Loader::RuntimeLinker>::Instance()->StackTrace(info->rbp);
		}

		EXIT("Access violation: %s [%016" PRIx64 "] %s\n", Core::EnumName(info->access_violation_type).C_Str(),
		     info->access_violation_vaddr, (info->access_violation_vaddr == g_invalid_memory ? "(Unpatched object)" : ""));
	}

	EXIT("Unknown exception!!! (%08" PRIx32 ")", info->exception_win_code);
}

static void encode_id_64(uint16_t in_id, String* out_id)
{
	static const char32_t* str = U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
	if (in_id < 0x40u)
	{
		*out_id += str[in_id];
	} else
	{
		if (in_id < 0x1000u)
		{
			*out_id += str[static_cast<uint16_t>(in_id >> 6u) & 0x3fu];
			*out_id += str[in_id & 0x3fu];
		} else
		{
			*out_id += str[static_cast<uint16_t>(in_id >> 12u) & 0x3fu];
			*out_id += str[static_cast<uint16_t>(in_id >> 6u) & 0x3fu];
			*out_id += str[in_id & 0x3fu];
		}
	}
}

template <class T>
static void get_dyn_data_os(Elf64* elf, T* out, Elf64_Sxword tag)
{
	if (const auto* dyn = elf->GetDynValue(tag); dyn != nullptr)
	{
		*out = elf->GetDynamicData<T>(dyn->d_un.d_ptr);
	}
}

template <class T>
static void get_dyn_data(Elf64* elf, uint64_t base_vaddr, T* out, Elf64_Sxword tag)
{
	if (const auto* dyn = elf->GetDynValue(tag); dyn != nullptr)
	{
		*out = reinterpret_cast<T>(base_vaddr + dyn->d_un.d_ptr);
	}
}

template <class T>
static void get_dyn_value(Elf64* elf, T* out, Elf64_Sxword tag)
{
	if (const auto* dyn = elf->GetDynValue(tag); dyn != nullptr)
	{
		*out = dyn->d_un.d_val;
	}
}

template <class T>
static void get_dyn_values(Elf64* elf, T* out, Elf64_Sxword tag)
{
	for (const auto* dyn: elf->GetDynList(tag))
	{
		out->Add(dyn->d_un.d_val);
	}
}

template <class T>
static void get_dyn_ptr(Elf64* elf, T* out, Elf64_Sxword tag)
{
	if (const auto* dyn = elf->GetDynValue(tag); dyn != nullptr)
	{
		*out = dyn->d_un.d_ptr;
	}
}

static void KYTY_SYSV_ABI ProgramExitHandler()
{
	Core::Singleton<RuntimeLinker>::Instance()->StopAllModules();

	printf("exit!!!\n");
}

template <class T>
static void get_dyn_modules(Elf64* elf, T* out, const char* names, Elf64_Sxword tag)
{
	Vector<uint64_t> needed_modules;
	get_dyn_values(elf, &needed_modules, tag);
	for (auto need: needed_modules)
	{
		ModuleId id {};
		// id.id            = static_cast<int>((need >> 48u) & 0xffffu);
		encode_id_64(static_cast<uint16_t>((need >> 48u) & 0xffffu), &id.id);
		id.version_major = static_cast<int>((need >> 40u) & 0xffu);
		id.version_minor = static_cast<int>((need >> 32u) & 0xffu);
		id.name          = names + (need & 0xffffffff);
		out->Add(id);
	}
}

template <class T>
static void get_dyn_libs(Elf64* elf, T* out, const char* names, Elf64_Sxword tag)
{
	Vector<uint64_t> needed_modules;
	get_dyn_values(elf, &needed_modules, tag);
	for (auto need: needed_modules)
	{
		LibraryId id {};
		// id.id      = static_cast<int>((need >> 48u) & 0xffffu);
		encode_id_64(static_cast<uint16_t>((need >> 48u) & 0xffffu), &id.id);
		id.version = static_cast<int>((need >> 32u) & 0xffffu);
		id.name    = names + (need & 0xffffffff);
		out->Add(id);
	}
}

static RelocationInfo GetRelocationInfo(Elf64_Rela* r, Program* program)
{
	KYTY_PROFILER_FUNCTION();

	// KYTY_PROFILER_BLOCK("1");

	RelocationInfo ret;
	// SymbolRecord   sr {};

	// KYTY_PROFILER_END_BLOCK;

	// KYTY_PROFILER_BLOCK("2");

	auto         type    = r->GetType();
	auto         symbol  = r->GetSymbol();
	Elf64_Sxword addend  = r->r_addend;
	auto*        symbols = program->dynamic_info->symbol_table;
	auto*        names   = program->dynamic_info->str_table;
	ret.base_vaddr       = program->base_vaddr;
	ret.vaddr            = ret.base_vaddr + r->r_offset;
	ret.bind_self        = false;

	// KYTY_PROFILER_END_BLOCK;

	// KYTY_PROFILER_BLOCK("3");

	switch (type)
	{
		case R_X86_64_GLOB_DAT:
		case R_X86_64_JUMP_SLOT: addend = 0; [[fallthrough]];
		case R_X86_64_64:
		{
			auto         sym          = symbols[symbol];
			auto         bind         = sym.GetBind();
			auto         sym_type     = sym.GetType();
			uint64_t     symbol_vaddr = 0;
			SymbolRecord sr {};
			switch (sym_type)
			{
				case STT_NOTYPE: ret.type = SymbolType::NoType; break;
				case STT_FUNC: ret.type = SymbolType::Func; break;
				case STT_OBJECT: ret.type = SymbolType::Object; break;
				default: EXIT("unknown symbol type: %d\n", (int)sym_type);
			}
			switch (bind)
			{
				case STB_LOCAL:
					symbol_vaddr = ret.base_vaddr + sym.st_value;
					ret.bind     = BindType::Local;
					break;
				case STB_GLOBAL: ret.bind = BindType::Global; [[fallthrough]];
				case STB_WEAK:
				{
					ret.bind = (ret.bind == BindType::Unknown ? BindType::Weak : ret.bind);
					ret.name = names + sym.st_name;
					program->rt->Resolve(ret.name, ret.type, program, &sr, &ret.bind_self);
					symbol_vaddr = sr.vaddr;
				}
				break;
				default: EXIT("unknown bind: %d\n", (int)bind);
			}
			ret.resolved = (symbol_vaddr != 0);
			ret.value    = (ret.resolved ? symbol_vaddr + addend : 0);
			ret.name     = sr.name;
			ret.dbg_name = sr.dbg_name;
		}
		break;
		case R_X86_64_RELATIVE:
			ret.value    = ret.base_vaddr + addend;
			ret.resolved = true;
			break;
		case R_X86_64_DTPMOD64:
			ret.value    = reinterpret_cast<uint64_t>(program);
			ret.resolved = true;
			ret.type     = SymbolType::TlsModule;
			ret.bind     = BindType::Local;
			ret.dbg_name = program->file_name;
			break;
		default: EXIT("unknown type: %d\n", (int)type);
	}

	// KYTY_PROFILER_END_BLOCK;

	return ret;
}

static void relocate(uint32_t index, Elf64_Rela* r, Program* program, bool jmprela_table)
{
	KYTY_PROFILER_FUNCTION();

	auto ri = GetRelocationInfo(r, program);

	[[maybe_unused]] bool patched = false;

	// KYTY_PROFILER_BLOCK("patch");

	if (ri.resolved)
	{
		patched = VirtualMemory::PatchReplace(ri.vaddr, ri.value);
	} else
	{
		uint64_t value = 0;
		bool     weak  = (ri.bind == BindType::Weak || !program->fail_if_global_not_resolved);
		if (ri.type == SymbolType::Object && weak)
		{
			value = g_invalid_memory;
		} else if (ri.type == SymbolType::Func && jmprela_table && weak)
		{
			if (program->custom_call_plt_vaddr != 0)
			{
				EXIT_NOT_IMPLEMENTED(index >= program->custom_call_plt_num);
				value = reinterpret_cast<Jit::CallPlt*>(program->custom_call_plt_vaddr)->GetAddr(index);
			} else
			{
				value = RuntimeLinker::ReadFromElf(program, ri.vaddr) + ri.base_vaddr;
			}
		} else if ((ri.type == SymbolType::Func && !jmprela_table && weak) || (ri.type == SymbolType::NoType && weak))
		{
			value = RuntimeLinker::ReadFromElf(program, ri.vaddr) + ri.base_vaddr;
		}

		if (value != 0)
		{
			patched = VirtualMemory::PatchReplace(ri.vaddr, value);
		} else
		{
			auto dbg_str = String::FromPrintf("[%016" PRIx64 "] <- %s%016" PRIx64 "%s, %s, %s, %s, %s", ri.vaddr,
			                                  ri.value == 0 ? FG_BRIGHT_RED : FG_BRIGHT_GREEN, ri.value, DEFAULT, ri.name.C_Str(),
			                                  Core::EnumName(ri.type).C_Str(), Core::EnumName(ri.bind).C_Str(), ri.dbg_name.C_Str());

			EXIT("Can't resolve: %s\n", (Log::IsColoredPrintf() ? dbg_str : Log::RemoveColors(dbg_str)).C_Str());
		}
	}

	// KYTY_PROFILER_END_BLOCK;

	if (program->dbg_print_reloc)
	{
		if (/* !dbg_str.ContainsStr(U"libc_") && */ patched && !ri.bind_self &&
		    (ri.bind == BindType::Global || ri.bind == BindType::Weak || ri.type == SymbolType::TlsModule))
		{
			auto dbg_str = String::FromPrintf("[%016" PRIx64 "] <- %s%016" PRIx64 "%s, %s, %s, %s, %s", ri.vaddr,
			                                  ri.value == 0 ? FG_BRIGHT_RED : FG_BRIGHT_GREEN, ri.value, DEFAULT, ri.name.C_Str(),
			                                  Core::EnumName(ri.type).C_Str(), Core::EnumName(ri.bind).C_Str(), ri.dbg_name.C_Str());

			printf("Relocate: %s\n", dbg_str.C_Str());
		}
	}
}

static void relocate_all(Elf64_Rela* records, uint64_t size, Program* program, bool jmprela_table)
{
	KYTY_PROFILER_FUNCTION();

	uint32_t index = 0;
	for (auto* r = records; reinterpret_cast<uint8_t*>(r) < reinterpret_cast<uint8_t*>(records) + size; r++, index++)
	{
		relocate(index, r, program, jmprela_table);
	}
}

static KYTY_SYSV_ABI void RelocateHandler(RelocateHandlerStack s)
{
	auto*  stack     = s.stack;
	auto*  program   = reinterpret_cast<Program*>(stack[-1]);
	auto   rel_index = stack[0];
	String name      = U"<unknown function>";

	if (program != nullptr && program->dynamic_info != nullptr && program->dynamic_info->jmprela_table != nullptr)
	{
		auto ri = GetRelocationInfo(program->dynamic_info->jmprela_table + rel_index, program);

		name = String::FromPrintf(FG_BRIGHT_RED "%s" DEFAULT, ri.name.C_Str());
	}

	// Restore return address (for stack trace)
	stack[-1] = stack[1];

	Core::Singleton<Loader::RuntimeLinker>::Instance()->StackTrace(reinterpret_cast<uint64_t>(s.stack - 2));

	EXIT("=== Unpatched function!!! ===\n[%d]\t%s\n", Core::Thread::GetThreadIdUnique(),
	     (Log::IsColoredPrintf() ? name : Log::RemoveColors(name)).C_Str());
}

static KYTY_MS_ABI uint8_t* TlsMainGetAddr()
{
	EXIT_IF(g_tls_main_program == nullptr);

	if (memcmp(&g_tls_reg_save_area[XSAVE_BUFFER_SIZE], &XSAVE_CHK_GUARD, sizeof(XSAVE_CHK_GUARD)) != 0)
	{
		EXIT("xsave buffer is too small\n");
	}

	return RuntimeLinker::TlsGetAddr(g_tls_main_program) + g_tls_main_program->tls.image_size;
}

static void PatchProgram(Program* program, uint64_t address, uint64_t size)
{
	EXIT_IF(program == nullptr);
	EXIT_IF(program->elf == nullptr);

	if (!program->elf->IsShared() && program->tls.handler_vaddr != 0)
	{
		// Replace:
		//   mov rax, qword ptr fs:[0x00]
		// with:
		//   call <handler>
		//   mov rax,rax
		//   nop
		// TODO() sometimes prefix 666666 is present
		const uint8_t tls_pattern[9] = {0x64, 0x48, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00};

		EXIT_IF(Jit::Call9::GetSize() != sizeof(tls_pattern));

		auto* start_ptr = reinterpret_cast<uint8_t*>(address);
		auto* end_ptr   = start_ptr + size - sizeof(tls_pattern);

		for (auto* ptr = start_ptr; ptr < end_ptr; ptr++)
		{
			if (memcmp(ptr, tls_pattern, sizeof(tls_pattern)) == 0)
			{
				printf("Patch tls at addr: [%016" PRIx64 "]\n", reinterpret_cast<uint64_t>(ptr));

				auto* code = new (ptr) Jit::Call9;
				code->SetFunc(program->tls.handler_vaddr);
			}
		}
	}
}

uint64_t RuntimeLinker::GetEntry()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (const auto* p: m_programs)
	{
		if (p->elf != nullptr && !p->elf->IsShared())
		{
			return p->elf->GetEntry() + p->base_vaddr;
		}
	}
	return 0;
}

uint64_t RuntimeLinker::GetProcParam()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (const auto* p: m_programs)
	{
		if (p->elf != nullptr && !p->elf->IsShared())
		{
			return p->proc_param_vaddr;
		}
	}
	return 0;
}

void RuntimeLinker::DbgDump(const String& folder)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (const auto* p: m_programs)
	{
		auto folder_str = folder.FixDirectorySlash();
		folder_str += p->file_name.FilenameWithoutDirectory();

		EXIT_IF(p->elf == nullptr);

		p->elf->DbgDump(folder_str);

		if (p->dynamic_info != nullptr)
		{
			EXIT_NOT_IMPLEMENTED(p->dynamic_info->symbol_table_entry_size != 0 &&
			                     p->dynamic_info->symbol_table_entry_size != sizeof(Elf64_Sym));
			EXIT_NOT_IMPLEMENTED(p->dynamic_info->rela_table_entry_size != 0 &&
			                     p->dynamic_info->rela_table_entry_size != sizeof(Elf64_Rela));
			// EXIT_NOT_IMPLEMENTED(p->dynamic_info->jmprela_table == nullptr);
			// EXIT_NOT_IMPLEMENTED(p->dynamic_info->rela_table == nullptr);
			// EXIT_NOT_IMPLEMENTED(p->dynamic_info->symbol_table == nullptr);

			if (p->dynamic_info->symbol_table != nullptr)
			{
				dbg_dump_symbols(folder_str, p->dynamic_info->symbol_table, p->dynamic_info->symbol_table_total_size,
				                 p->dynamic_info->str_table);
			}
			if (p->dynamic_info->jmprela_table != nullptr)
			{
				dbg_dump_rela(folder_str, p->dynamic_info->jmprela_table, p->dynamic_info->jmprela_table_size, p->dynamic_info->str_table,
				              "jmprela_table.txt");
			}
			if (p->dynamic_info->rela_table != nullptr)
			{
				dbg_dump_rela(folder_str, p->dynamic_info->rela_table, p->dynamic_info->rela_table_total_size, p->dynamic_info->str_table,
				              "rela_table.txt");
			}
		}

		if (p->export_symbols != nullptr)
		{
			p->export_symbols->DbgDump(folder_str, U"export_symbols.txt");
		}
		if (p->import_symbols != nullptr)
		{
			p->import_symbols->DbgDump(folder_str, U"import_symbols.txt");
		}
	}
}

void RuntimeLinker::RelocateAll()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		Relocate(p);
	}

	m_relocated = true;
}

void RuntimeLinker::UnloadProgram(Program* program)
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	if (auto index = m_programs.Find(program); m_programs.IndexValid(index))
	{
		DeleteProgram(m_programs.At(index));
		m_programs.RemoveAt(index);
	} else
	{
		EXIT("program not found");
	}

	if (m_relocated)
	{
		RelocateAll();
	}
}

RuntimeLinker::RuntimeLinker(): m_symbols(new SymbolDatabase)
{
	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());
}

RuntimeLinker::~RuntimeLinker()
{
	Clear();
}

Program* RuntimeLinker::LoadProgram(const String& elf_name)
{
	KYTY_PROFILER_FUNCTION();

	Core::LockGuard lock(m_mutex);

	static int32_t id_seq = 0;

	printf("Loading: %s\n", elf_name.C_Str());

	auto* program = new Program;

	program->rt        = this;
	program->file_name = elf_name;
	program->unique_id = ++id_seq;

	program->elf = new Elf64;
	program->elf->Open(elf_name);

	if (program->elf->IsValid())
	{
		LoadProgramToMemory(program);
		ParseProgramDynamicInfo(program);
		CreateSymbolDatabase(program);
	} else
	{
		EXIT("elf is not valid: %s\n", elf_name.C_Str());
	}

	m_programs.Add(program);

	if (!program->elf->IsShared())
	{
		program->fail_if_global_not_resolved = false;
		Libs::LibKernel::SetProgName(elf_name.FilenameWithoutDirectory());
	}

	if (/*elf_name.FilenameWithoutExtension().EndsWith(U"libc") || elf_name.FilenameWithoutExtension().EndsWith(U"Fios2") ||
	    elf_name.FilenameWithoutExtension().EndsWith(U"Fios2_debug") || elf_name.FilenameWithoutExtension().EndsWith(U"NpToolkit") ||
	    elf_name.FilenameWithoutExtension().EndsWith(U"NpToolkit2") || elf_name.FilenameWithoutExtension().EndsWith(U"JobManager")*/
	    elf_name.DirectoryWithoutFilename().EndsWith(U"_module/", String::Case::Insensitive))
	{
		program->fail_if_global_not_resolved = false;
	}

	return program;
}

void RuntimeLinker::SaveMainProgram(const String& elf_name)
{
	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (const auto* p: m_programs)
	{
		EXIT_IF(p->elf == nullptr);

		if (!p->elf->IsShared())
		{
			p->elf->Save(elf_name);
			break;
		}
	}
}

void RuntimeLinker::SaveProgram(Program* program, const String& elf_name)
{
	EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	if (auto index = m_programs.Find(program); m_programs.IndexValid(index))
	{
		EXIT_IF(m_programs.At(index)->elf == nullptr);

		m_programs.At(index)->elf->Save(elf_name);
	} else
	{
		EXIT("program not found");
	}
}

void RuntimeLinker::Execute()
{
	KYTY_PROFILER_THREAD("Thread_Main");

	Libs::LibKernel::PthreadInitSelfForMainThread();

	RelocateAll();
	StartAllModules();

	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");
	printf(FG_BRIGHT_YELLOW "--- Execute: " BOLD BG_BLUE "%s" BG_DEFAULT NO_BOLD DEFAULT "\n", "Main");
	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");

	// Reserve some stack. There may be jumps over guard page. To prevent segfault we need to expand committed area.

	size_t expanded_size = 0;
	size_t expanded_max  = static_cast<size_t>(256) * 1024;

	while (expanded_size < expanded_max)
	{
		sys_dbg_stack_info_t s {};
		sys_stack_usage(s);
		*reinterpret_cast<uint32_t*>(s.guard_addr) = 0;
		expanded_size += s.guard_size;
	}

	if (auto entry = GetEntry(); entry != 0)
	{
		EntryParams p {};
		p.argc    = 1;
		p.argv[0] = "KytyEmu";

		printf("stack_addr = %" PRIx64 "\n", reinterpret_cast<uint64_t>(&p));

		run_entry(entry, &p, ProgramExitHandler);
	}
}

void RuntimeLinker::Clear()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		DeleteProgram(p);
	}
	m_programs.Clear();
	delete m_symbols;
	m_symbols   = nullptr;
	m_relocated = false;
}

void RuntimeLinker::Resolve(const String& name, SymbolType type, Program* program, SymbolRecord* out_info, bool* bind_self)
{
	KYTY_PROFILER_FUNCTION();

	Core::LockGuard lock(m_mutex);

	EXIT_IF(out_info == nullptr);

	auto ids = name.Split(U'#');

	if (bind_self != nullptr)
	{
		*bind_self = false;
	}

	if (ids.Size() == 3)
	{
		const LibraryId* l = FindLibrary(*program, ids.At(1));
		const ModuleId*  m = FindModule(*program, ids.At(2));

		if (l != nullptr && m != nullptr)
		{
			SymbolResolve sr {};
			sr.name                 = ids.At(0);
			sr.library              = l->name;
			sr.library_version      = l->version;
			sr.module               = m->name;
			sr.module_version_major = m->version_major;
			sr.module_version_minor = m->version_minor;
			sr.type                 = type;

			const SymbolRecord* rec = nullptr;

			if (m_symbols != nullptr)
			{
				rec = m_symbols->Find(sr);
			}

			if (rec == nullptr)
			{
				if (auto* p = FindProgram(*m, *l); p != nullptr && p->export_symbols != nullptr)
				{
					rec = p->export_symbols->Find(sr);
					if (bind_self != nullptr)
					{
						*bind_self = (p == program);
					}
				}
			}

			if (rec != nullptr)
			{
				//*out_vaddr = rec->vaddr;
				*out_info = *rec;
			} else
			{
				out_info->vaddr    = 0;
				out_info->name     = SymbolDatabase::GenerateName(sr);
				out_info->dbg_name = U"";
			}
		} else
		{
			EXIT("l == nullptr || m == nullptr");
		}
	} else
	{
		out_info->vaddr    = 0;
		out_info->name     = name;
		out_info->dbg_name = U"";
	}
}

uint64_t RuntimeLinker::ReadFromElf(Program* program, uint64_t vaddr)
{
	EXIT_IF(program == nullptr);
	EXIT_IF(program->base_vaddr == 0 || program->base_size == 0);
	EXIT_IF(program->elf == nullptr);

	uint64_t ret = 0;

	const auto* ehdr = program->elf->GetEhdr();
	const auto* phdr = program->elf->GetPhdr();

	EXIT_IF(phdr == nullptr || ehdr == nullptr);

	for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_OS_RELRO))
		{
			uint64_t segment_addr      = phdr[i].p_vaddr + program->base_vaddr;
			uint64_t segment_file_size = phdr[i].p_filesz;

			if (vaddr >= segment_addr && vaddr < segment_addr + segment_file_size)
			{
				program->elf->LoadSegment(reinterpret_cast<uint64_t>(&ret), phdr[i].p_offset + vaddr - segment_addr, sizeof(ret));
				break;
			}
		}
	}

	return ret;
}

Program* RuntimeLinker::FindProgramById(int32_t id)
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		if (p->unique_id == id)
		{
			return p;
		}
	}

	return nullptr;
}

Program* RuntimeLinker::FindProgramByAddr(uint64_t vaddr)
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		const auto* ehdr = p->elf->GetEhdr();
		const auto* phdr = p->elf->GetPhdr();

		EXIT_IF(phdr == nullptr || ehdr == nullptr);

		for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
		{
			if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_OS_RELRO))
			{
				uint64_t segment_addr = phdr[i].p_vaddr + p->base_vaddr;
				uint64_t segment_size = get_aligned_size(phdr + i);

				if (vaddr >= segment_addr && vaddr < segment_addr + segment_size)
				{
					return p;
				}
			}
		}
	}

	return nullptr;
}

void RuntimeLinker::StackTrace(uint64_t frame_ptr)
{
	void* stack[20];
	int   depth = 20;

	sys_stack_walk_x86(frame_ptr, stack, &depth);

	std::printf("Stack trace [thread = %d]:\n", Core::Thread::GetThreadIdUnique());

	for (int i = 0; i < depth; i++)
	{
		auto  vaddr = reinterpret_cast<uint64_t>(stack[i]);
		auto* p     = FindProgramByAddr(vaddr);
		std::printf("[%d] %016" PRIx64 ", %s\n", i, vaddr, (p == nullptr ? "???" : p->file_name.FilenameWithoutDirectory().C_Str()));
	}
}

void RuntimeLinker::StartAllModules()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		if (p->elf->IsShared())
		{
			StartModule(p, 0, nullptr, nullptr);
		}
	}
}

void RuntimeLinker::StopAllModules()
{
	// EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread());

	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		if (p->elf->IsShared())
		{
			StopModule(p, 0, nullptr, nullptr);
		}
	}
}

int RuntimeLinker::StartModule(Program* program, size_t args, const void* argp, module_func_t func)
{
	EXIT_IF(program == nullptr);
	EXIT_IF(program->dynamic_info == nullptr);
	EXIT_IF(program->elf == nullptr);
	EXIT_IF(!program->elf->IsShared());

	EXIT_IF(!m_programs.Contains(program));

	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");
	printf(FG_BRIGHT_YELLOW "--- Start module: " BG_BLUE BOLD "%s" BG_DEFAULT NO_BOLD DEFAULT "\n", program->file_name.C_Str());
	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");

	return run_ini_fini(program->dynamic_info->init_vaddr + program->base_vaddr, args, argp, func);
}

int RuntimeLinker::StopModule(Program* program, size_t args, const void* argp, module_func_t func)
{
	EXIT_IF(program == nullptr);
	EXIT_IF(program->dynamic_info == nullptr);
	EXIT_IF(program->elf == nullptr);
	EXIT_IF(!program->elf->IsShared());

	EXIT_IF(!m_programs.Contains(program));

	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");
	printf(FG_BRIGHT_YELLOW "--- Stop module: " BG_BLUE BOLD "%s" BG_DEFAULT NO_BOLD DEFAULT "\n", program->file_name.C_Str());
	printf(FG_BRIGHT_YELLOW "---" DEFAULT "\n");

	int result = run_ini_fini(program->dynamic_info->fini_vaddr + program->base_vaddr, args, argp, func);

	Libs::LibKernel::PthreadDeleteStaticObjects(program);

	return result;
}

uint8_t* RuntimeLinker::TlsGetAddr(Program* program)
{
	EXIT_IF(program == nullptr);

	program->tls.mutex.Lock();

	auto& tls = program->tls.tlss.GetOrPutDef(Core::Thread::GetThreadIdUnique(), nullptr);

	if (tls == nullptr)
	{
		tls = new uint8_t[program->tls.image_size];
		std::memcpy(tls, reinterpret_cast<void*>(program->tls.image_vaddr), program->tls.image_size);
	}

	uint8_t* ret = tls;

	program->tls.mutex.Unlock();

	return ret;
}

void RuntimeLinker::DeleteTls(Program* program, int thread_id)
{
	EXIT_IF(program == nullptr);

	program->tls.mutex.Lock();

	delete[] program->tls.tlss.Get(thread_id, nullptr);
	program->tls.tlss.Remove(thread_id);

	program->tls.mutex.Unlock();
}

static uint64_t calc_base_size(const Elf64_Ehdr* ehdr, const Elf64_Phdr* phdr)
{
	uint64_t base_size = 0;
	for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_OS_RELRO))
		{
			uint64_t last_addr = phdr[i].p_vaddr + get_aligned_size(phdr + i);
			if (last_addr > base_size)
			{
				base_size = last_addr;
			}
		}
	}
	return base_size;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void RuntimeLinker::LoadProgramToMemory(Program* program)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(program == nullptr || program->base_vaddr != 0 || program->base_size != 0 || program->elf == nullptr ||
	        program->exception_handler != nullptr);

	// static uint64_t desired_base_addr = DESIRED_BASE_ADDR;

	bool is_shared   = program->elf->IsShared();
	bool is_next_gen = program->elf->IsNextGen();

	const auto* ehdr = program->elf->GetEhdr();
	const auto* phdr = program->elf->GetPhdr();

	EXIT_IF(phdr == nullptr || ehdr == nullptr);

	if (is_next_gen && !is_shared)
	{
		Config::SetNextGen(true);
	}

	program->base_size         = calc_base_size(ehdr, phdr);
	program->base_size_aligned = (program->base_size & ~(static_cast<uint64_t>(0x1000) - 1)) + 0x1000;

	uint64_t exception_handler_size = VirtualMemory::ExceptionHandler::GetSize();
	uint64_t tls_handler_size       = is_shared ? 0 : Jit::SafeCall::GetSize();
	uint64_t alloc_size             = program->base_size_aligned + exception_handler_size + tls_handler_size;

	program->base_vaddr = VirtualMemory::Alloc(g_desired_base_addr, alloc_size, VirtualMemory::Mode::ExecuteReadWrite);

	if (!is_shared)
	{
		program->tls.handler_vaddr = program->base_vaddr + program->base_size_aligned + exception_handler_size;
	}

	g_desired_base_addr += CODE_BASE_INCR * (1 + alloc_size / CODE_BASE_INCR);

	EXIT_IF(program->base_vaddr == 0);
	EXIT_IF(program->base_size_aligned < program->base_size);

	printf("base_vaddr             = 0x%016" PRIx64 "\n", program->base_vaddr);
	printf("base_size              = 0x%016" PRIx64 "\n", program->base_size);
	printf("base_size_aligned      = 0x%016" PRIx64 "\n", program->base_size_aligned);
	printf("exception_handler_size = 0x%016" PRIx64 "\n", exception_handler_size);
	if (!is_shared)
	{
		printf("tls_handler_size       = 0x%016" PRIx64 "\n", tls_handler_size);
	}

	program->exception_handler = new VirtualMemory::ExceptionHandler;

	if (is_shared)
	{
		program->exception_handler->Install(program->base_vaddr, program->base_vaddr + program->base_size_aligned,
		                                    program->base_size_aligned + exception_handler_size + tls_handler_size, kyty_exception_handler);
	} else
	{
		program->exception_handler->Install(SYSTEM_RESERVED, program->base_vaddr + program->base_size_aligned,
		                                    program->base_vaddr + program->base_size_aligned + exception_handler_size + tls_handler_size -
		                                        SYSTEM_RESERVED,
		                                    kyty_exception_handler);

		// if (Libs::Graphics::GpuMemoryWatcherEnabled())
		{
			VirtualMemory::ExceptionHandler::InstallVectored(kyty_exception_handler);
		}
	}

	// program->elf->SetBaseVAddr(program->base_vaddr);

	for (Elf64_Half i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_OS_RELRO))
		{
			uint64_t segment_addr        = phdr[i].p_vaddr + program->base_vaddr;
			uint64_t segment_file_size   = phdr[i].p_filesz;
			uint64_t segment_memory_size = get_aligned_size(phdr + i);
			auto     mode                = get_mode(phdr[i].p_flags);

			printf("[%d] addr        = 0x%016" PRIx64 "\n", i, segment_addr);
			printf("[%d] file_size   = %" PRIu64 "\n", i, segment_file_size);
			printf("[%d] memory_size = %" PRIu64 "\n", i, segment_memory_size);
			printf("[%d] mode        = %s\n", i, Core::EnumName(mode).C_Str());

			program->elf->LoadSegment(segment_addr, phdr[i].p_offset, segment_file_size);

			bool skip_protect = (phdr[i].p_type == PT_LOAD && is_next_gen && mode == VirtualMemory::Mode::NoAccess);

			if (VirtualMemory::IsExecute(mode))
			{
				PatchProgram(program, segment_addr, segment_memory_size);
			}

			if (!skip_protect)
			{
				VirtualMemory::Protect(segment_addr, segment_memory_size, mode);

				if (VirtualMemory::IsExecute(mode))
				{
					VirtualMemory::FlushInstructionCache(segment_addr, segment_memory_size);
				}
			}
		}

		if (phdr[i].p_type == PT_TLS)
		{
			EXIT_IF(phdr[i].p_vaddr >= program->base_size);

			program->tls.image_vaddr = phdr[i].p_vaddr + program->base_vaddr;
			program->tls.image_size  = get_aligned_size(phdr + i);

			printf("tls addr = 0x%016" PRIx64 "\n", program->tls.image_vaddr);
			printf("tls size   = %" PRIu64 "\n", program->tls.image_size);
		}

		if (phdr[i].p_type == PT_OS_PROCPARAM)
		{
			EXIT_IF(program->proc_param_vaddr != 0);
			EXIT_IF(phdr[i].p_vaddr >= program->base_size);

			program->proc_param_vaddr = phdr[i].p_vaddr + program->base_vaddr;
		}
	}

	if (!is_shared)
	{
		SetupTlsHandler(program);
	}

	printf("entry = 0x%016" PRIx64 "\n", program->elf->GetEntry() + program->base_vaddr);
}

void RuntimeLinker::DeleteProgram(Program* p)
{
	if (p->base_vaddr != 0 || p->base_size != 0)
	{
		VirtualMemory::Free(p->base_vaddr);
	}

	if (p->custom_call_plt_vaddr != 0 || p->custom_call_plt_num != 0)
	{
		VirtualMemory::Free(p->custom_call_plt_vaddr);
	}

	delete p->elf;
	delete p->dynamic_info;
	delete p->exception_handler;
	delete p->export_symbols;
	delete p->import_symbols;

	FOR_HASH (p->tls.tlss)
	{
		delete p->tls.tlss.Value();
	}

	delete p;
}

void RuntimeLinker::ParseProgramDynamicInfo(Program* program)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(program == nullptr);
	EXIT_IF(program->elf == nullptr);
	EXIT_IF(program->dynamic_info != nullptr);

	program->dynamic_info = new DynamicInfo;

	auto* elf = program->elf;

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_HASH) && elf->HasDynValue(DT_HASH));
	get_dyn_data_os(elf, &program->dynamic_info->hash_table, DT_OS_HASH);
	get_dyn_data(elf, program->base_vaddr, &program->dynamic_info->hash_table, DT_HASH);
	get_dyn_value(elf, &program->dynamic_info->hash_table_size, DT_OS_HASHSZ);

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_STRTAB) && elf->HasDynValue(DT_STRTAB));
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_STRSZ) && elf->HasDynValue(DT_STRSZ));
	get_dyn_data_os(elf, &program->dynamic_info->str_table, DT_OS_STRTAB);
	get_dyn_data(elf, program->base_vaddr, &program->dynamic_info->str_table, DT_STRTAB);
	get_dyn_value(elf, &program->dynamic_info->str_table_size, DT_OS_STRSZ);
	get_dyn_value(elf, &program->dynamic_info->str_table_size, DT_STRSZ);

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_SYMTAB) && elf->HasDynValue(DT_SYMTAB));
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_SYMENT) && elf->HasDynValue(DT_SYMENT));
	get_dyn_data_os(elf, &program->dynamic_info->symbol_table, DT_OS_SYMTAB);
	get_dyn_data(elf, program->base_vaddr, &program->dynamic_info->symbol_table, DT_SYMTAB);
	get_dyn_value(elf, &program->dynamic_info->symbol_table_total_size, DT_OS_SYMTABSZ);
	get_dyn_value(elf, &program->dynamic_info->symbol_table_entry_size, DT_OS_SYMENT);
	get_dyn_value(elf, &program->dynamic_info->symbol_table_entry_size, DT_SYMENT);

	get_dyn_ptr(elf, &program->dynamic_info->init_vaddr, DT_INIT);
	get_dyn_ptr(elf, &program->dynamic_info->fini_vaddr, DT_FINI);
	get_dyn_ptr(elf, &program->dynamic_info->init_array_vaddr, DT_INIT_ARRAY);
	get_dyn_ptr(elf, &program->dynamic_info->fini_array_vaddr, DT_FINI_ARRAY);
	get_dyn_ptr(elf, &program->dynamic_info->preinit_array_vaddr, DT_PREINIT_ARRAY);
	get_dyn_value(elf, &program->dynamic_info->init_array_size, DT_INIT_ARRAYSZ);
	get_dyn_value(elf, &program->dynamic_info->fini_array_size, DT_FINI_ARRAYSZ);
	get_dyn_value(elf, &program->dynamic_info->preinit_array_size, DT_PREINIT_ARRAYSZ);

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_PLTGOT) && elf->HasDynValue(DT_PLTGOT));
	get_dyn_ptr(elf, &program->dynamic_info->pltgot_vaddr, DT_OS_PLTGOT);
	get_dyn_ptr(elf, &program->dynamic_info->pltgot_vaddr, DT_PLTGOT);

	Elf64_Sxword jmprel_type = 0;
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_PLTREL) && elf->HasDynValue(DT_PLTREL));
	get_dyn_value(elf, &jmprel_type, DT_OS_PLTREL);
	get_dyn_value(elf, &jmprel_type, DT_PLTREL);

	EXIT_NOT_IMPLEMENTED(jmprel_type != DT_RELA);
	if (jmprel_type == DT_RELA)
	{
		EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_JMPREL) && elf->HasDynValue(DT_JMPREL));
		EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_PLTRELSZ) && elf->HasDynValue(DT_PLTRELSZ));
		get_dyn_data_os(elf, &program->dynamic_info->jmprela_table, DT_OS_JMPREL);
		get_dyn_data(elf, program->base_vaddr, &program->dynamic_info->jmprela_table, DT_JMPREL);
		get_dyn_value(elf, &program->dynamic_info->jmprela_table_size, DT_OS_PLTRELSZ);
		get_dyn_value(elf, &program->dynamic_info->jmprela_table_size, DT_PLTRELSZ);
	}

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_RELA) && elf->HasDynValue(DT_RELA));
	get_dyn_data_os(elf, &program->dynamic_info->rela_table, DT_OS_RELA);
	get_dyn_data(elf, program->base_vaddr, &program->dynamic_info->rela_table, DT_RELA);
	get_dyn_value(elf, &program->dynamic_info->rela_table_total_size, DT_OS_RELASZ);
	get_dyn_value(elf, &program->dynamic_info->rela_table_total_size, DT_RELASZ);
	get_dyn_value(elf, &program->dynamic_info->rela_table_entry_size, DT_OS_RELAENT);
	get_dyn_value(elf, &program->dynamic_info->rela_table_entry_size, DT_RELAENT);

	get_dyn_value(elf, &program->dynamic_info->relative_count, DT_RELACOUNT);

	get_dyn_value(elf, &program->dynamic_info->debug, DT_DEBUG);
	get_dyn_value(elf, &program->dynamic_info->flags, DT_FLAGS);
	get_dyn_value(elf, &program->dynamic_info->textrel, DT_TEXTREL);

	EXIT_NOT_IMPLEMENTED(program->dynamic_info->debug != 0);
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->textrel != 0);

	Vector<uint64_t> needed;
	get_dyn_values(elf, &needed, DT_NEEDED);
	for (auto need: needed)
	{
		program->dynamic_info->needed.Add(program->dynamic_info->str_table + need);
	}

	uint64_t so_name = 0;
	get_dyn_value(elf, &so_name, DT_SONAME);
	program->dynamic_info->so_name = program->dynamic_info->str_table + so_name;

	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_NEEDED_MODULE) && elf->HasDynValue(DT_OS_NEEDED_MODULE_1));
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_MODULE_INFO) && elf->HasDynValue(DT_OS_MODULE_INFO_1));
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_IMPORT_LIB) && elf->HasDynValue(DT_OS_IMPORT_LIB_1));
	EXIT_NOT_IMPLEMENTED(elf->HasDynValue(DT_OS_EXPORT_LIB) && elf->HasDynValue(DT_OS_EXPORT_LIB_1));
	get_dyn_modules(elf, &program->dynamic_info->import_modules, program->dynamic_info->str_table, DT_OS_NEEDED_MODULE);
	get_dyn_modules(elf, &program->dynamic_info->import_modules, program->dynamic_info->str_table, DT_OS_NEEDED_MODULE_1);
	get_dyn_modules(elf, &program->dynamic_info->export_modules, program->dynamic_info->str_table, DT_OS_MODULE_INFO);
	get_dyn_modules(elf, &program->dynamic_info->export_modules, program->dynamic_info->str_table, DT_OS_MODULE_INFO_1);
	get_dyn_libs(elf, &program->dynamic_info->import_libs, program->dynamic_info->str_table, DT_OS_IMPORT_LIB);
	get_dyn_libs(elf, &program->dynamic_info->import_libs, program->dynamic_info->str_table, DT_OS_IMPORT_LIB_1);
	get_dyn_libs(elf, &program->dynamic_info->export_libs, program->dynamic_info->str_table, DT_OS_EXPORT_LIB);
	get_dyn_libs(elf, &program->dynamic_info->export_libs, program->dynamic_info->str_table, DT_OS_EXPORT_LIB_1);
}

static void InstallRelocateHandler(Program* program)
{
	KYTY_PROFILER_FUNCTION();

	uint64_t pltgot_vaddr = program->dynamic_info->pltgot_vaddr + program->base_vaddr;
	uint64_t pltgot_size  = static_cast<uint64_t>(3) * 8;
	void**   pltgot       = reinterpret_cast<void**>(pltgot_vaddr);

	VirtualMemory::Mode old_mode {};
	VirtualMemory::Protect(pltgot_vaddr, pltgot_size, VirtualMemory::Mode::Write, &old_mode);

	pltgot[1] = program;
	pltgot[2] = reinterpret_cast<void*>(RelocateHandler);

	VirtualMemory::Protect(pltgot_vaddr, pltgot_size, old_mode);

	if (VirtualMemory::IsExecute(old_mode))
	{
		VirtualMemory::FlushInstructionCache(pltgot_vaddr, pltgot_size);
	}

	// TODO(): check if this table already generated by compiler (sometimes it is missing)
	if (program->custom_call_plt_vaddr == 0)
	{
		program->custom_call_plt_num   = program->dynamic_info->jmprela_table_size / sizeof(Elf64_Rela);
		auto size                      = Jit::CallPlt::GetSize(program->custom_call_plt_num);
		program->custom_call_plt_vaddr = VirtualMemory::Alloc(SYSTEM_RESERVED, size, VirtualMemory::Mode::Write);
		EXIT_NOT_IMPLEMENTED(program->custom_call_plt_vaddr == 0);
		auto* code = new (reinterpret_cast<void*>(program->custom_call_plt_vaddr)) Jit::CallPlt(program->custom_call_plt_num);
		code->SetPltGot(pltgot_vaddr);
		VirtualMemory::Protect(program->custom_call_plt_vaddr, size, VirtualMemory::Mode::Execute);
		VirtualMemory::FlushInstructionCache(program->custom_call_plt_vaddr, size);
	}
}

void RuntimeLinker::Relocate(Program* program)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(program == nullptr);

	if (g_invalid_memory == 0)
	{
		g_invalid_memory = VirtualMemory::Alloc(INVALID_MEMORY, 4096, VirtualMemory::Mode::NoAccess);
		EXIT_NOT_IMPLEMENTED(g_invalid_memory == 0);
	}

	printf("--- Relocate program: " FG_WHITE BOLD "%s" DEFAULT " ---\n", program->file_name.C_Str());

	EXIT_NOT_IMPLEMENTED(program->dynamic_info->symbol_table_entry_size != sizeof(Elf64_Sym));
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->rela_table_entry_size != sizeof(Elf64_Rela));
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->jmprela_table == nullptr);
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->rela_table == nullptr);
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->symbol_table == nullptr);
	EXIT_NOT_IMPLEMENTED(program->dynamic_info->pltgot_vaddr == 0);

	InstallRelocateHandler(program);

	relocate_all(program->dynamic_info->rela_table, program->dynamic_info->rela_table_total_size, program, false);
	relocate_all(program->dynamic_info->jmprela_table, program->dynamic_info->jmprela_table_size, program, true);
}

Program* RuntimeLinker::FindProgram(const ModuleId& m, const LibraryId& l)
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		const auto& export_libs    = p->dynamic_info->export_libs;
		const auto& export_modules = p->dynamic_info->export_modules;

		if (export_libs.Contains(l) && export_modules.Contains(m))
		{
			return p;
		}
	}
	return nullptr;
}

const ModuleId* RuntimeLinker::FindModule(const Program& program, const String& id)
{
	const auto& import_modules = program.dynamic_info->import_modules;

	if (auto index = import_modules.Find(id, [](auto module, auto str) { return module.id == str; }); import_modules.IndexValid(index))
	{
		return &import_modules.At(index);
	}

	const auto& export_modules = program.dynamic_info->export_modules;

	if (auto index = export_modules.Find(id, [](auto module, auto str) { return module.id == str; }); export_modules.IndexValid(index))
	{
		return &export_modules.At(index);
	}

	return nullptr;
}

// void RuntimeLinker::CreateTls() {}

const LibraryId* RuntimeLinker::FindLibrary(const Program& program, const String& id)
{
	const auto& import_libs = program.dynamic_info->import_libs;

	if (auto index = import_libs.Find(id, [](auto lib, auto str) { return lib.id == str; }); import_libs.IndexValid(index))
	{
		return &import_libs.At(index);
	}

	const auto& export_libs = program.dynamic_info->export_libs;

	if (auto index = export_libs.Find(id, [](auto lib, auto str) { return lib.id == str; }); export_libs.IndexValid(index))
	{
		return &export_libs.At(index);
	}

	return nullptr;
}

void RuntimeLinker::CreateSymbolDatabase(Program* program)
{
	KYTY_PROFILER_FUNCTION();

	EXIT_IF(program == nullptr);
	EXIT_IF(program->export_symbols != nullptr);
	EXIT_IF(program->import_symbols != nullptr);

	program->export_symbols = new SymbolDatabase;
	program->import_symbols = new SymbolDatabase;

	auto syms = [](Program* program, SymbolDatabase* symbols, bool is_export)
	{
		if (program->dynamic_info->symbol_table == nullptr || program->dynamic_info->str_table == nullptr)
		{
			return;
		}

		for (auto* sym = program->dynamic_info->symbol_table;
		     reinterpret_cast<uint8_t*>(sym) <
		     reinterpret_cast<uint8_t*>(program->dynamic_info->symbol_table) + program->dynamic_info->symbol_table_total_size;
		     sym++)
		{
			String id   = String::FromUtf8(program->dynamic_info->str_table + sym->st_name);
			auto   bind = sym->GetBind();
			auto   type = sym->GetType();
			auto   ids  = id.Split(U'#');

			if (ids.Size() == 3)
			{
				const auto* l = FindLibrary(*program, ids.At(1));
				const auto* m = FindModule(*program, ids.At(2));

				if (l != nullptr && m != nullptr && (bind == STB_GLOBAL || bind == STB_WEAK) && (type == STT_FUNC || type == STT_OBJECT) &&
				    is_export == (sym->st_value != 0))
				{
					SymbolResolve sr {};
					sr.name                 = ids.At(0);
					sr.library              = l->name;
					sr.library_version      = l->version;
					sr.module               = m->name;
					sr.module_version_major = m->version_major;
					sr.module_version_minor = m->version_minor;
					switch (type)
					{
						case STT_NOTYPE: sr.type = SymbolType::NoType; break;
						case STT_FUNC: sr.type = SymbolType::Func; break;
						case STT_OBJECT: sr.type = SymbolType::Object; break;
						default: sr.type = SymbolType::Unknown; break;
					}
					symbols->Add(sr, (is_export ? sym->st_value + program->base_vaddr : 0));
				}
			}
		}
	};

	syms(program, program->export_symbols, true);
	syms(program, program->import_symbols, false);
}

void RuntimeLinker::SetupTlsHandler(Program* program)
{
	EXIT_IF(program == nullptr);
	EXIT_IF(g_tls_main_program != nullptr);
	EXIT_IF(program->elf == nullptr);
	EXIT_IF(program->elf->IsShared());
	EXIT_IF(program->tls.handler_vaddr == 0);

	g_tls_main_program = program;

	auto* code = new (reinterpret_cast<void*>(program->tls.handler_vaddr)) Jit::SafeCall;

	memset(g_tls_reg_save_area, 0, XSAVE_BUFFER_SIZE);
	std::memcpy(&g_tls_reg_save_area[XSAVE_BUFFER_SIZE], &XSAVE_CHK_GUARD, sizeof(XSAVE_CHK_GUARD));

	code->SetFunc(TlsMainGetAddr);
	code->SetRegSaveArea(g_tls_reg_save_area);
	code->SetLockVar(&g_tls_spinlock);

	VirtualMemory::Protect(program->tls.handler_vaddr, Jit::SafeCall::GetSize(), VirtualMemory::Mode::Execute);
	VirtualMemory::FlushInstructionCache(program->tls.handler_vaddr, Jit::SafeCall::GetSize());
}

void RuntimeLinker::DeleteTlss(int thread_id)
{
	Core::LockGuard lock(m_mutex);

	for (auto* p: m_programs)
	{
		DeleteTls(p, thread_id);
	}
}

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED
