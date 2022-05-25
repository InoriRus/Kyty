#include "Kyty/Core/Debug.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/Sys/SysDbg.h"
#include "Kyty/Sys/SysStdlib.h"

// IWYU pragma: no_include <sec_api/string_s.h>

#if 1 && (KYTY_COMPILER == KYTY_COMPILER_MSVC || KYTY_LINKER == KYTY_LINKER_LLD_LINK)
#define KYTY_UNDECORATE
#endif

#ifdef KYTY_UNDECORATE
// String unDName(const String &mangled,
//		void* (*memget)(size_t), void (*memfree)(void*),
//                       unsigned short int flags);
#endif

namespace Kyty::Core {

//#ifndef KYTY_FINAL
#if KYTY_PROJECT != KYTY_PROJECT_BUILD_TOOLS
#define DEBUG_MAP_ENABLED
#endif

constexpr bool DBG_PRINTF = false;

static DebugMap* g_dbg_map  = nullptr;
static String*   g_exe_name = nullptr;

struct DebugFunctionInfo
{
	uintptr_t    addr;
	uintptr_t    length;
	String::Utf8 name;
	String::Utf8 obj;

	bool operator<(const DebugFunctionInfo& f) const { return addr < f.addr; }
};

struct DebugFunctionInfo2
{
	uintptr_t   addr;
	uintptr_t   length;
	const char* name;
	const char* obj;

	bool operator<(const DebugFunctionInfo2& f) const { return addr < f.addr; }
};

using DebugMapType   = Hashmap<uintptr_t, uint32_t>;
using DebugDataType  = Vector<DebugFunctionInfo>;
using DebugDataType2 = Vector<DebugFunctionInfo2>;

struct DebugMapPrivate
{
	DebugMapPrivate() = default;
	~DebugMapPrivate()
	{
		if (buf != nullptr)
		{
			DeleteArray(buf);
		}
	}

	void FixBaseAddress();

	static const DebugFunctionInfo* FindFunc(DebugMap* map, uintptr_t addr);

	KYTY_CLASS_NO_COPY(DebugMapPrivate)

	DebugDataType  data;
	DebugDataType2 data2;
	DebugMapType   map;
	char*          buf {nullptr};
};

static void exception_filter(void* /*addr*/)
{
	EXIT("Exception!!!");
}

void core_debug_init(const char* app_name)
{
	sys_set_exception_filter(exception_filter);

	g_exe_name = new String(String::FromUtf8(app_name));
	g_dbg_map  = new DebugMap;
	// g_dbg_map->LoadMap();
	g_dbg_map->LoadCsv();
}

DebugMap::DebugMap(): m_p(new DebugMapPrivate) {}

void DebugMap::LoadMap()
{
#ifdef DEBUG_MAP_ENABLED
	printf("exe_name = %s\n", g_exe_name->utf8_str().GetData());

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	String linker = Debug::GetLinker();
	String map_file =
	    g_exe_name->FilenameWithoutExtension() + U"_" + Debug::GetCompiler() + U"_" + linker + U"_" + Debug::GetBitness() + U".map";

	if (linker == U"ld")
	{
		LoadGnuLd(map_file, KYTY_BITNESS);
	} else if (linker == U"link")
	{
		LoadMsvcLink(map_file, KYTY_BITNESS);
	} else if (linker == U"lld_link")
	{
		LoadMsvcLldLink(map_file, KYTY_BITNESS);
	} else if (linker == U"lld")
	{
		LoadLlvmLld(map_file, KYTY_BITNESS);
	} else
	{
		return;
	}

#elif KYTY_PLATFORM == KYTY_PLATFORM_ANDROID

#if KYTY_ABI == KYTY_ABI_ARMEABI
	LoadGnuLd(U"armeabi/fc_android.map", 32);
#elif KYTY_ABI == KYTY_ABI_ARM64_V8A
	LoadGnuLd(U"arm64-v8a/fc_android.map", 64);
#elif KYTY_ABI == KYTY_ABI_ARMEABI_V7A
	LoadGnuLd(U"armeabi-v7a/fc_android.map", 32);
#elif KYTY_ABI == KYTY_ABI_MIPS
	LoadGnuLd(U"mips/fc_android.map", 64);
#elif KYTY_ABI == KYTY_ABI_MIPS64
	LoadGnuLd(U"mips64/fc_android.map", 64);
#elif KYTY_ABI == KYTY_ABI_X86
	LoadGnuLd(U"x86/fc_android.map", 32);
#elif KYTY_ABI == KYTY_ABI_X86_64
	LoadGnuLd(U"x86_64/fc_android.map", 64);
#endif

#endif

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	DumpMap(map_file.FilenameWithoutExtension() + U".csv");

	if (linker == U"lld")
	{
		m_p->FixBaseAddress();
	}

#elif KYTY_PLATFORM == KYTY_PLATFORM_ANDROID
	DumpMap(U"_map.csv");
#endif

	// EXIT("1");

#endif
}

void DebugMap::LoadCsv()
{
#ifdef DEBUG_MAP_ENABLED

	printf("exe_name = %s\n", g_exe_name->utf8_str().GetData());

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	String linker = Debug::GetLinker();
	String map_file =
	    g_exe_name->FilenameWithoutExtension() + U"_" + Debug::GetCompiler() + U"_" + linker + U"_" + Debug::GetBitness() + U".csv";

	LoadCsv(map_file);

	if (linker == U"lld")
	{
		m_p->FixBaseAddress();
	}

#elif KYTY_PLATFORM == KYTY_PLATFORM_ANDROID

#if KYTY_ABI == KYTY_ABI_ARMEABI
	LoadCsv(U"armeabi/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_ARM64_V8A
	LoadCsv(U"arm64-v8a/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_ARMEABI_V7A
	LoadCsv(U"armeabi-v7a/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_MIPS
	LoadCsv(U"mips/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_MIPS64
	LoadCsv(U"mips64/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_X86
	LoadCsv(U"x86/fc_android.csv");
#elif KYTY_ABI == KYTY_ABI_X86_64
	LoadCsv(U"x86_64/fc_android.csv");
#endif

#endif

	// EXIT("1");

#endif
}

DebugMap::~DebugMap()
{
	Delete(m_p);
}

KYTY_ARRAY_DEFINE_SWAP(DebugMapSortSwapFunc, DebugFunctionInfo)
{
	DebugMapType& map  = *static_cast<DebugMapType*>(arg);
	map[array[i].addr] = j;
	map[array[j].addr] = i;

	//	DebugMap::DataType &data = *(DebugMap::DataType*)arg;
	//
	//	printf("swap %d, %d\n", i, j);
	//	for(uint32_t ii = 0; ii < data.Size(); ii++)
	//	{
	//		const DebugMap::FunctionInfo& f = data.At(ii);
	//
	//		printf("\t0x%016I64x;%I64u;%s;%s\n", f.addr, f.length, f.name.utf8_str().GetData(), f.obj.utf8_str().GetData());
	//	}

	DebugFunctionInfo t = array[i];
	array[i]            = array[j];
	array[j]            = t;
}

void DebugMap::LoadMsvcLink(const String& name, int mode)
{
	File pf(name, File::Mode::Read);
	if (pf.IsInvalid())
	{
		return;
	}

	auto* buf = new uint8_t[pf.Size()];
	pf.Read(buf, pf.Size());
	File f;
	f.OpenInMem(buf, pf.Size());
	f.SetEncoding(File::Encoding::Utf8);

	pf.Close();

	for (;;)
	{
		if (f.IsEOF())
		{
			break;
		}

		String s = f.ReadLine();
		s        = s.RemoveChar(U'\n');

		StringList list = s.Split(U" ");

		if (list.Size() >= 5 && list[3] == U"f")
		{
			EXIT_IF(!((list.Size() == 5 && list[3] == U"f") || (list.Size() == 6 && list[3] == U"f" && list[4] == U"i")));

			uintptr_t    addr = (mode == 32 ? list[2].ToUint32(16) : list[2].ToUint64(16));
			const String func = list.At(1);
			const String obj  = list.Size() == 5 ? list[4] : list[5];

			if (DBG_PRINTF)
			{
				printf("%016" PRIx64 "; %s; %s\n", static_cast<uint64_t>(addr), func.utf8_str().GetData(), obj.utf8_str().GetData());
				// int ok = fflush(stdout);
			}

			DebugFunctionInfo inf = {addr, 0, func.utf8_str(), obj.utf8_str()};

			if (m_p->map.Contains(addr))
			{
				String name1(m_p->data.At(m_p->map[addr]).name);
				if (name1.StartsWith(U"_"))
				{
					name1 = name1.Mid(1);
				}
				// EXIT_IF(data.At(map[addr]).name != func_name);
				if (name1 != func && !name1.ContainsStr(func) && !func.ContainsStr(name1))
				{
					if (DBG_PRINTF)
					{
						printf("warning: name1: %s, name2: %s\n", name1.utf8_str().GetData(), func.utf8_str().GetData());
					}
					// exit(1);
				}

				continue;
			}

			//			if (mem_alloc_obj.Contains(inf.obj, String::CASE_INSENSITIVE)
			//				&& func.ContainsAny(mem_alloc_names))
			//			{
			//				inf.is_mem_alloc = true;
			//			}

			m_p->data.Add(inf);
			m_p->map.Put(addr, m_p->data.Size() - 1);

		} else
		{
			EXIT_IF(list.Size() >= 5 && list[0].ContainsStr(U":") && list[3] != U"f");
		}
	}

	f.Close();

	DeleteArray(buf);

	m_p->data.Sort(DebugMapSortSwapFunc, &m_p->map);

	for (uint32_t i = 0; i < m_p->data.Size(); i++)
	{
		if (i > 0)
		{
			m_p->data[i - 1].length = m_p->data[i].addr - m_p->data[i - 1].addr;
		}

		// UNDNAME_COMPLETE
#ifdef KYTY_UNDECORATE
#endif
	}
}

void DebugMap::LoadMsvcLldLink(const String& name, int mode)
{
	File pf(name, File::Mode::Read);
	if (pf.IsInvalid())
	{
		return;
	}

	auto* buf = new uint8_t[pf.Size()];
	pf.Read(buf, pf.Size());
	File f;
	f.OpenInMem(buf, pf.Size());
	f.SetEncoding(File::Encoding::Utf8);

	pf.Close();

	for (;;)
	{
		if (f.IsEOF())
		{
			break;
		}

		String s = f.ReadLine();
		s        = s.RemoveChar(U'\n');

		StringList list = s.Split(U" ");

		if (list.Size() == 4 && list[2].StartsWith(U'0'))
		{
			uintptr_t    addr = (mode == 32 ? list[2].ToUint32(16) : list[2].ToUint64(16));
			const String func = list.At(1);
			const String obj  = list[3];

			if (DBG_PRINTF)
			{
				printf("%016" PRIx64 "; %s; %s\n", static_cast<uint64_t>(addr), func.utf8_str().GetData(), obj.utf8_str().GetData());
				// fflush(stdout);
			}

			DebugFunctionInfo inf = {addr, 0, func.utf8_str(), obj.utf8_str()};

			if (m_p->map.Contains(addr))
			{
				String name1(m_p->data.At(m_p->map[addr]).name);
				if (name1.StartsWith(U"_"))
				{
					name1 = name1.Mid(1);
				}
				// EXIT_IF(data.At(map[addr]).name != func_name);
				if (name1 != func && !name1.ContainsStr(func) && !func.ContainsStr(name1))
				{
					if (DBG_PRINTF)
					{
						printf("warning: name1: %s, name2: %s\n", name1.utf8_str().GetData(), func.utf8_str().GetData());
					}
					// exit(1);
				}

				continue;
			}

			//			if (mem_alloc_obj.Contains(inf.obj, String::CASE_INSENSITIVE)
			//				&& func.ContainsAny(mem_alloc_names))
			//			{
			//				inf.is_mem_alloc = true;
			//			}

			m_p->data.Add(inf);
			m_p->map.Put(addr, m_p->data.Size() - 1);

		} else
		{
			EXIT_IF(list.Size() >= 5 && list[0].ContainsStr(U":") && list[3] != U"f");
		}
	}

	f.Close();

	DeleteArray(buf);

	m_p->data.Sort(DebugMapSortSwapFunc, &m_p->map);

	for (uint32_t i = 0; i < m_p->data.Size(); i++)
	{
		if (i > 0)
		{
			m_p->data[i - 1].length = m_p->data[i].addr - m_p->data[i - 1].addr;
		}

#ifdef KYTY_UNDECORATE
#endif
	}
}

void DebugMap::LoadLlvmLld(const String& name, int bitness)
{
	if (bitness != 64)
	{
		return;
	}

	File pf(name, File::Mode::Read);
	if (pf.IsInvalid())
	{
		return;
	}

	auto* buf = new uint8_t[pf.Size()];
	pf.Read(buf, pf.Size());
	File f;
	f.OpenInMem(buf, pf.Size());
	f.SetEncoding(File::Encoding::Utf8);

	pf.Close();

	String header = f.ReadLine().RemoveChar(U'\n');
	String in;

	if (header == U"Address  Size     Align Out     In      Symbol")
	{
		uint32_t prev_addr = 0;
		for (;;)
		{
			if (f.IsEOF())
			{
				break;
			}

			String   s       = f.ReadLine().RemoveChar(U'\n');
			uint32_t address = 0;

			if (s.At(8) == U' ')
			{
				address = s.Mid(0, 8).ToUint32(16);
			} else
			{
				break;
			}

			if (address != prev_addr)
			{
				if (s.At(24) != U' ')
				{
					if (!s.Mid(24).StartsWith(U".text"))
					{
						break;
					}

					continue;
				}

				if (s.At(32) != U' ')
				{
					in = s.Mid(32);
					in = in.FixFilenameSlash().FilenameWithoutDirectory();
					in = in.Mid(0, in.FindIndex(U':'));
				} else
				{
					String sym = s.Mid(40);

					DebugFunctionInfo inf = {address, 0, sym.utf8_str(), in.utf8_str()};

					m_p->data.Add(inf);
					m_p->map.Put(address, m_p->data.Size() - 1);

					prev_addr = address;
				}
			}
		}
	}

	f.Close();

	DeleteArray(buf);

	m_p->data.Sort(DebugMapSortSwapFunc, &m_p->map);

	for (uint32_t i = 0; i < m_p->data.Size(); i++)
	{
		if (i > 0 && m_p->data[i - 1].length == 0)
		{
			m_p->data[i - 1].length = m_p->data[i].addr - m_p->data[i - 1].addr;
		}
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void DebugMap::LoadGnuLd(const String& name, int bitness)
{
	File pf(name, File::Mode::Read);
	if (pf.IsInvalid())
	{
		return;
	}

	auto* buf = new uint8_t[pf.Size()];
	pf.Read(buf, pf.Size());
	File f;
	f.OpenInMem(buf, pf.Size());
	f.SetEncoding(File::Encoding::Utf8);

	pf.Close();

	int mode = 0;

	String    object_name;
	uintptr_t object_addr     = 0;
	uintptr_t object_size     = 0;
	bool      object_start    = false;
	bool      object_unsorted = false;

	for (;;)
	{
		if (f.IsEOF())
		{
			break;
		}

		String s = f.ReadLine();
		s        = s.RemoveChar(U'\n');

		if (mode == 1)
		{
			if (s.StartsWith(U"                "))
			{
				uintptr_t addr      = bitness == 32 ? s.Mid(16, 10).ToUint32(16) : s.Mid(16, 18).ToUint64(16);
				String    func_name = bitness == 32 ? s.Mid(42) : s.Mid(50);

				if (DBG_PRINTF)
				{
					printf("\t%08" PRIx32 ", %s\n", static_cast<uint32_t>(addr), func_name.utf8_str().GetData());
				}

				DebugFunctionInfo inf = {addr, object_size, func_name.utf8_str(), object_name.utf8_str()};

				if (m_p->map.Contains(addr))
				{
					String name1(m_p->data.At(m_p->map[addr]).name);
					if (name1.StartsWith(U"_"))
					{
						name1 = name1.Mid(1);
					}
					// EXIT_IF(data.At(map[addr]).name != func_name);
					if (name1 != func_name && !name1.ContainsStr(func_name) && !func_name.ContainsStr(name1))
					{
						if (DBG_PRINTF)
						{
							printf("warning: name1: %s, name2: %s\n", name1.utf8_str().GetData(), func_name.utf8_str().GetData());
						}
						// exit(1);
					}

					continue;
				}

				if (object_unsorted)
				{
					inf.length = 0;
				} else if (m_p->data.Size() > 0 && String(m_p->data.At(m_p->data.Size() - 1).obj) == object_name && !object_start)
				{
					// EXIT_IF(addr <= data.At(data.Size() - 1).addr);

					if (addr <= m_p->data.At(m_p->data.Size() - 1).addr)
					{
						object_unsorted                        = true;
						m_p->data[m_p->data.Size() - 1].length = 0;
						inf.length                             = 0;
					} else
					{
						uintptr_t l                            = addr - m_p->data.At(m_p->data.Size() - 1).addr;
						m_p->data[m_p->data.Size() - 1].length = l;

						EXIT_IF(object_size < l);

						object_size -= l;
						inf.length -= l;
					}
					const DebugFunctionInfo& func = m_p->data.At(m_p->data.Size() - 1);
					if (DBG_PRINTF)
					{
						printf(" -- > %016" PRIx64 "; %" PRIu64 "; %s; %s\n", static_cast<uint64_t>(func.addr),
						       static_cast<uint64_t>(func.length), func.name.GetData(), func.obj.GetData());
					}
				} else if (inf.addr != object_addr)
				{
					// EXIT_IF(inf.addr < object_addr);
					if (inf.addr < object_addr)
					{
						object_unsorted = true;
						inf.length      = 0;
					} else
					{
						uintptr_t l = addr - object_addr;
						object_size -= l;
						inf.length -= l;
					}
				}

				//				if (mem_alloc_obj.Contains(inf.obj, String::CASE_INSENSITIVE)
				//						&& func_name.ContainsAny(mem_alloc_names))
				//				{
				//					inf.is_mem_alloc = true;
				//				}

				m_p->data.Add(inf);
				m_p->map.Put(addr, m_p->data.Size() - 1);

				const DebugFunctionInfo& func = m_p->data.At(m_p->data.Size() - 1);
				if (DBG_PRINTF)
				{
					printf(" -- > %016" PRIx64 "; %" PRIu64 "; %s; %s\n", static_cast<uint64_t>(func.addr),
					       static_cast<uint64_t>(func.length), func.name.GetData(), func.obj.GetData());
				}

				object_start = false;

			} else
			{
				mode = 0;
			}
		}

		if (mode == 2)
		{
			StringList l = s.Split(U" ");

			if (l.Size() == 3 && l[0].StartsWith(U"0x"))
			{
				s    = U".text " + s;
				mode = 0;
			}
		}

		if (mode == 0)
		{
			if (s.TrimLeft().StartsWith(U".text"))
			{
				s            = s.RemoveChar(U'\n');
				StringList l = s.Split(U" ");
				if (l.Size() == 4 && l[0] == U".text")
				{
					mode = 1;

					object_name = l[3].Mid(l[3].FindLastIndex(U"/") + 1);
					object_addr = l[1].ToUint64(16);
					object_size = l[2].ToUint64(16);

					object_start    = true;
					object_unsorted = false;

					if (DBG_PRINTF)
					{
						printf("%08" PRIx32 ", %08" PRIx32 ", %s\n", static_cast<uint32_t>(object_addr), static_cast<uint32_t>(object_size),
						       object_name.utf8_str().GetData());
					}
				} else if (l.Size() == 1 && l[0] != U".text")
				{
					mode = 2;
				}
			}
		}
	}

	f.Close();

	DeleteArray(buf);

	m_p->data.Sort(DebugMapSortSwapFunc, &m_p->map);
	// data.Sort();

	for (uint32_t i = 0; i < m_p->data.Size(); i++)
	{
		if (i > 0 && m_p->data[i - 1].length == 0)
		{
			m_p->data[i - 1].length = m_p->data[i].addr - m_p->data[i - 1].addr;
		}
	}
}

void DebugMap::DumpMap(const String& name)
{
	File::CreateDirectories(name.DirectoryWithoutFilename());

	File file(name);

	if (file.IsInvalid())
	{
		return;
	}

	uint32_t size = m_p->data.Size();

	file.Printf("Addr;Size;Func;Obj\n");

	for (uint32_t i = 0; i < size; i++)
	{
		const DebugFunctionInfo& f = m_p->data.At(i);

		file.Printf("0x%016" PRIx64 ";%" PRIu64 ";%s;%s\n", static_cast<uint64_t>(f.addr), static_cast<uint64_t>(f.length),
		            f.name.GetData(), f.obj.GetData());
	}

	file.Close();
}

static const DebugFunctionInfo* GetFunc(const DebugStack& s, int i)
{
	uintptr_t addr = s.GetAddr(i);

	if (g_dbg_map != nullptr)
	{
		return DebugMapPrivate::FindFunc(g_dbg_map, addr);
	}

	return nullptr;
}

template <class T>
static const T* find_info(const T* data, uintptr_t addr, uint32_t low, uint32_t high)
{

	// AGAIN:
	for (;;)
	{
		if (low == high)
		{
			const T* f = data + low;
			if (addr >= f->addr && addr < f->addr + f->length)
			{
				return f;
			}
		} else
		{
			uint32_t mid = (low + high) >> 1u;

			const T* f = data + mid;
			if (addr < f->addr)
			{
				// return find_info(data, addr, low, mid);
				high = mid;
				// goto AGAIN;
				continue;
			}

			if (addr >= f->addr + f->length)
			{
				// return find_info(data, addr, mid + 1, high);
				low = mid + 1;
				// goto AGAIN;
				continue;
			}

			return f;
		}

		break;
	}

	return nullptr;
}

void DebugMapPrivate::FixBaseAddress()
{
	uintptr_t code_base = 0;
	size_t    code_size = 0;
	sys_get_code_info(&code_base, &code_size);

	if (data.Size() != 0)
	{
		for (auto& p: data)
		{
			p.addr += code_base;
		}
	}

	if (data2.Size() != 0)
	{
		for (auto& p: data2)
		{
			p.addr += code_base;
		}
	}
}

const DebugFunctionInfo* DebugMapPrivate::FindFunc(DebugMap* map, uintptr_t addr)
{
	static DebugFunctionInfo s {};

	if (map->m_p->data.Size() == 0)
	{
		if (map->m_p->data2.Size() == 0)
		{
			return nullptr;
		}

		const DebugFunctionInfo2* f2 = find_info(map->m_p->data2.GetData(), addr, 0, map->m_p->data2.Size() - 1);

		if (f2 != nullptr)
		{
			s.addr   = f2->addr;
			s.length = f2->length;
			s.name   = String::FromUtf8(f2->name).utf8_str();
			s.obj    = String::FromUtf8(f2->obj).utf8_str();

			return &s;
		}

		return nullptr;
	}

	return find_info(map->m_p->data.GetData(), addr, 0, map->m_p->data.Size() - 1);
}

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
static char* strtok_s(char* _Str, const char* _Delim, char** /*_Context*/)
{
	return strtok(_Str, _Delim);
}
#endif

void DebugMap::LoadCsv(const String& name)
{
	File pf(name, File::Mode::Read);
	if (pf.IsInvalid())
	{
		return;
	}

	uint32_t size = pf.Size();
	m_p->buf      = new char[size + 1];
	pf.Read(m_p->buf, size);
	pf.Close();
	m_p->buf[size] = 0;
	char* context  = nullptr;

	[[maybe_unused]] char* p = strtok_s(m_p->buf, "\r\n;", &context);
	EXIT_IF(String::FromUtf8(p) != U"Addr");
	p = strtok_s(nullptr, "\r\n;", &context);
	EXIT_IF(String::FromUtf8(p) != U"Size");
	p = strtok_s(nullptr, "\r\n;", &context);
	EXIT_IF(String::FromUtf8(p) != U"Func");
	p = strtok_s(nullptr, "\r\n;", &context);
	EXIT_IF(String::FromUtf8(p) != U"Obj");

	for (;;)
	{
		char* s1 = strtok_s(nullptr, "\r\n;", &context);
		char* s2 = strtok_s(nullptr, "\r\n;", &context);
		char* s3 = strtok_s(nullptr, "\r\n;", &context);
		char* s4 = strtok_s(nullptr, "\r\n;", &context);

		if ((s1 == nullptr) || (s2 == nullptr) || (s3 == nullptr) || (s4 == nullptr))
		{
			break;
		}

		DebugFunctionInfo2 inf = {static_cast<uintptr_t>(sys_strtoui64(s1 + 2, nullptr, 16)),
		                          static_cast<uintptr_t>(sys_strtoui64(s2, nullptr, 10)), s3, s4};

		m_p->data2.Add(inf);
	}
}

void DebugStack::Print(int from, bool with_name) const
{
	for (int i = from; i < depth; i++)
	{
		const DebugFunctionInfo* f = with_name ? GetFunc(*this, i) : nullptr;
		if (sizeof(uintptr_t) == 4)
		{
			printf("[%d] %08" PRIx32 ", %08" PRIx32 ", %s, %s\n", i - from, static_cast<uint32_t>(GetAddr(i)),
			       f != nullptr ? static_cast<uint32_t>(f->addr) : 0, f != nullptr ? f->obj.GetData() : "unknown",
			       f != nullptr ? f->name.GetData() : "unknown");
		} else
		{
			printf("[%d] %016" PRIx64 ", %016" PRIx64 ", %s, %s\n", i - from, static_cast<uint64_t>(GetAddr(i)),
			       f != nullptr ? static_cast<uint64_t>(f->addr) : 0, f != nullptr ? f->obj.GetData() : "unknown",
			       f != nullptr ? f->name.GetData() : "unknown");
		}
	}
}

void DebugStack::Trace(DebugStack* stack)
{
	stack->depth = DEBUG_MAX_STACK_DEPTH;
	sys_stack_walk(stack->stack, &stack->depth);
}

void DebugStack::PrintAndroid(int from, bool with_name) const
{
	KYTY_LOGI("---stack---\n");
	for (int i = from; i < depth; i++)
	{
		const DebugFunctionInfo* f = with_name ? GetFunc(*this, i) : nullptr;
		if (sizeof(uintptr_t) == 4)
		{
			KYTY_LOGI("[%d] %08" PRIx32 ", %08" PRIx32 ", %s, %s\n", i - from, static_cast<uint32_t>(GetAddr(i)),
			          f != nullptr ? static_cast<uint32_t>(f->addr) : 0, f != nullptr ? f->obj.GetData() : "unknown",
			          f != nullptr ? f->name.GetData() : "unknown");
		} else
		{
			KYTY_LOGI("[%d] %016" PRIx64 ", %016" PRIx64 ", %s, %s\n", i - from, static_cast<uint64_t>(GetAddr(i)),
			          f != nullptr ? static_cast<uint64_t>(f->addr) : 0, f != nullptr ? f->obj.GetData() : "unknown",
			          f != nullptr ? f->name.GetData() : "unknown");
		}
	}
}

String Debug::GetCompiler()
{
#if KYTY_COMPILER == KYTY_COMPILER_MSVC
	return U"msvc";
#elif KYTY_COMPILER == KYTY_COMPILER_CLANG
	return U"clang";
#elif KYTY_COMPILER == KYTY_COMPILER_MINGW
	return U"mingw";
#elif KYTY_COMPILER == KYTY_COMPILER_GCC
	return U"gcc";
#else
	return U"????";
#endif
}

String Debug::GetLinker()
{
#if KYTY_LINKER == KYTY_LINKER_LD
	return U"ld";
#elif KYTY_LINKER == KYTY_LINKER_LLD
	return U"lld";
#elif KYTY_LINKER == KYTY_LINKER_LINK
	return U"link";
#elif KYTY_LINKER == KYTY_LINKER_LLD_LINK
	return U"lld_link";
#else
	return U"??";
#endif
}

String Debug::GetBitness()
{
#if KYTY_BITNESS == 32
	return U"32";
#elif KYTY_BITNESS == 64
	return U"64";
#else
	return U"??";
#endif
}

} // namespace Kyty::Core
