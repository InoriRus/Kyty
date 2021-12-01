#include "Kyty/Core/File.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/Vector.h"
#include "Kyty/Sys/SysFileIO.h"
#include "Kyty/Sys/SysSwapByteOrder.h"
#include "Kyty/Sys/SysTimer.h"

#include "SDL_rwops.h"
#include "SDL_stdinc.h"

// IWYU pragma: no_include <fileapi.h>
// IWYU pragma: no_include <windows.h>

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef MoveFile
#undef MoveFile
#endif
#endif

namespace Kyty::Core {

// namespace Core {

struct File::FilePrivate
{
	Encoding    e;
	sys_file_t* f;
};

String* g_assets_dir     = nullptr;
String* g_assets_sub_dir = nullptr;

void core_file_init()
{
	if (!sys_file_io_init())
	{
		EXIT("fail in sys_file_io_init()");
	}

	g_assets_dir     = new String();
	g_assets_sub_dir = new String();
}

File::File(): m_p(new FilePrivate)
{
	m_p->f = nullptr;
	m_p->e = File::Encoding::Unknown;
}

File::~File()
{
	EXIT_IF(m_p->f != nullptr);

	Delete(m_p);
}

File::File(const String& name): m_p(new FilePrivate)
{
	m_p->f = nullptr;
	m_p->e = File::Encoding::Unknown;

	Create(name);
}

File::File(const String& name, Mode mode): m_p(new FilePrivate)
{
	m_p->f = nullptr;
	m_p->e = File::Encoding::Unknown;

	Open(name, mode);
}

bool File::IsInvalid() const
{
	return m_p->f == nullptr;
}

bool File::Create(const String& name)
{
	EXIT_IF(m_p->f != nullptr);

	m_file_name = name;

	m_p->f = sys_file_create(name);

	if (sys_file_is_error(*m_p->f))
	{
		Close();
		return false;
	}

	return true;
}

bool File::Open(const String& name, Mode mode)
{
	EXIT_IF(m_p->f != nullptr);

	m_file_name = name;

	if (mode == Mode::Read)
	{
		m_p->f = sys_file_open_r(*g_assets_dir + *g_assets_sub_dir + name);

		if ((m_p->f == nullptr) || sys_file_is_error(*m_p->f))
		{
			Close();
			m_p->f = sys_file_open_r(name);
		}
	} else if (mode == Mode::Write)
	{
		m_p->f = sys_file_open_w(name);
	}
	if (mode == Mode::ReadWrite || mode == Mode::WriteRead)
	{
		m_p->f = sys_file_open_rw(name);
	}

	if ((m_p->f == nullptr) || sys_file_is_error(*m_p->f))
	{
		Close();
		return false;
	}

	return true;
}

bool File::OpenInMem(void* buf, uint32_t buf_size)
{
	EXIT_IF(m_p->f != nullptr);

	m_p->f = sys_file_open(static_cast<uint8_t*>(buf), buf_size);

	if (sys_file_is_error(*m_p->f))
	{
		Close();
		return false;
	}

	return true;
}

bool File::OpenInMem(ByteBuffer& buf)
{
	return OpenInMem(buf.GetData(), buf.Size());
}

bool File::CreateInMem()
{
	EXIT_IF(m_p->f != nullptr);

	m_p->f = sys_file_create();

	if (sys_file_is_error(*m_p->f))
	{
		Close();
		return false;
	}

	return true;
}

void File::Close()
{
	if (m_p->f != nullptr)
	{
		sys_file_close(m_p->f);
		m_p->f = nullptr;
		m_p->e = Encoding::Unknown;
	}
}

uint64_t File::Size() const
{
	EXIT_IF(m_p->f == nullptr);

	if (m_p->f == nullptr)
	{
		return 0;
	}

	return sys_file_size(*m_p->f);
}

uint64_t File::Remaining() const
{
	EXIT_IF(m_p->f == nullptr);

	return Size() - Tell();
}

uint64_t File::Size(const String& name)
{
	return sys_file_size(name);
}

bool File::Seek(uint64_t offset)
{
	EXIT_IF(m_p->f == nullptr);

	return sys_file_seek(*m_p->f, offset);
}

bool File::Truncate(uint64_t size)
{
	EXIT_IF(m_p->f == nullptr);

	return sys_file_truncate(*m_p->f, size);
}

uint64_t File::Tell() const
{
	EXIT_IF(m_p->f == nullptr);

	return sys_file_tell(*m_p->f);
}

void File::Read(void* data, uint32_t size, uint32_t* bytes_read)
{
	EXIT_IF(m_p->f == nullptr);

	if (m_p->f != nullptr)
	{
		sys_file_read(data, size, *m_p->f, bytes_read);
	}
}

void File::Write(const void* data, uint32_t size, uint32_t* bytes_written)
{
	EXIT_IF(m_p->f == nullptr);

	sys_file_write(data, size, *m_p->f, bytes_written);
}

void File::ReadR(void* data, uint32_t size)
{
	EXIT_IF(m_p->f == nullptr);

	sys_file_read_r(data, size, *m_p->f);
}

void File::WriteR(const void* data, uint32_t size)
{
	EXIT_IF(m_p->f == nullptr);

	sys_file_write_r(data, size, *m_p->f);
}

void File::Write(const String& str, uint32_t* bytes_written)
{
	EXIT_IF(IsInvalid());

	if (m_p->e == Encoding::Unknown)
	{
		m_p->e = Encoding::Utf8;
	}

#if KYTY_ENDIAN != KYTY_ENDIAN_LITTLE
#error "KYTY_ENDIAN != KYTY_ENDIAN_LITTLE"
#endif

	switch (m_p->e)
	{
		case Encoding::Utf16BE:
		{
			String::Utf16 u  = str.utf16_str();
			uint32_t      bw = 0;
			for (auto& b: u)
			{
				SwapByteOrder(b);
			}
			Write(u.GetData(), (u.Size() - 1) * 2, &bw);
			if (bytes_written != nullptr)
			{
				*bytes_written = bw;
			}
			break;
		}
		case Encoding::Utf16LE:
		{
			String::Utf16 u  = str.utf16_str();
			uint32_t      bw = 0;
			Write(u.GetData(), (u.Size() - 1) * 2, &bw);
			if (bytes_written != nullptr)
			{
				*bytes_written = bw;
			}
			break;
		}
		case Encoding::Utf32BE:
		{
			String::Utf32 u  = str.utf32_str();
			uint32_t      bw = 0;
			for (auto& b: u)
			{
				SwapByteOrder(b);
			}
			Write(u.GetData(), (u.Size() - 1) * 4, &bw);
			if (bytes_written != nullptr)
			{
				*bytes_written = bw;
			}
			break;
		}
		case Encoding::Utf32LE:
		{
			String::Utf32 u  = str.utf32_str();
			uint32_t      bw = 0;
			Write(u.GetData(), (u.Size() - 1) * 4, &bw);
			if (bytes_written != nullptr)
			{
				*bytes_written = bw;
			}
			break;
		}
		case Encoding::Utf8:
		case Encoding::Unknown:
		default:
		{
			String::Utf8 u  = str.utf8_str();
			uint32_t     bw = 0;
			Write(u.GetData(), u.Size() - 1, &bw);
			if (bytes_written != nullptr)
			{
				*bytes_written = bw;
			}
			break;
		}
	}
}

void File::WriteBOM()
{
	EXIT_IF(IsInvalid());

	if (m_p->e == Encoding::Unknown)
	{
		m_p->e = Encoding::Utf8;
	}

#if KYTY_ENDIAN != KYTY_ENDIAN_LITTLE
#error "KYTY_ENDIAN != KYTY_ENDIAN_LITTLE"
#endif

	switch (m_p->e)
	{
		case Encoding::Utf16BE:
		{
			const uint8_t bom[2] = {0xFE, 0xFF};
			Write(&bom, 2);
			break;
		}
		case Encoding::Utf16LE:
		{
			const uint8_t bom[2] = {0xFF, 0xFE};
			Write(&bom, 2);
			break;
		}
		case Encoding::Utf32BE:
		{
			const uint8_t bom[4] = {0x00, 0x00, 0xFE, 0xFF};
			Write(&bom, 4);
			break;
		}
		case Encoding::Utf32LE:
		{
			const uint8_t bom[4] = {0xFF, 0xFE, 0x00, 0x00};
			Write(&bom, 4);
			break;
		}
		case Encoding::Utf8:
		{
			const uint8_t bom[3] = {0xEF, 0xBB, 0xBF};
			Write(&bom, 3);
			break;
		}
		case Encoding::Unknown:
		default:
		{
			break;
		}
	}
}

template <class T, class OP>
static void file_read_line(File* f, T end_line, Vector<T>& buf, OP&& swap)
{
	for (;;)
	{
		T        c;
		uint32_t br = 0;
		f->Read(&c, sizeof(c), &br);
		swap(c);
		if (br == 0)
		{
			break;
		}
		buf.Add(c);
		if (c == end_line)
		{
			break;
		}
	}

	buf.Add(0);
}

String File::ReadLine()
{
	EXIT_IF(IsInvalid());

	if (IsInvalid())
	{
		return U"";
	}

	if (m_p->e == Encoding::Unknown)
	{
		m_p->e = Encoding::Utf8;
	}

#if KYTY_ENDIAN != KYTY_ENDIAN_LITTLE
#error "KYTY_ENDIAN != KYTY_ENDIAN_LITTLE"
#endif

	String s;

	switch (m_p->e)
	{
		case Encoding::Utf16BE:
		{
			Vector<char16_t> buf16;
			file_read_line(this, u'\n', buf16, &SwapByteOrder<char16_t>);
			s = String::FromUtf16(buf16.GetData());
			break;
		}
		case Encoding::Utf16LE:
		{
			Vector<char16_t> buf16;
			file_read_line(this, u'\n', buf16, &NoSwapByteOrder<char16_t>);
			s = String::FromUtf16(buf16.GetData());
			break;
		}
		case Encoding::Utf32BE:
		{
			Vector<char32_t> buf32;
			file_read_line(this, U'\n', buf32, &SwapByteOrder<char32_t>);
			s = String::FromUtf32(buf32.GetData());
			break;
		}
		case Encoding::Utf32LE:
		{
			Vector<char32_t> buf32;
			file_read_line(this, U'\n', buf32, &NoSwapByteOrder<char32_t>);
			s = String::FromUtf32(buf32.GetData());
			break;
		}
		case Encoding::Utf8:
		case Encoding::Unknown:
		default:
		{
			Vector<char> buf8;
			file_read_line(this, '\n', buf8, &NoSwapByteOrder<char>);
			s = String::FromUtf8(buf8.GetData());
			break;
		}
	}

	return s.ReplaceStr(U"\r\n", U"\n");
}

void File::Printf(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	String s;
	s.Printf(format, args);

	va_end(args);

	Write(s.ReplaceStr(U"\n", U"\r\n"));
}

void File::PrintfLF(const char* format, ...)
{
	va_list args {};
	va_start(args, format);

	String s;
	s.Printf(format, args);

	va_end(args);

	Write(s);
}

bool File::IsDirectoryExisting(const String& path)
{
	return sys_file_is_directory_existing(path.EndsWith(U"/") ? path.RemoveLast(1) : path);
}

bool File::IsFileExisting(const String& name)
{
	return sys_file_is_file_existing(name);
}

bool File::IsAssetFileExisting(const String& name)
{
	return sys_file_is_file_existing(*g_assets_dir + *g_assets_sub_dir + name);
}

bool File::CreateDirectory(const String& path) // @suppress("Member declaration not found")
{
	return sys_file_create_directory(path.EndsWith(U"/") ? path.RemoveLast(1) : path);
}

bool File::DeleteDirectory(const String& path)
{
	return sys_file_delete_directory(path.EndsWith(U"/") ? path.RemoveLast(1) : path);
}

bool File::CreateDirectories(const String& path)
{
	String real_path = path.ReplaceChar(U'\\', U'/');

	StringList list = real_path.Split(U"/");

	String p;

	FOR (si, list)
	{
		const String& s = list.At(si);

		if (si != 0 || real_path.StartsWith(U"/"))
		{
			p += U"/";
		}

		p += s;

		if (IsDirectoryExisting(p))
		{
			continue;
		}

		if (!CreateDirectory(p)) // @suppress("Invalid arguments")
		{
			return false;
		}
	}

	return true;
}

bool File::DeleteDirectories(const String& path)
{
	String real_path = path.ReplaceChar(U'\\', U'/');

	StringList list = real_path.Split(U"/");

	String     p;
	StringList list2;

	FOR (si, list)
	{
		const String& s = list.At(si);

		if (si != 0 || real_path.StartsWith(U"/"))
		{
			p += U"/";
		}

		p += s;

		list2.Add(p);
	}

	uint32_t num = list2.Size();

	for (uint32_t si = num - 1; si < num; si--)
	{
		if (!DeleteDirectory(list2.At(si)))
		{
			return false;
		}
	}

	return true;
}

bool File::DeleteFile(const String& name) // @suppress("Member declaration not found")
{
	return sys_file_delete_file(name);
}

bool File::Flush()
{
	EXIT_IF(m_p->f == nullptr);

	return sys_file_flush(*m_p->f);
}

String File::Read(const String& name, Encoding e)
{
	File f;
	f.Open(name, Mode::Read);

	EXIT_IF(f.IsInvalid());

	f.SetEncoding(e);

	String str = f.ReadWholeString();

	f.Close();

	return str;
}

static Sint64 myseekfunc(SDL_RWops* context, Sint64 offset, int whence)
{
	File* f = static_cast<File*>(context->hidden.unknown.data1);

	switch (whence)
	{
		case RW_SEEK_SET:
			if (offset < 0)
			{
				return -1;
			}
			f->Seek(offset);
			return static_cast<Sint64>(f->Tell());
			break;
		case RW_SEEK_CUR:
			f->Seek(f->Tell() + offset);
			return static_cast<Sint64>(f->Tell());
			break;
		case RW_SEEK_END:
			f->Seek(f->Size() + offset);
			return static_cast<Sint64>(f->Tell());
			break;
		default: break;
	}

	return -1;
}

static size_t myreadfunc(SDL_RWops* context, void* ptr, size_t size, size_t maxnum)
{
	File* f = static_cast<File*>(context->hidden.unknown.data1);

	uint32_t bytes = 0;

	size_t sm = size * maxnum;

	EXIT_IF(sizeof(size_t) > 4 && (uint64_t(sm) >> 32u) > 0);

	auto sm2 = static_cast<uint32_t>(sm);

	f->Read(ptr, sm2, &bytes);

	return bytes / size;
}

static size_t mywritefunc(SDL_RWops* context, const void* ptr, size_t size, size_t num)
{
	File* f = static_cast<File*>(context->hidden.unknown.data1);

	uint32_t bytes = 0;

	size_t sm = size * num;

	EXIT_IF(sizeof(size_t) > 4 && (uint64_t(sm) >> 32u) > 0);

	auto sm2 = static_cast<uint32_t>(sm);

	f->Write(ptr, sm2, &bytes);

	return bytes / size;
}

static int myclosefunc(SDL_RWops* context)
{
	File* f = static_cast<File*>(context->hidden.unknown.data1);

	f->Close();

	SDL_FreeRW(context);
	return 0;
}

void File::SetAssetsDir(const String& dir)
{
	*g_assets_dir = dir.ReplaceChar(U'\\', U'/');
	if (!g_assets_dir->EndsWith(U"/"))
	{
		*g_assets_dir += U"/";
	}
}

String File::GetAssetsDir()
{
	return *g_assets_dir;
}

void File::SetAssetsSubDir(const String& dir)
{
	*g_assets_sub_dir = dir.ReplaceChar(U'\\', U'/');
	if (!g_assets_sub_dir->EndsWith(U"/"))
	{
		*g_assets_sub_dir += U"/";
	}

	if (!File::IsAssetFileExisting(U"top.level"))
	{
		EXIT("invalid asset sub dir: %s\n", dir.C_Str());
	}
}

String File::GetAssetsSubDir()
{
	return *g_assets_sub_dir;
}

File::Encoding File::GetEncoding() const
{
	return m_p->e;
}

void File::SetEncoding(Encoding e)
{
	EXIT_IF(m_p->e != Encoding::Unknown);

	m_p->e = e;
}

void File::DetectEncoding()
{
	EXIT_IF(m_p->e != Encoding::Unknown);

	auto pos = Tell();

	Seek(0);

	uint8_t  b[2]      = {0, 0};
	uint32_t bytes_num = 0;

	Read(b, 2, &bytes_num);

	if (bytes_num == 2 && b[0] == 0 && b[1] == 0)
	{
		Read(b, 2, &bytes_num);

		if (bytes_num == 2 && b[0] == 0xFE && b[1] == 0xFF)
		{
			m_p->e = Encoding::Utf32BE;
		}
	} else if (bytes_num == 2 && b[0] == 0xFF && b[1] == 0xFE)
	{
		Read(b, 2, &bytes_num);

		if (bytes_num == 2 && b[0] == 0 && b[1] == 0)
		{
			m_p->e = Encoding::Utf32LE;
		} else
		{
			m_p->e = Encoding::Utf16LE;
		}
	} else if (bytes_num == 2 && b[0] == 0xFE && b[1] == 0xFF)
	{
		m_p->e = Encoding::Utf16BE;
	} else if (bytes_num == 2 && b[0] == 0xEF && b[1] == 0xBB)
	{
		Read(b, 1, &bytes_num);

		if (bytes_num == 1 && b[0] == 0xBF)
		{
			m_p->e = Encoding::Utf8;
		}
	}

	Seek(pos);
}

SDL_RWops* File::CreateSdlRWops()
{
	SDL_RWops* c = SDL_AllocRW();

	EXIT_IF(c == nullptr);

	c->seek  = myseekfunc;
	c->read  = myreadfunc;
	c->write = mywritefunc;
	c->close = myclosefunc;
	c->type  = 0;

	c->hidden.unknown.data1 = this;

	return c;
}

ByteBuffer File::ReadWholeBuffer()
{
	EXIT_IF(IsInvalid());
	EXIT_IF(Tell() != 0);

	uint64_t s = Size();

	EXIT_IF((s >> 32u) != 0);

	ByteBuffer buf(s, false);

	Read(buf.GetData(), s);

	return buf;
}

String File::ReadWholeString()
{
	EXIT_IF(IsInvalid());
	EXIT_IF(Tell() != 0);

	if (m_p->e == Encoding::Unknown)
	{
		m_p->e = Encoding::Utf8;
	}

#if KYTY_ENDIAN != KYTY_ENDIAN_LITTLE
#error "KYTY_ENDIAN != KYTY_ENDIAN_LITTLE"
#endif

	String s;

	switch (m_p->e)
	{
		case Encoding::Utf16BE:
		{
			uint64_t size = Size() / 2;
			EXIT_IF((size >> 32u) != 0);
			Vector<char16_t> buf(size + 1, false);
			Read(buf.GetData(), size * 2);
			buf[size] = 0;
			for (auto& b: buf)
			{
				SwapByteOrder(b);
			}
			s = String::FromUtf16(buf.GetData());
			break;
		}
		case Encoding::Utf16LE:
		{
			uint64_t size = Size() / 2;
			EXIT_IF((size >> 32u) != 0);
			Vector<char16_t> buf(size + 1, false);
			Read(buf.GetData(), size * 2);
			buf[size] = 0;
			s         = String::FromUtf16(buf.GetData());
			break;
		}
		case Encoding::Utf32BE:
		{
			uint64_t size = Size() / 4;
			EXIT_IF((size >> 32u) != 0);
			Vector<char32_t> buf(size + 1, false);
			Read(buf.GetData(), size * 4);
			buf[size] = 0;
			for (auto& b: buf)
			{
				SwapByteOrder(b);
			}
			s = String::FromUtf32(buf.GetData());
			break;
		}
		case Encoding::Utf32LE:
		{
			uint64_t size = Size() / 4;
			EXIT_IF((size >> 32u) != 0);
			Vector<char32_t> buf(size + 1, false);
			Read(buf.GetData(), size * 4);
			buf[size] = 0;
			s         = String::FromUtf32(buf.GetData());
			break;
		}
		case Encoding::Utf8:
		case Encoding::Unknown:
		default:
		{
			uint64_t size = Size();
			EXIT_IF((size >> 32u) != 0);
			Vector<char> buf(size + 1, false);
			Read(buf.GetData(), size);
			buf[size] = 0;
			s         = String::FromUtf8(buf.GetData());
			break;
		}
	}

	return s.ReplaceStr(U"\r\n", U"\n");
}

ByteBuffer File::Read(uint32_t size)
{
	ByteBuffer buf(size);
	uint32_t   b = 0;
	Read(buf.GetData(), size, &b);
	buf.RemoveAt(b, size - b);
	return buf;
}

void File::Write(const ByteBuffer& buf, uint32_t* bytes_written)
{
	Write(buf.GetDataConst(), buf.Size(), bytes_written);
}

DateTime File::GetLastAccessTimeUTC(const String& name)
{
	SysTimeStruct t {};
	sys_file_to_system_time_utc(sys_file_get_last_access_time_utc(name), t);

	if (t.is_invalid)
	{
		return DateTime();
	}

	return DateTime(Date(t.Year, t.Month, t.Day), Time(t.Hour, t.Minute, t.Second, t.Milliseconds));
}

DateTime File::GetLastWriteTimeUTC(const String& name)
{
	SysTimeStruct t {};
	sys_file_to_system_time_utc(sys_file_get_last_write_time_utc(name), t);

	if (t.is_invalid)
	{
		return DateTime();
	}

	return DateTime(Date(t.Year, t.Month, t.Day), Time(t.Hour, t.Minute, t.Second, t.Milliseconds));
}

void File::GetLastAccessAndWriteTimeUTC(const String& name, DateTime* access, DateTime* write)
{
	EXIT_IF(access == nullptr);
	EXIT_IF(write == nullptr);

	SysTimeStruct     at {};
	SysTimeStruct     wt {};
	SysFileTimeStruct af {};
	SysFileTimeStruct wf {};

	sys_file_get_last_access_and_write_time_utc(name, af, wf);

	sys_file_to_system_time_utc(af, at);
	sys_file_to_system_time_utc(wf, wt);

	if (at.is_invalid || wt.is_invalid)
	{
		*access = DateTime();
		*write  = DateTime();
	} else
	{
		*access = DateTime(Date(at.Year, at.Month, at.Day), Time(at.Hour, at.Minute, at.Second, at.Milliseconds));
		*write  = DateTime(Date(wt.Year, wt.Month, wt.Day), Time(wt.Hour, wt.Minute, wt.Second, wt.Milliseconds));
	}
}

void File::GetLastAccessAndWriteTimeUTC(DateTime* access, DateTime* write)
{
	EXIT_IF(access == nullptr);
	EXIT_IF(write == nullptr);

	EXIT_IF(m_p->f == nullptr);

	if (m_p->f == nullptr)
	{
		*access = DateTime();
		*write  = DateTime();
		return;
	}

	SysTimeStruct     at {};
	SysTimeStruct     wt {};
	SysFileTimeStruct af {};
	SysFileTimeStruct wf {};

	sys_file_get_last_access_and_write_time_utc(*m_p->f, af, wf);

	sys_file_to_system_time_utc(af, at);
	sys_file_to_system_time_utc(wf, wt);

	if (at.is_invalid || wt.is_invalid)
	{
		*access = DateTime();
		*write  = DateTime();
	} else
	{
		*access = DateTime(Date(at.Year, at.Month, at.Day), Time(at.Hour, at.Minute, at.Second, at.Milliseconds));
		*write  = DateTime(Date(wt.Year, wt.Month, wt.Day), Time(wt.Hour, wt.Minute, wt.Second, wt.Milliseconds));
	}
}

bool File::SetLastAccessTimeUTC(const String& name, const DateTime& dt)
{
	SysTimeStruct     t {};
	SysFileTimeStruct f {};

	t.is_invalid = false;

	int year  = 0;
	int month = 0;
	int day   = 0;
	dt.GetDate().Get(&year, &month, &day);
	t.Year  = year;
	t.Month = month;
	t.Day   = day;

	int hour         = 0;
	int minute       = 0;
	int second       = 0;
	int milliseconds = 0;
	dt.GetTime().Get(&hour, &minute, &second, &milliseconds);
	t.Hour         = hour;
	t.Minute       = minute;
	t.Second       = second;
	t.Milliseconds = milliseconds;

	sys_system_to_file_time_utc(t, f);

	return sys_file_set_last_access_time_utc(name, f);
}

bool File::SetLastWriteTimeUTC(const String& name, const DateTime& dt)
{
	SysTimeStruct     t {};
	SysFileTimeStruct f {};

	t.is_invalid = false;

	int year  = 0;
	int month = 0;
	int day   = 0;
	dt.GetDate().Get(&year, &month, &day);
	t.Year  = year;
	t.Month = month;
	t.Day   = day;

	int hour         = 0;
	int minute       = 0;
	int second       = 0;
	int milliseconds = 0;
	dt.GetTime().Get(&hour, &minute, &second, &milliseconds);
	t.Hour         = hour;
	t.Minute       = minute;
	t.Second       = second;
	t.Milliseconds = milliseconds;

	sys_system_to_file_time_utc(t, f);

	return sys_file_set_last_write_time_utc(name, f);
}

bool File::SetLastAccessAndWriteTimeUTC(const String& name, const DateTime& access, const DateTime& write)
{
	SysTimeStruct     at {};
	SysTimeStruct     wt {};
	SysFileTimeStruct af {};
	SysFileTimeStruct wf {};

	at.is_invalid = false;
	wt.is_invalid = false;

	int year  = 0;
	int month = 0;
	int day   = 0;
	access.GetDate().Get(&year, &month, &day);
	at.Year  = year;
	at.Month = month;
	at.Day   = day;
	write.GetDate().Get(&year, &month, &day);
	wt.Year  = year;
	wt.Month = month;
	wt.Day   = day;

	int hour         = 0;
	int minute       = 0;
	int second       = 0;
	int milliseconds = 0;
	access.GetTime().Get(&hour, &minute, &second, &milliseconds);
	at.Hour         = hour;
	at.Minute       = minute;
	at.Second       = second;
	at.Milliseconds = milliseconds;
	write.GetTime().Get(&hour, &minute, &second, &milliseconds);
	wt.Hour         = hour;
	wt.Minute       = minute;
	wt.Second       = second;
	wt.Milliseconds = milliseconds;

	sys_system_to_file_time_utc(at, af);
	sys_system_to_file_time_utc(wt, wf);

	return sys_file_set_last_access_and_write_time_utc(name, af, wf);
}

Vector<File::FindInfo> File::FindFiles(const String& path)
{
	Vector<sys_file_find_t> files;

	sys_file_find_files(path, files);

	uint32_t len = path.Size();

	if (!path.EndsWith(U"/") && !path.EndsWith(U"\\"))
	{
		len++;
	}

	Vector<File::FindInfo> ret;

	for (const auto& f: files)
	{
		File::FindInfo r {};

		r.path_with_name     = f.path_with_name;
		r.rel_path_with_name = f.path_with_name.Mid(len);
		r.size               = f.size;

		SysTimeStruct at {};
		SysTimeStruct wt {};

		sys_file_to_system_time_utc(f.last_access_time, at);
		sys_file_to_system_time_utc(f.last_write_time, wt);

		if (!at.is_invalid && !wt.is_invalid)
		{
			r.last_access_time = DateTime(Date(at.Year, at.Month, at.Day), Time(at.Hour, at.Minute, at.Second, at.Milliseconds));
			r.last_write_time  = DateTime(Date(wt.Year, wt.Month, wt.Day), Time(wt.Hour, wt.Minute, wt.Second, wt.Milliseconds));
		}

		// printf("%s, %s, %" PRIu64", %s, %s\n", r.path_with_name.C_Str(), r.rel_path_with_name.C_Str(), r.size,
		// r.last_access_time.ToString().C_Str(), r.last_write_time.ToString().C_Str());

		ret.Add(r);
	}

	return ret;
}

Vector<File::DirEntry> File::GetDirEntries(const String& path)
{
	Vector<sys_dir_entry_t> files;

	sys_file_get_dents(path, files);

	Vector<File::DirEntry> ret;

	for (const auto& f: files)
	{
		File::DirEntry r {};

		r.name    = f.name;
		r.is_file = f.is_file;

		ret.Add(r);
	}

	return ret;
}

bool File::CopyFile(const String& src, const String& dst) // @suppress("Member declaration not found")
{
	return sys_file_copy_file(src, dst);
}

bool File::MoveFile(const String& src, const String& dst) // @suppress("Member declaration not found")
{
	if (IsFileExisting(dst))
	{
		DeleteFile(dst); // @suppress("Invalid arguments")
	}

	return sys_file_move_file(src, dst);
}

void File::RemoveReadonly(const String& name)
{
	sys_file_remove_readonly(name);
}

namespace SyncDirectories {

struct CopyT
{
	String   src, dst;
	DateTime src_last_access_time, src_last_write_time;
};

static void sync(const Vector<SyncDirectories::CopyT>& list)
{
	for (const auto& f: list)
	{
		File::CreateDirectories(f.dst.DirectoryWithoutFilename());
		File::CopyFile(f.src, f.dst); // @suppress("Invalid arguments") // @suppress("Function cannot be resolved")
		File::RemoveReadonly(f.dst);
		File::SetLastAccessAndWriteTimeUTC(f.dst, f.src_last_access_time, f.src_last_write_time);

		// printf("%s: %s\n", str, f.dst.C_Str());
	}
}

} // namespace SyncDirectories

void File::SyncDirectories(const String& src_dir, const String& dst_dir, bool del_dst)
{
	auto src_list = FindFiles(src_dir);
	auto dst_list = FindFiles(dst_dir);

	String dir = (dst_dir.EndsWith(U"/") || dst_dir.EndsWith(U"\\") ? dst_dir.RemoveLast(1) : dst_dir);

	Vector<SyncDirectories::CopyT> new_list;
	Vector<SyncDirectories::CopyT> upd_list;

	for (const auto& src: src_list)
	{

		bool sync     = true;
		bool copy_new = true;

		for (const auto& dst: dst_list)
		{

			if (src.rel_path_with_name == dst.rel_path_with_name)
			{
				copy_new = false;

				if (src.size == dst.size && src.last_access_time == dst.last_access_time && src.last_write_time == dst.last_write_time)
				{
					sync = false;
					break;
				}
			}
		}

		if (sync)
		{
			String name = dir + U"/" + src.rel_path_with_name;

			if (copy_new)
			{
				new_list.Add({src.path_with_name, name, src.last_access_time, src.last_write_time});
			} else
			{
				upd_list.Add({src.path_with_name, name, src.last_access_time, src.last_write_time});
			}
		}
	}

	int del_cnt = 0;
	if (del_dst)
	{
		FOR (i, dst_list)
		{
			const FindInfo& dst = dst_list.At(i);

			bool del = true;

			FOR (j, src_list)
			{
				const FindInfo& src = src_list.At(j);

				if (src.rel_path_with_name == dst.rel_path_with_name)
				{
					del = false;
					break;
				}
			}

			if (del)
			{
				DeleteFile(dst.path_with_name); // @suppress("Invalid arguments")

				del_cnt++;
				// printf("del: %s\n", dst.path_with_name.C_Str());
			}
		}
	}

	if (new_list.Size() + upd_list.Size() + del_cnt > 0)
	{
		printf("[sync] new: %d, upd: %d, del %d -> %s/...\n", static_cast<int>(new_list.Size()), static_cast<int>(upd_list.Size()), del_cnt,
		       dst_dir.C_Str());
	}

	SyncDirectories::sync(new_list);
	SyncDirectories::sync(upd_list);
}

} // namespace Kyty::Core
