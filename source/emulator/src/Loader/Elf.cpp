#include "Emulator/Loader/Elf.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Loader {

static SelfHeader* load_self(Core::File& f)
{
	if (f.Remaining() < sizeof(SelfHeader))
	{
		return nullptr;
	}

	auto* self = new SelfHeader;

	f.Read(self, sizeof(SelfHeader));

	return self;
}

static SelfSegment* load_self_segments(Core::File& f, uint16_t num)
{
	auto* segs = new SelfSegment[num];

	f.Read(segs, sizeof(SelfSegment) * num);

	return segs;
}

static Elf64_Ehdr* load_ehdr_64(Core::File& f)
{
	if (f.Remaining() < sizeof(Elf64_Ehdr))
	{
		return nullptr;
	}

	auto* ehdr = new Elf64_Ehdr;

	f.Read(ehdr, sizeof(Elf64_Ehdr));

	return ehdr;
}

static void save_ehdr_64(Core::File& f, const Elf64_Ehdr* ehdr)
{
	EXIT_IF(ehdr == nullptr);

	uint32_t bytes_written = 0;

	f.Write(ehdr, sizeof(Elf64_Ehdr), &bytes_written);

	EXIT_IF(bytes_written == 0);
}

static Elf64_Phdr* load_phdr_64(Core::File& f, uint64_t offset, Elf64_Half num)
{
	auto* phdr = new Elf64_Phdr[num];

	f.Seek(offset);
	f.Read(phdr, sizeof(Elf64_Phdr) * num);

	return phdr;
}

static void save_phdr_64(Core::File& f, uint64_t offset, Elf64_Half num, const Elf64_Phdr* phdr)
{
	EXIT_IF(phdr == nullptr);

	uint32_t bytes_written = 0;

	f.Seek(offset);
	f.Write(phdr, sizeof(Elf64_Phdr) * num, &bytes_written);

	EXIT_IF(bytes_written == 0);
}

static Elf64_Shdr* load_shdr_64(Core::File& f, uint64_t offset, Elf64_Half num)
{
	if (num == 0)
	{
		return nullptr;
	}

	auto* shdr = new Elf64_Shdr[num];

	f.Seek(offset);
	f.Read(shdr, sizeof(Elf64_Shdr) * num);

	return shdr;
}

static void save_shdr_64(Core::File& f, uint64_t offset, Elf64_Half num, const Elf64_Shdr* shdr)
{
	if (num == 0)
	{
		return;
	}

	EXIT_IF(shdr == nullptr);

	uint32_t bytes_written = 0;

	f.Seek(offset);
	f.Write(shdr, sizeof(Elf64_Shdr) * num, &bytes_written);

	EXIT_IF(bytes_written == 0);
}

static void* load_dynamic_64(Elf64* f, uint64_t offset, uint64_t size)
{
	void* dynamic_data = new uint8_t[size];

	// f.Seek(offset);
	// f.Read(dynamic_data, size);

	f->LoadSegment(reinterpret_cast<uint64_t>(dynamic_data), offset, size);

	return dynamic_data;
}

static char* load_str_table(Core::File& f, uint64_t offset, uint32_t size)
{
	auto* str_table = new char[size];
	f.Seek(offset);
	f.Read(str_table, size);
	return str_table;
}

static void dbg_print_ehdr_64(Elf64_Ehdr* ehdr, Core::File& f)
{
	f.Printf("ehdr->e_ident = ");
	for (auto i: ehdr->e_ident)
	{
		f.Printf("%02x", i);
	}
	f.Printf("\n");

	f.Printf("ehdr->e_type = 0x%04" PRIx16 "\n", ehdr->e_type);
	f.Printf("ehdr->e_machine = 0x%04" PRIx16 "\n", ehdr->e_machine);
	f.Printf("ehdr->e_version = 0x%08" PRIx32 "\n", ehdr->e_version);

	f.Printf("ehdr->e_entry = 0x%016" PRIx64 "\n", ehdr->e_entry);
	f.Printf("ehdr->e_phoff = 0x%016" PRIx64 "\n", ehdr->e_phoff);
	f.Printf("ehdr->e_shoff = 0x%016" PRIx64 "\n", ehdr->e_shoff);
	f.Printf("ehdr->e_flags = 0x%08" PRIx32 "\n", ehdr->e_flags);
	f.Printf("ehdr->e_ehsize = 0x%04" PRIx16 "\n", ehdr->e_ehsize);
	f.Printf("ehdr->e_phentsize = 0x%04" PRIx16 "\n", ehdr->e_phentsize);
	f.Printf("ehdr->e_phnum = %" PRIu16 "\n", ehdr->e_phnum);
	f.Printf("ehdr->e_shentsize = 0x%04" PRIx16 "\n", ehdr->e_shentsize);
	f.Printf("ehdr->e_shnum = %" PRIu16 "\n", ehdr->e_shnum);
	f.Printf("ehdr->e_shstrndx = %" PRIu16 "\n", ehdr->e_shstrndx);
}

static void dbg_print_phdr_64(Elf64_Phdr* phdr, Core::File& f)
{
	f.Printf("phdr->p_type = 0x%08" PRIx32 "\n", phdr->p_type);
	f.Printf("phdr->p_flags = 0x%08" PRIx32 "\n", phdr->p_flags);
	f.Printf("phdr->p_offset = 0x%016" PRIx64 "\n", phdr->p_offset);
	f.Printf("phdr->p_vaddr = 0x%016" PRIx64 "\n", phdr->p_vaddr);
	f.Printf("phdr->p_paddr = 0x%016" PRIx64 "\n", phdr->p_paddr);
	f.Printf("phdr->p_filesz = 0x%016" PRIx64 "\n", phdr->p_filesz);
	f.Printf("phdr->p_memsz = 0x%016" PRIx64 "\n", phdr->p_memsz);
	f.Printf("phdr->p_align = 0x%016" PRIx64 "\n", phdr->p_align);
}

static void dbg_print_shdr_64(Elf64_Shdr* shdr, Core::File& f)
{
	f.Printf("shdr->sh_name = %d\n", shdr->sh_name);
	f.Printf("shdr->sh_type = 0x%08" PRIx32 "\n", shdr->sh_type);
	f.Printf("shdr->sh_flags = 0x%016" PRIx64 "\n", shdr->sh_flags);
	f.Printf("shdr->sh_addr = 0x%016" PRIx64 "\n", shdr->sh_addr);
	f.Printf("shdr->sh_offset = 0x%016" PRIx64 "\n", shdr->sh_offset);
	f.Printf("shdr->sh_size = 0x%016" PRIx64 "\n", shdr->sh_size);
	f.Printf("shdr->sh_link = %" PRId32 "\n", shdr->sh_link);
	f.Printf("shdr->sh_info = 0x%08" PRIx32 "\n", shdr->sh_info);
	f.Printf("shdr->sh_addralign = 0x%016" PRIx64 "\n", shdr->sh_addralign);
	f.Printf("shdr->sh_entsize = 0x%016" PRIx64 "\n", shdr->sh_entsize);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DBG_NAME(tag)                                                                                                                      \
	case tag: name = #tag; break;

static void dbg_print_dynamic_64(const Elf64_Dyn* dyn, Core::File& f)
{
	const char* name = "Unknown";
	switch (dyn->d_tag)
	{
		DBG_NAME(DT_OS_HASH)
		DBG_NAME(DT_HASH)
		DBG_NAME(DT_OS_STRTAB)
		DBG_NAME(DT_OS_STRSZ)
		DBG_NAME(DT_STRTAB)
		DBG_NAME(DT_STRSZ)
		DBG_NAME(DT_OS_SYMTAB)
		DBG_NAME(DT_SYMTAB)
		DBG_NAME(DT_OS_HASHSZ)
		DBG_NAME(DT_OS_SYMTABSZ)
		DBG_NAME(DT_INIT)
		DBG_NAME(DT_FINI)
		DBG_NAME(DT_OS_PLTGOT)
		DBG_NAME(DT_PLTGOT)
		DBG_NAME(DT_OS_JMPREL)
		DBG_NAME(DT_JMPREL)
		DBG_NAME(DT_OS_PLTRELSZ)
		DBG_NAME(DT_PLTRELSZ)
		DBG_NAME(DT_OS_PLTREL)
		DBG_NAME(DT_PLTREL)
		DBG_NAME(DT_OS_RELA)
		DBG_NAME(DT_RELA)
		DBG_NAME(DT_OS_RELASZ)
		DBG_NAME(DT_RELASZ)
		DBG_NAME(DT_OS_RELAENT)
		DBG_NAME(DT_RELAENT)
		DBG_NAME(DT_INIT_ARRAY)
		DBG_NAME(DT_INIT_ARRAYSZ)
		DBG_NAME(DT_FINI_ARRAY)
		DBG_NAME(DT_FINI_ARRAYSZ)
		DBG_NAME(DT_PREINIT_ARRAY)
		DBG_NAME(DT_PREINIT_ARRAYSZ)
		DBG_NAME(DT_OS_SYMENT)
		DBG_NAME(DT_SYMENT)
		DBG_NAME(DT_DEBUG)
		DBG_NAME(DT_TEXTREL)
		DBG_NAME(DT_FLAGS)
		DBG_NAME(DT_NEEDED)
		DBG_NAME(DT_OS_NEEDED_MODULE)
		DBG_NAME(DT_OS_NEEDED_MODULE_1)
		DBG_NAME(DT_OS_IMPORT_LIB)
		DBG_NAME(DT_OS_IMPORT_LIB_1)
		DBG_NAME(DT_OS_IMPORT_LIB_ATTR)
		DBG_NAME(DT_OS_FINGERPRINT)
		DBG_NAME(DT_OS_ORIGINAL_FILENAME)
		DBG_NAME(DT_OS_ORIGINAL_FILENAME_1)
		DBG_NAME(DT_OS_MODULE_INFO)
		DBG_NAME(DT_OS_MODULE_INFO_1)
		DBG_NAME(DT_OS_MODULE_ATTR)
		DBG_NAME(DT_SONAME)
		DBG_NAME(DT_OS_EXPORT_LIB)
		DBG_NAME(DT_OS_EXPORT_LIB_1)
		DBG_NAME(DT_OS_EXPORT_LIB_ATTR)
		DBG_NAME(DT_RELACOUNT)
		DBG_NAME(DT_NULL)
	}
	f.Printf("d_tag = 0x%016" PRIx64 ", d_val = 0x%016" PRIx64 ", name = %s\n", dyn->d_tag, dyn->d_un.d_val, name);
}

Elf64::~Elf64()
{
	Clear();
}

void Elf64::LoadSegment(uint64_t vaddr, uint64_t file_offset, uint64_t size)
{
	EXIT_IF(m_f == nullptr);

	if (m_self != nullptr)
	{
		EXIT_IF(m_self_segments == nullptr);
		EXIT_IF(m_phdr == nullptr);

		for (uint16_t i = 0; i < m_self->segments_num; i++)
		{
			const auto& seg = m_self_segments[i];
			if ((seg.type & 0x800u) != 0)
			{
				auto phdr_id = ((seg.type >> 20u) & 0xFFFu);

				const auto& phdr = m_phdr[phdr_id];

				if (file_offset >= phdr.p_offset && file_offset < phdr.p_offset + phdr.p_filesz)
				{
					EXIT_NOT_IMPLEMENTED(seg.decompressed_size != phdr.p_filesz);
					EXIT_NOT_IMPLEMENTED(seg.compressed_size != seg.decompressed_size);

					auto offset = file_offset - phdr.p_offset;

					EXIT_NOT_IMPLEMENTED(offset + size > seg.decompressed_size);

					m_f->Seek(offset + seg.offset);
					m_f->Read(reinterpret_cast<void*>(static_cast<uintptr_t>(vaddr)), size);

					return;
				}
			}
		}

		if (m_f->Size() - m_self->file_size == size)
		{
			m_f->Seek(m_self->file_size);
			m_f->Read(reinterpret_cast<void*>(static_cast<uintptr_t>(vaddr)), size);

			return;
		}

		EXIT("missing self segment\n");
	} else
	{
		m_f->Seek(file_offset);
		m_f->Read(reinterpret_cast<void*>(static_cast<uintptr_t>(vaddr)), size);
	}
}

const Elf64_Dyn* Elf64::GetDynValue(Elf64_Sxword tag) const
{
	for (const auto* dyn = GetDynamic(); dyn->d_tag != DT_NULL; dyn++)
	{
		if (dyn->d_tag == tag)
		{
			return dyn;
		}
	}
	return nullptr;
}

Vector<const Elf64_Dyn*> Elf64::GetDynList(Elf64_Sxword tag) const
{
	Vector<const Elf64_Dyn*> ret;
	for (const auto* dyn = GetDynamic(); dyn->d_tag != DT_NULL; dyn++)
	{
		if (dyn->d_tag == tag)
		{
			ret.Add(dyn);
		}
	}
	return ret;
}

bool Elf64::IsShared() const
{
	return (m_ehdr->e_type == ET_DYNAMIC);
}

bool Elf64::IsNextGen() const
{
	return (m_ehdr->e_ident[EI_ABIVERSION] == 2);
}

void Elf64::Clear()
{
	if (m_f != nullptr)
	{
		m_f->Close();
		delete m_f;
	}
	delete m_self;
	delete m_ehdr;
	delete[] m_self_segments;
	delete[] m_phdr;
	delete[] m_shdr;
	delete[] m_str_table;
	delete[] static_cast<uint8_t*>(m_dynamic);
	delete[] static_cast<uint8_t*>(m_dynamic_data);

	m_self          = nullptr;
	m_self_segments = nullptr;
	m_ehdr          = nullptr;
	m_phdr          = nullptr;
	m_shdr          = nullptr;
	m_str_table     = nullptr;
	m_dynamic       = nullptr;
	m_dynamic_data  = nullptr;
}

void Elf64::DbgDump(const String& folder)
{
	auto folder_str = folder.FixDirectorySlash();

	Core::File::CreateDirectories(folder_str);

	for (uint16_t i = 0; i < m_ehdr->e_phnum; i++)
	{
		if (m_phdr[i].p_filesz == 0u)
		{
			continue;
		}

		char str[512];
		int  s = snprintf(str, 512, "phdr_%03d", i);
		EXIT_NOT_IMPLEMENTED(s >= 512);

		Core::File fout;
		fout.Create(folder_str + str);

		auto* buf = new char[static_cast<uint32_t>(m_phdr[i].p_filesz)];

		// m_f->Seek(m_phdr[i].p_offset);
		// m_f->Read(buf, static_cast<uint32_t>(m_phdr[i].p_filesz));

		LoadSegment(reinterpret_cast<uint64_t>(buf), m_phdr[i].p_offset, m_phdr[i].p_filesz);

		fout.Write(buf, static_cast<uint32_t>(m_phdr[i].p_filesz));

		delete[] buf;

		fout.Close();
	}

	for (uint16_t i = 0; i < m_ehdr->e_shnum; i++)
	{
		if (m_shdr[i].sh_size == 0u)
		{
			continue;
		}

		char str[512];
		int  s = snprintf(str, 512, "shdr_%03d", i);
		EXIT_NOT_IMPLEMENTED(s >= 512);

		Core::File fout;
		fout.Create(folder_str + str);

		auto* buf = new char[static_cast<uint32_t>(m_shdr[i].sh_size)];

		m_f->Seek(m_shdr[i].sh_offset);
		m_f->Read(buf, static_cast<uint32_t>(m_shdr[i].sh_size));
		fout.Write(buf, static_cast<uint32_t>(m_shdr[i].sh_size));

		delete[] buf;

		fout.Close();
	}

	Core::File fout;

	fout.Create(folder_str + U"ehdr.txt");
	dbg_print_ehdr_64(m_ehdr, fout);
	fout.Close();

	fout.Create(folder_str + U"phdr.txt");
	for (uint16_t i = 0; i < m_ehdr->e_phnum; i++)
	{
		fout.Printf("--- phdr [%d] ---\n", i);
		dbg_print_phdr_64(m_phdr + i, fout);
	}
	fout.Close();

	fout.Create(folder_str + U"shdr.txt");
	for (uint16_t i = 0; i < m_ehdr->e_shnum; i++)
	{
		fout.Printf("--- shdr [%d] %s ---\n", i, GetSectionName(i));
		dbg_print_shdr_64(m_shdr + i, fout);
	}
	fout.Close();

	fout.Create(folder_str + U"dynamic.txt");
	for (const auto* dyn = GetDynamic(); dyn->d_tag != DT_NULL; dyn++)
	{
		dbg_print_dynamic_64(dyn, fout);
	}
	fout.Close();
}

uint64_t Elf64::GetEntry()
{
	return m_ehdr->e_entry;
}

bool Elf64::IsSelf() const
{
	if (m_f == nullptr || m_f->IsInvalid())
	{
		return false;
	}

	if (m_self == nullptr)
	{
		return false;
	}

	if (m_self->ident[0] != 0x4f || m_self->ident[1] != 0x15 || m_self->ident[2] != 0x3d || m_self->ident[3] != 0x1d)
	{
		return false;
	}

	if (m_self->ident[4] != 0x00 || m_self->ident[5] != 0x01 || m_self->ident[6] != 0x01 || m_self->ident[7] != 0x12)
	{
		printf("Unknown SELF file\n");
		return false;
	}

	if (m_self->ident[8] != 0x01 || m_self->ident[9] != 0x01 || m_self->ident[10] != 0x00 || m_self->ident[11] != 0x00)
	{
		printf("Unknown SELF file\n");
		return false;
	}

	if (m_self->unknown != 0x22)
	{
		printf("Unknown SELF file\n");
		return false;
	}

	return true;
}

bool Elf64::IsValid() const
{
	if (m_f == nullptr || m_f->IsInvalid())
	{
		return false;
	}

	if (m_ehdr == nullptr)
	{
		return false;
	}

	if (m_ehdr->e_ident[EI_MAG0] != '\x7f' || m_ehdr->e_ident[EI_MAG1] != 'E' || m_ehdr->e_ident[EI_MAG2] != 'L' ||
	    m_ehdr->e_ident[EI_MAG3] != 'F')
	{
		printf("Not an ELF file\n");
		return false;
	}

	if (m_ehdr->e_ident[EI_CLASS] != ELFCLASS64)
	{
		printf("ehdr->e_ident[EI_CLASS] (0x%x) != ELFCLASS64\n", m_ehdr->e_ident[EI_CLASS]);
		return false;
	}

	if (m_ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		printf("ehdr->e_ident[EI_DATA] (0x%x) != ELFDATA2LSB\n", m_ehdr->e_ident[EI_DATA]);
		return false;
	}

	if (m_ehdr->e_ident[EI_VERSION] != EV_CURRENT)
	{
		printf("ehdr->e_ident[EI_VERSION] != EV_CURRENT\n");
		return false;
	}

	if (m_ehdr->e_ident[EI_OSABI] != ELFOSABI_FREEBSD)
	{
		printf("ehdr->e_ident[EI_OSABI] (0x%x) != ELFOSABI_FREEBSD\n", m_ehdr->e_ident[EI_OSABI]);
		return false;
	}

	if (m_ehdr->e_ident[EI_ABIVERSION] != 0 && m_ehdr->e_ident[EI_ABIVERSION] != 2)
	{
		printf("ehdr->e_ident[EI_ABIVERSION] (0x%x) != (0 or 2)\n", m_ehdr->e_ident[EI_ABIVERSION]);
		return false;
	}

	if (m_ehdr->e_type != ET_DYNEXEC && m_ehdr->e_type != ET_DYNAMIC)
	{
		printf("ehdr->e_type (%04x) != ET_DYNEXEC && m_ehdr->e_type != ET_DYNAMIC\n", m_ehdr->e_type);
		return false;
	}

	if (m_ehdr->e_machine != EM_X86_64)
	{
		printf("ehdr->e_machine (%04x) != EM_X86_64\n", m_ehdr->e_machine);
		return false;
	}

	if (m_ehdr->e_version != EV_CURRENT)
	{
		printf("ehdr->e_version != EV_CURRENT\n");
		return false;
	}

	if (m_ehdr->e_phentsize != sizeof(Elf64_Phdr))
	{
		printf("ehdr->e_phentsize != sizeof(Elf64_Phdr)\n");
		return false;
	}

	if (m_ehdr->e_shentsize > 0 && m_ehdr->e_shentsize != sizeof(Elf64_Shdr))
	{
		printf("ehdr->e_shentsize (%d) != sizeof(Elf64_Shdr)\n", m_ehdr->e_shentsize);
		return false;
	}

	return true;
}

void Elf64::Open(const String& file_name)
{
	Clear();

	m_f = new Core::File;
	m_f->Open(file_name, Core::File::Mode::Read);

	if (m_f->IsInvalid())
	{
		EXIT("Can't open %s\n", file_name.C_Str());
	}

	m_self = load_self(*m_f);

	if (!IsSelf())
	{
		delete m_self;
		m_self = nullptr;
		m_f->Seek(0);
	} else
	{
		m_self_segments = load_self_segments(*m_f, m_self->segments_num);
	}

	auto ehdr_pos = m_f->Tell();

	m_ehdr = load_ehdr_64(*m_f);

	if (!IsValid())
	{
		delete m_ehdr;
		m_ehdr = nullptr;
	}

	if (m_ehdr != nullptr /*&& m_self == nullptr*/)
	{
		m_phdr = load_phdr_64(*m_f, ehdr_pos + m_ehdr->e_phoff, m_ehdr->e_phnum);
		m_shdr = load_shdr_64(*m_f, ehdr_pos + m_ehdr->e_shoff, m_ehdr->e_shnum);

		EXIT_NOT_IMPLEMENTED(m_shdr != nullptr && m_self != nullptr);

		if (m_shdr != nullptr)
		{
			m_str_table =
			    load_str_table(*m_f, m_shdr[m_ehdr->e_shstrndx].sh_offset, static_cast<uint32_t>(m_shdr[m_ehdr->e_shstrndx].sh_size));
		}

		for (Elf64_Half i = 0; i < m_ehdr->e_phnum; i++)
		{
			if (m_phdr[i].p_type == PT_DYNAMIC)
			{
				m_dynamic = load_dynamic_64(this, m_phdr[i].p_offset, m_phdr[i].p_filesz);
			}

			if (m_phdr[i].p_type == PT_OS_DYNLIBDATA)
			{
				m_dynamic_data = load_dynamic_64(this, m_phdr[i].p_offset, m_phdr[i].p_filesz);
			}
		}
	}
}

void Elf64::Save(const String& file_name)
{
	EXIT_IF(!IsValid());

	if (IsValid())
	{
		Core::File f;
		f.Create(file_name);

		if (f.IsInvalid())
		{
			EXIT("Can't create %s\n", file_name.C_Str());
		}

		save_ehdr_64(f, m_ehdr);

		save_phdr_64(f, m_ehdr->e_phoff, m_ehdr->e_phnum, m_phdr);
		save_shdr_64(f, m_ehdr->e_shoff, m_ehdr->e_shnum, m_shdr);

		for (uint16_t i = 0; i < m_ehdr->e_phnum; i++)
		{
			if (m_phdr[i].p_filesz == 0u)
			{
				continue;
			}

			auto* buf = new char[static_cast<uint32_t>(m_phdr[i].p_filesz)];

			LoadSegment(reinterpret_cast<uint64_t>(buf), m_phdr[i].p_offset, m_phdr[i].p_filesz);

			uint32_t bytes_written = 0;

			f.Seek(m_phdr[i].p_offset);
			f.Write(buf, static_cast<uint32_t>(m_phdr[i].p_filesz), &bytes_written);

			EXIT_IF(bytes_written == 0);

			delete[] buf;
		}

		for (uint16_t i = 0; i < m_ehdr->e_shnum; i++)
		{
			if (m_shdr[i].sh_size == 0u)
			{
				continue;
			}

			auto* buf = new char[static_cast<uint32_t>(m_shdr[i].sh_size)];

			m_f->Seek(m_shdr[i].sh_offset);
			m_f->Read(buf, static_cast<uint32_t>(m_shdr[i].sh_size));

			uint32_t bytes_written = 0;

			f.Seek(m_shdr[i].sh_offset);
			f.Write(buf, static_cast<uint32_t>(m_shdr[i].sh_size), &bytes_written);

			EXIT_IF(bytes_written == 0);

			delete[] buf;
		}

		f.Close();
	}
}

} // namespace Kyty::Loader

#endif // KYTY_EMU_ENABLED
