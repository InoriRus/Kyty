#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_RUNTIMELINKER_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_RUNTIMELINKER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader {

class Elf64;
struct Elf64_Sym;
struct Elf64_Rela;
class RuntimeLinker;

namespace VirtualMemory {
class ExceptionHandler;
} // namespace VirtualMemory

using module_func_t = int (*)(size_t args, const void* argp);

struct ModuleId
{
	bool operator==(const ModuleId& other) const
	{
		return version_major == other.version_major && version_minor == other.version_minor && name == other.name;
	}

	String id;
	int    version_major;
	int    version_minor;
	String name;
};

struct LibraryId
{
	bool operator==(const LibraryId& other) const { return version == other.version && name == other.name; }

	String id;
	int    version;
	String name;
};

struct ThreadLocalStorage
{
	uint64_t image_vaddr   = 0;
	uint64_t image_size    = 0;
	uint64_t handler_vaddr = 0;

	Core::Hashmap<int, uint8_t*> tlss;
	Core::Mutex                  mutex;
};

struct DynamicInfo
{
	void*    hash_table      = nullptr;
	uint64_t hash_table_size = 0;

	char*    str_table      = nullptr;
	uint64_t str_table_size = 0;

	Elf64_Sym* symbol_table            = nullptr;
	uint64_t   symbol_table_total_size = 0;
	uint64_t   symbol_table_entry_size = 0;

	uint64_t init_vaddr          = 0;
	uint64_t fini_vaddr          = 0;
	uint64_t init_array_vaddr    = 0;
	uint64_t fini_array_vaddr    = 0;
	uint64_t preinit_array_vaddr = 0;
	uint64_t init_array_size     = 0;
	uint64_t fini_array_size     = 0;
	uint64_t preinit_array_size  = 0;
	uint64_t pltgot_vaddr        = 0;

	Elf64_Rela* jmprela_table      = nullptr;
	uint64_t    jmprela_table_size = 0;

	Elf64_Rela* rela_table            = nullptr;
	uint64_t    rela_table_total_size = 0;
	uint64_t    rela_table_entry_size = 0;

	uint64_t relative_count = 0;

	uint64_t debug   = 0;
	uint64_t textrel = 0;
	uint64_t flags   = 0;

	const char* so_name = nullptr;

	Vector<const char*> needed;
	Vector<ModuleId>    export_modules;
	Vector<ModuleId>    import_modules;
	Vector<LibraryId>   export_libs;
	Vector<LibraryId>   import_libs;
};

struct Program
{
	int32_t                          unique_id = -1;
	RuntimeLinker*                   rt        = nullptr;
	String                           file_name;
	Elf64*                           elf               = nullptr;
	VirtualMemory::ExceptionHandler* exception_handler = nullptr;
	DynamicInfo*                     dynamic_info      = nullptr;
	uint64_t                         base_vaddr        = 0;
	uint64_t                         base_size         = 0;
	uint64_t                         base_size_aligned = 0;
	SymbolDatabase*                  export_symbols    = nullptr;
	SymbolDatabase*                  import_symbols    = nullptr;
	ThreadLocalStorage               tls;
	bool                             fail_if_global_not_resolved = true;
	bool                             dbg_print_reloc             = false;
	uint64_t                         proc_param_vaddr            = 0;
	uint64_t                         custom_call_plt_vaddr       = 0;
	uint32_t                         custom_call_plt_num         = 0;
};

class RuntimeLinker
{
public:
	RuntimeLinker();
	virtual ~RuntimeLinker();
	void Clear();

	KYTY_CLASS_NO_COPY(RuntimeLinker);

	void DbgDump(const String& folder);

	Program* LoadProgram(const String& elf_name);
	void     SaveMainProgram(const String& elf_name);
	void     SaveProgram(Program* program, const String& elf_name);
	void     UnloadProgram(Program* program);

	[[nodiscard]] uint64_t GetEntry();
	[[nodiscard]] uint64_t GetProcParam();

	void RelocateAll();

	void Execute();
	int  StartModule(Program* program, size_t args, const void* argp, module_func_t func);
	int  StopModule(Program* program, size_t args, const void* argp, module_func_t func);
	void StartAllModules();
	void StopAllModules();
	void DeleteTlss(int thread_id);

	void Resolve(const String& name, SymbolType type, Program* program, SymbolRecord* out_info, bool* bind_self);

	SymbolDatabase* Symbols() { return m_symbols; }

	static uint64_t ReadFromElf(Program* program, uint64_t vaddr);
	Program*        FindProgramByAddr(uint64_t vaddr);
	Program*        FindProgramById(int32_t id);

	static uint8_t* TlsGetAddr(Program* program);
	static void     DeleteTls(Program* program, int thread_id);

	void StackTrace(uint64_t frame_ptr);

private:
	static void LoadProgramToMemory(Program* program);
	static void ParseProgramDynamicInfo(Program* program);
	static void CreateSymbolDatabase(Program* program);
	static void Relocate(Program* program);
	static void DeleteProgram(Program* program);
	static void SetupTlsHandler(Program* program);

	Program* FindProgram(const ModuleId& m, const LibraryId& l);

	static const ModuleId*  FindModule(const Program& program, const String& id);
	static const LibraryId* FindLibrary(const Program& program, const String& id);

	Vector<Program*> m_programs;
	SymbolDatabase*  m_symbols   = nullptr;
	bool             m_relocated = false;
	Core::Mutex      m_mutex;
};

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_RUNTIMELINKER_H_ */
