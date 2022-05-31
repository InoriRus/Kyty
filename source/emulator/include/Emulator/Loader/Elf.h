#ifndef EMULATOR_INCLUDE_EMULATOR_LOADER_ELF_H_
#define EMULATOR_INCLUDE_EMULATOR_LOADER_ELF_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Common.h"

namespace Kyty::Core {
class File;
} // namespace Kyty::Core

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader {

using Elf64_Addr   = uint64_t; // Unsigned program address
using Elf64_Off    = uint64_t; // Unsigned file offset
using Elf64_Half   = uint16_t; // Unsigned medium integer
using Elf64_Word   = uint32_t; // Unsigned integer
using Elf64_Sword  = int32_t;  // Signed integer
using Elf64_Xword  = uint64_t; // Unsigned long integer
using Elf64_Sxword = int64_t;  // Signed long integer

constexpr int EI_MAG0       = 0;
constexpr int EI_MAG1       = 1;
constexpr int EI_MAG2       = 2;
constexpr int EI_MAG3       = 3;
constexpr int EI_CLASS      = 4;
constexpr int EI_DATA       = 5;
constexpr int EI_VERSION    = 6;
constexpr int EI_OSABI      = 7;
constexpr int EI_ABIVERSION = 8;
constexpr int EI_PAD        = 9;
constexpr int EI_NIDENT     = 16;

constexpr char ELFCLASS64 = 2; // 64-bit objects

constexpr char ELFDATA2LSB = 1; // Object file data structures are little-endian

constexpr Elf64_Half EM_X86_64 = 62; /* AMD x86-64 architecture */

constexpr int EV_CURRENT = 1;

constexpr char ELFOSABI_FREEBSD = 9; // FreeBSD operating system

constexpr Elf64_Half ET_DYNEXEC = 0xfe10; // Executable file
constexpr Elf64_Half ET_DYNAMIC = 0xfe18; // Shared

//#define SHT_PROGBITS 1

constexpr Elf64_Word PT_LOAD          = 1;
constexpr Elf64_Word PT_DYNAMIC       = 2;
constexpr Elf64_Word PT_TLS           = 7;
constexpr Elf64_Word PT_OS_DYNLIBDATA = 0x61000000;
constexpr Elf64_Word PT_OS_PROCPARAM  = 0x61000001;
constexpr Elf64_Word PT_OS_RELRO      = 0x61000010;

constexpr Elf64_Word PF_X = 0x1; // Execute
constexpr Elf64_Word PF_W = 0x2; // Write
constexpr Elf64_Word PF_R = 0x4; // Read

constexpr Elf64_Sxword DT_DEBUG                  = 0x00000015;
constexpr Elf64_Sxword DT_FINI                   = 0x0000000d;
constexpr Elf64_Sxword DT_FINI_ARRAY             = 0x0000001a;
constexpr Elf64_Sxword DT_FINI_ARRAYSZ           = 0x0000001c;
constexpr Elf64_Sxword DT_FLAGS                  = 0x0000001e;
constexpr Elf64_Sxword DT_INIT                   = 0x0000000c;
constexpr Elf64_Sxword DT_INIT_ARRAY             = 0x00000019;
constexpr Elf64_Sxword DT_INIT_ARRAYSZ           = 0x0000001b;
constexpr Elf64_Sxword DT_NEEDED                 = 0x00000001;
constexpr Elf64_Sxword DT_OS_EXPORT_LIB          = 0x61000013;
constexpr Elf64_Sxword DT_OS_EXPORT_LIB_1        = 0x61000047;
constexpr Elf64_Sxword DT_OS_EXPORT_LIB_ATTR     = 0x61000017;
constexpr Elf64_Sxword DT_OS_FINGERPRINT         = 0x61000007;
constexpr Elf64_Sxword DT_OS_HASH                = 0x61000025;
constexpr Elf64_Sxword DT_OS_HASHSZ              = 0x6100003d;
constexpr Elf64_Sxword DT_OS_IMPORT_LIB          = 0x61000015;
constexpr Elf64_Sxword DT_OS_IMPORT_LIB_1        = 0x61000049;
constexpr Elf64_Sxword DT_OS_IMPORT_LIB_ATTR     = 0x61000019;
constexpr Elf64_Sxword DT_OS_JMPREL              = 0x61000029;
constexpr Elf64_Sxword DT_OS_MODULE_ATTR         = 0x61000011;
constexpr Elf64_Sxword DT_OS_MODULE_INFO         = 0x6100000d;
constexpr Elf64_Sxword DT_OS_MODULE_INFO_1       = 0x61000043;
constexpr Elf64_Sxword DT_OS_NEEDED_MODULE       = 0x6100000f;
constexpr Elf64_Sxword DT_OS_NEEDED_MODULE_1     = 0x61000045;
constexpr Elf64_Sxword DT_OS_ORIGINAL_FILENAME   = 0x61000009;
constexpr Elf64_Sxword DT_OS_ORIGINAL_FILENAME_1 = 0x61000041;
constexpr Elf64_Sxword DT_OS_PLTGOT              = 0x61000027;
constexpr Elf64_Sxword DT_OS_PLTREL              = 0x6100002b;
constexpr Elf64_Sxword DT_OS_PLTRELSZ            = 0x6100002d;
constexpr Elf64_Sxword DT_OS_RELA                = 0x6100002f;
constexpr Elf64_Sxword DT_OS_RELAENT             = 0x61000033;
constexpr Elf64_Sxword DT_OS_RELASZ              = 0x61000031;
constexpr Elf64_Sxword DT_OS_STRSZ               = 0x61000037;
constexpr Elf64_Sxword DT_OS_STRTAB              = 0x61000035;
constexpr Elf64_Sxword DT_OS_SYMENT              = 0x6100003b;
constexpr Elf64_Sxword DT_OS_SYMTAB              = 0x61000039;
constexpr Elf64_Sxword DT_OS_SYMTABSZ            = 0x6100003f;
constexpr Elf64_Sxword DT_PREINIT_ARRAY          = 0x00000020;
constexpr Elf64_Sxword DT_PREINIT_ARRAYSZ        = 0x00000021;
constexpr Elf64_Sxword DT_REL                    = 0x00000011;
constexpr Elf64_Sxword DT_RELA                   = 0x00000007;
constexpr Elf64_Sxword DT_SONAME                 = 0x0000000e;
constexpr Elf64_Sxword DT_TEXTREL                = 0x00000016;

constexpr Elf64_Sxword DT_HASH      = 0x00000004;
constexpr Elf64_Sxword DT_STRTAB    = 0x00000005;
constexpr Elf64_Sxword DT_STRSZ     = 0x0000000a;
constexpr Elf64_Sxword DT_SYMTAB    = 0x00000006;
constexpr Elf64_Sxword DT_SYMENT    = 0x0000000b;
constexpr Elf64_Sxword DT_PLTGOT    = 0x00000003;
constexpr Elf64_Sxword DT_PLTREL    = 0x00000014;
constexpr Elf64_Sxword DT_JMPREL    = 0x00000017;
constexpr Elf64_Sxword DT_PLTRELSZ  = 0x00000002;
constexpr Elf64_Sxword DT_RELASZ    = 0x00000008;
constexpr Elf64_Sxword DT_RELAENT   = 0x00000009;
constexpr Elf64_Sxword DT_RELACOUNT = 0x6ffffff9;

constexpr Elf64_Sxword DT_NULL = 0;

constexpr Elf64_Word R_X86_64_64        = 1;
constexpr Elf64_Word R_X86_64_GLOB_DAT  = 6;
constexpr Elf64_Word R_X86_64_JUMP_SLOT = 7;
constexpr Elf64_Word R_X86_64_RELATIVE  = 8;
constexpr Elf64_Word R_X86_64_DTPMOD64  = 16;

constexpr uint8_t STB_LOCAL  = 0;
constexpr uint8_t STB_GLOBAL = 1;
constexpr uint8_t STB_WEAK   = 2;

constexpr uint8_t STT_NOTYPE = 0;
constexpr uint8_t STT_OBJECT = 1;
constexpr uint8_t STT_FUNC   = 2;

#pragma pack(1)

struct SelfHeader
{
	uint8_t  ident[12];
	uint16_t size1;
	uint16_t size2;
	uint64_t file_size;
	uint16_t segments_num;
	uint16_t unknown;
	uint32_t pad;
};

struct SelfSegment
{
	uint64_t type;
	uint64_t offset;
	uint64_t compressed_size;
	uint64_t decompressed_size;
};

struct Elf64_Ehdr // NOLINT(readability-identifier-naming)
{
	uint8_t    e_ident[EI_NIDENT]; /* ELF identification */
	Elf64_Half e_type;             /* Object file type */
	Elf64_Half e_machine;          /* Machine type */
	Elf64_Word e_version;          /* Object file version */
	Elf64_Addr e_entry;            /* Entry point address */
	Elf64_Off  e_phoff;            /* Program header offset */
	Elf64_Off  e_shoff;            /* Section header offset */
	Elf64_Word e_flags;            /* Processor-specific flags */
	Elf64_Half e_ehsize;           /* ELF header size */
	Elf64_Half e_phentsize;        /* Size of program header entry */
	Elf64_Half e_phnum;            /* Number of program header entries */
	Elf64_Half e_shentsize;        /* Size of section header entry */
	Elf64_Half e_shnum;            /* Number of section header entries */
	Elf64_Half e_shstrndx;         /* Section name string table index */
};

struct Elf64_Phdr // NOLINT(readability-identifier-naming)
{
	Elf64_Word  p_type;   /* Type of segment */
	Elf64_Word  p_flags;  /* Segment attributes */
	Elf64_Off   p_offset; /* Offset in file */
	Elf64_Addr  p_vaddr;  /* Virtual address in memory */
	Elf64_Addr  p_paddr;  /* Reserved */
	Elf64_Xword p_filesz; /* Size of segment in file */
	Elf64_Xword p_memsz;  /* Size of segment in memory */
	Elf64_Xword p_align;  /* Alignment of segment */
};

struct Elf64_Shdr // NOLINT(readability-identifier-naming)
{
	Elf64_Word  sh_name;      /* Section name */
	Elf64_Word  sh_type;      /* Section type */
	Elf64_Xword sh_flags;     /* Section attributes */
	Elf64_Addr  sh_addr;      /* Virtual address in memory */
	Elf64_Off   sh_offset;    /* Offset in file */
	Elf64_Xword sh_size;      /* Size of section */
	Elf64_Word  sh_link;      /* Link to other section */
	Elf64_Word  sh_info;      /* Miscellaneous information */
	Elf64_Xword sh_addralign; /* Address alignment boundary */
	Elf64_Xword sh_entsize;   /* Size of entries, if section has table */
};

struct Elf64_Dyn // NOLINT(readability-identifier-naming)
{
	Elf64_Sxword d_tag;
	union
	{
		Elf64_Xword d_val;
		Elf64_Addr  d_ptr;
	} d_un;
};

struct Elf64_Sym // NOLINT(readability-identifier-naming)
{
	[[nodiscard]] unsigned char GetBind() const { return st_info >> 4u; }
	[[nodiscard]] unsigned char GetType() const { return st_info & 0xfu; }

	Elf64_Word    st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf64_Half    st_shndx;
	Elf64_Addr    st_value;
	Elf64_Xword   st_size;
};

// struct Elf64_Rel // NOLINT(readability-identifier-naming)
//{
//	Elf64_Addr  r_offset;
//	Elf64_Xword r_info;
//};

struct Elf64_Rela // NOLINT(readability-identifier-naming)
{
	[[nodiscard]] Elf64_Word GetSymbol() const { return static_cast<Elf64_Word>(r_info >> 32u); }
	[[nodiscard]] Elf64_Word GetType() const { return static_cast<Elf64_Word>(r_info & 0xffffffff); }

	Elf64_Addr   r_offset;
	Elf64_Xword  r_info;
	Elf64_Sxword r_addend;
};

struct tls_info_t // NOLINT(readability-identifier-naming)
{
	uint64_t addr;
	uint64_t filesz;
	uint64_t memsz;
};

#pragma pack()

class Elf64
{
public:
	Elf64() = default;
	virtual ~Elf64();

	void Open(const String& file_name);
	void Save(const String& file_name);

	void DbgDump(const String& folder);

	const char* GetSectionName(int index) { return m_str_table + m_shdr[index].sh_name; }

	[[nodiscard]] bool IsSelf() const;
	[[nodiscard]] bool IsValid() const;
	[[nodiscard]] bool IsShared() const;
	[[nodiscard]] bool IsNextGen() const;

	void LoadSegment(uint64_t vaddr, uint64_t file_offset, uint64_t size);

	uint64_t GetEntry();

	[[nodiscard]] const Elf64_Dyn*         GetDynValue(Elf64_Sxword tag) const;
	[[nodiscard]] Vector<const Elf64_Dyn*> GetDynList(Elf64_Sxword tag) const;
	[[nodiscard]] bool                     HasDynValue(Elf64_Sxword tag) const { return GetDynValue(tag) != nullptr; }

	KYTY_CLASS_NO_COPY(Elf64);

	[[nodiscard]] const Elf64_Dyn*  GetDynamic() const { return static_cast<Elf64_Dyn*>(m_dynamic); }
	[[nodiscard]] const Elf64_Ehdr* GetEhdr() const { return m_ehdr; }
	[[nodiscard]] const Elf64_Phdr* GetPhdr() const { return m_phdr; }
	[[nodiscard]] const Elf64_Shdr* GetShdr() const { return m_shdr; }
	[[nodiscard]] char*             GetStrTable() const { return m_str_table; }

	template <class T>
	[[nodiscard]] T GetDynamicData(uint64_t offset) const
	{
		return (m_dynamic_data == nullptr ? nullptr : reinterpret_cast<T>(static_cast<uint8_t*>(m_dynamic_data) + offset));
	}

private:
	void Clear();

	Core::File*  m_f             = nullptr;
	SelfHeader*  m_self          = nullptr;
	SelfSegment* m_self_segments = nullptr;
	Elf64_Ehdr*  m_ehdr          = nullptr;
	Elf64_Phdr*  m_phdr          = nullptr;
	Elf64_Shdr*  m_shdr          = nullptr;
	void*        m_dynamic       = nullptr;
	void*        m_dynamic_data  = nullptr;
	char*        m_str_table     = nullptr;
	// uint64_t    m_base_vaddr   = 0;
};

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED

#endif /* EMULATOR_INCLUDE_EMULATOR_LOADER_ELF_H_ */
