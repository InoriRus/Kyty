#include "Kyty/Core/Common.h"

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Core/String.h"
#include "Kyty/Sys/SysFileIO.h"
#include "Kyty/Sys/SysTimer.h"

#include "SDL_system.h"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

namespace Kyty {

template <typename T>
class Vector;

static String* g_internal_files_dir = nullptr;

static String get_internal_name(const String& name)
{
	return name.StartsWith(U"/") ? name : *g_internal_files_dir + U"/" + name;
}

bool sys_file_io_init()
{
	g_internal_files_dir = new String();

	*g_internal_files_dir = U".";

	return !g_internal_files_dir->IsEmpty();
}

void sys_file_read(void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_read)
{
	if (f.type == SYS_FILE_FILE)
	{
		size_t w = fread(data, 1, size, f.f);
		if (bytes_read != nullptr)
		{
			*bytes_read = w;
		}
	} else if (f.type == SYS_FILE_MEMORY_STAT)
	{
		uint32_t s = size;
		if (f.buf->size != 0)
		{
			uint32_t l = f.buf->size - (f.buf->ptr - f.buf->base);
			if (s > l)
			{
				s = l;
			}
		}
		memcpy(data, f.buf->ptr, s);
		f.buf->ptr += s;
		if (bytes_read != nullptr)
		{
			*bytes_read = s;
		}
	} else if (f.type == SYS_FILE_MEMORY_DYN)
	{
		uint32_t s = size;
		if (f.buf->size != 0)
		{
			uint32_t l = f.buf->size - (f.buf->ptr - f.buf->base);
			if (s > l)
			{
				s = l;
			}
		} else
		{
			s = 0;
		}
		memcpy(data, f.buf->ptr, s);
		f.buf->ptr += s;
		if (bytes_read != nullptr)
		{
			*bytes_read = s;
		}
	}
}

void sys_file_write(const void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_written)
{
	if (f.type == SYS_FILE_FILE)
	{
		size_t w = fwrite(data, 1, size, f.f);
		if (bytes_written != nullptr)
		{
			*bytes_written = w;
		}
	} else if (f.type == SYS_FILE_MEMORY_STAT)
	{
		uint32_t s = size;
		if (f.buf->size != 0)
		{
			uint32_t l = f.buf->size - (f.buf->ptr - f.buf->base);
			if (s > l)
			{
				s = l;
			}
		}
		memcpy(f.buf->ptr, data, s);
		f.buf->ptr += s;
		if (bytes_written != nullptr)
		{
			*bytes_written = s;
		}
	} else if (f.type == SYS_FILE_MEMORY_DYN)
	{
		uint32_t pos = f.buf->ptr - f.buf->base;
		if (f.buf->size < pos + size)
		{
			f.buf->base = static_cast<uint8_t*>(Core::mem_realloc(f.buf->base, pos + size));
			f.buf->ptr  = f.buf->base + pos;
			f.buf->size = pos + size;
		}
		memcpy(f.buf->ptr, data, size);
		f.buf->ptr += size;
		if (bytes_written != nullptr)
		{
			*bytes_written = size;
		}
	}
}

void sys_file_read_r(void* data, uint32_t size, sys_file_t& f)
{
	// DWORD w;
	// ReadFile(f, data, size, &w, 0);

	sys_file_read(data, size, f);

	for (uint32_t i = 0; i < size / 2; i++)
	{
		char t                                   = (static_cast<char*>(data))[i];
		(static_cast<char*>(data))[i]            = (static_cast<char*>(data))[size - i - 1];
		(static_cast<char*>(data))[size - i - 1] = t;
	}
}

void sys_file_write_r(const void* data, uint32_t size, sys_file_t& f)
{
	char* data_r = new char[size];
	for (uint32_t i = 0; i < size; i++)
	{
		data_r[i] = (static_cast<const char*>(data))[size - i - 1];
	}

	sys_file_write(data_r, size, f);

	delete[] data_r;
}

void sys_file_write(uint32_t n, sys_file_t& f)
{
	sys_file_write(&n, 4, f);
}

void sys_file_write_r(uint32_t n, sys_file_t& f)
{
	sys_file_write_r(&n, 4, f);
}

sys_file_t* sys_file_create(const String& file_name)
{
	auto* ret = new sys_file_t;

	String real_name = get_internal_name(file_name);

	ret->f = fopen(real_name.utf8_str().GetData(), "w+");

	if (ret->f == nullptr)
	{
		printf("can't create file: %s\n", real_name.utf8_str().GetData());
	}

	ret->type = SYS_FILE_FILE;

	return ret;
}

sys_file_t* sys_file_open_r(const String& file_name, sys_file_cache_type_t /*cache_type*/)
{
	auto* ret = new sys_file_t;

	ret->type = SYS_FILE_FILE;

	String internal_name = get_internal_name(file_name);
	;

	FILE* f = fopen(internal_name.utf8_str().GetData(), "r");

	if (f == nullptr)
	{
		ret->type = SYS_FILE_ERROR;
	}

	ret->f = f;

	return ret;
}

sys_file_t* sys_file_open(uint8_t* buf, uint32_t buf_size)
{
	auto* ret = new sys_file_t;

	ret->type      = SYS_FILE_MEMORY_STAT;
	ret->buf       = new sys_file_mem_buf_t;
	ret->buf->base = buf;
	ret->buf->ptr  = buf;
	ret->buf->size = buf_size;

	return ret;
}

sys_file_t* sys_file_create()
{
	auto* ret = new sys_file_t;

	ret->type      = SYS_FILE_MEMORY_DYN;
	ret->buf       = new sys_file_mem_buf_t;
	ret->buf->base = nullptr;
	ret->buf->ptr  = nullptr;
	ret->buf->size = 0;

	return ret;
}

sys_file_t* sys_file_open_w(const String& file_name, sys_file_cache_type_t /*cache_type*/)
{
	auto* ret = new sys_file_t;

	String real_name = get_internal_name(file_name);
	;

	FILE* f = fopen(real_name.utf8_str().GetData(), "r+");

	if (f == nullptr)
	{
		ret->type = SYS_FILE_ERROR;
	} else
	{
		ret->type = SYS_FILE_FILE;
	}

	ret->f = f;

	return ret;
}

sys_file_t* sys_file_open_rw(const String& file_name, sys_file_cache_type_t /*cache_type*/)
{
	auto* ret = new sys_file_t;

	String real_name = get_internal_name(file_name);

	FILE* f = fopen(real_name.utf8_str().GetData(), "r+");

	if (f == nullptr)
	{
		ret->type = SYS_FILE_ERROR;
	} else
	{
		ret->type = SYS_FILE_FILE;
	}

	ret->f = f;

	return ret;
}

void sys_file_close(sys_file_t* f)
{
	[[maybe_unused]] int result = 0;

	if (f->type == SYS_FILE_FILE && f->f != nullptr)
	{
		result = fclose(f->f);
	} else if (f->type == SYS_FILE_MEMORY_STAT)
	{
		delete f->buf;
	} else if (f->type == SYS_FILE_MEMORY_DYN)
	{
		Core::mem_free(f->buf->base);
		delete f->buf;
	}

	// f.type = SYS_FILE_ERROR;
	delete f;
}

uint64_t sys_file_size(sys_file_t& f)
{
	[[maybe_unused]] int result = 0;

	if (f.type == SYS_FILE_FILE)
	{
		uint32_t pos  = ftell(f.f);
		result        = fseek(f.f, 0, SEEK_END);
		uint32_t size = ftell(f.f);
		result        = fseek(f.f, pos, SEEK_SET);
		return size;
	}

	if (f.type == SYS_FILE_MEMORY_STAT || f.type == SYS_FILE_MEMORY_DYN)
	{
		return f.buf->size;
	}

	return 0;
}

uint64_t sys_file_size(const String& file_name)
{
	sys_file_t* f    = sys_file_open_r(file_name);
	uint64_t    size = sys_file_size(*f);
	sys_file_close(f);
	return size;
}

bool sys_file_truncate(sys_file_t& /*f*/, uint64_t /*size*/)
{
	return false;
}

bool sys_file_seek(sys_file_t& f, uint64_t offset)
{
	bool ok = true;
	if (f.type == SYS_FILE_FILE)
	{
		ok = (fseek(f.f, static_cast<int64_t>(offset), SEEK_SET) == 0);
		//		LARGE_INTEGER s;
		//		s.QuadPart = offset;
		//		SetFilePointerEx(f.handle, s, 0, FILE_BEGIN);
		// printf("seek: %u\n", offset);
	} else if (f.type == SYS_FILE_MEMORY_STAT || f.type == SYS_FILE_MEMORY_DYN)
	{
		f.buf->ptr = f.buf->base + offset;
	}
	return ok;
}

uint64_t sys_file_tell(sys_file_t& f)
{
	if (f.type == SYS_FILE_FILE)
	{
		return ftell(f.f);
	}

	if (f.type == SYS_FILE_MEMORY_STAT || f.type == SYS_FILE_MEMORY_DYN)
	{
		return f.buf->ptr - f.buf->base;
	}

	return 0;
}

bool sys_file_is_error(sys_file_t& f)
{
	return f.type == SYS_FILE_ERROR || (f.type == SYS_FILE_FILE && f.f == nullptr);
}

bool sys_file_is_directory_existing(const String& path)
{
	String real_name = get_internal_name(path);

	struct stat s
	{
	};

	if (0 != stat(real_name.utf8_str().GetData(), &s))
	{
		return false;
	}

	return S_ISDIR(s.st_mode); // NOLINT
}

bool sys_file_is_file_existing(const String& name)
{
	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	if (0 != stat(real_name.utf8_str().GetData(), &s))
	{
		return false;
	}

	return !S_ISDIR(s.st_mode); // NOLINT
}

bool sys_file_create_directory(const String& path)
{
	String real_name = get_internal_name(path);

	mode_t m = S_IRWXU | S_IRWXG | S_IRWXO; // NOLINT

	String::Utf8 u = real_name.utf8_str();

	int r = mkdir(u.GetDataConst(), m);

	if (r != 0)
	{
		int e = errno;
		if (e == EEXIST)
		{
			printf("mkdir(%s, %" PRIx32 ") failed: The named file exists\n", real_name.C_Str(), static_cast<uint32_t>(m));
		} else
		{
			printf("mkdir(%s, %" PRIx32 ") failed: %d\n", real_name.C_Str(), static_cast<uint32_t>(m), e);
			return false;
		}
	}

	return true;
}

bool sys_file_delete_directory(const String& path)
{
	String real_name = get_internal_name(path);

	return 0 == remove(real_name.utf8_str().GetData());
}

bool sys_file_delete_file(const String& name)
{
	String real_name = get_internal_name(name);

	return 0 == unlink(real_name.utf8_str().GetData());
}

bool sys_file_flush(sys_file_t& f)
{
	if (f.type == SYS_FILE_FILE && f.f != nullptr)
	{
		return (fflush(f.f) == 0);
	}

	return false;
}

SysFileTimeStruct sys_file_get_last_access_time_utc(const String& name)
{
	SysFileTimeStruct r {};

	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	if (0 != stat(real_name.utf8_str().GetData(), &s))
	{
		r.is_invalid = true;
	} else
	{
		r.is_invalid = false;
		r.time       = s.st_atime;
	}

	return r;
}

SysFileTimeStruct sys_file_get_last_write_time_utc(const String& name)
{
	SysFileTimeStruct r {};

	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	if (0 != stat(real_name.utf8_str().GetData(), &s))
	{
		r.is_invalid = true;
	} else
	{
		r.is_invalid = false;
		r.time       = s.st_mtime;
	}

	return r;
}

void sys_file_get_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& a, SysFileTimeStruct& w)
{
	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	if (0 != stat(real_name.utf8_str().GetData(), &s))
	{
		a.is_invalid = true;
		w.is_invalid = true;
	} else
	{
		a.is_invalid = false;
		w.is_invalid = false;
		a.time       = s.st_atime;
		w.time       = s.st_mtime;
	}
}

void sys_file_get_last_access_and_write_time_utc(sys_file_t& /*f*/, SysFileTimeStruct& /*a*/, SysFileTimeStruct& /*w*/)
{
	EXIT("not implemented\n");
}

bool sys_file_set_last_access_time_utc(const String& name, SysFileTimeStruct& access)
{
	if (access.is_invalid)
	{
		return false;
	}

	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	String::Utf8 n = real_name.utf8_str();

	if (0 != stat(n.GetData(), &s))
	{
		return false;
	}

	utimbuf times {};
	times.actime  = access.time;
	times.modtime = s.st_mtime;

	return !(0 != utime(n.GetData(), &times));

	//	if (0 != stat(n.GetData(), &s))
	//	{
	//		return false;
	//	}
	//
	//	times.actime += times.actime - s.st_atime;
	//	times.modtime += times.modtime - s.st_mtime;
	//
	//	if (0 != utime(n.GetData(), &times))
	//	{
	//		return false;
	//	}
}

bool sys_file_set_last_write_time_utc(const String& name, SysFileTimeStruct& write)
{
	if (write.is_invalid)
	{
		return false;
	}

	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	String::Utf8 n = real_name.utf8_str();

	if (0 != stat(n.GetData(), &s))
	{
		return false;
	}

	utimbuf times {};
	times.actime  = s.st_atime;
	times.modtime = write.time;

	return !(0 != utime(n.GetData(), &times));

	//	if (0 != stat(n.GetData(), &s))
	//	{
	//		return false;
	//	}
	//
	//	times.actime += times.actime - s.st_atime;
	//	times.modtime += times.modtime - s.st_mtime;
	//
	//	if (0 != utime(n.GetData(), &times))
	//	{
	//		return false;
	//	}
}

bool sys_file_set_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& access, SysFileTimeStruct& write)
{
	if (access.is_invalid || write.is_invalid)
	{
		return false;
	}

	String real_name = get_internal_name(name);

	struct stat s
	{
	};

	String::Utf8 n = real_name.utf8_str();

	if (0 != stat(n.GetData(), &s))
	{
		return false;
	}

	utimbuf times {};
	times.actime  = access.time;
	times.modtime = write.time;

	return !(0 != utime(n.GetData(), &times));

	//	if (0 != stat(n.GetData(), &s))
	//	{
	//		return false;
	//	}
	//
	//	times.actime += times.actime - s.st_atime;
	//	times.modtime += times.modtime - s.st_mtime;
	//
	//	if (0 != utime(n.GetData(), &times))
	//	{
	//		return false;
	//	}
}

void sys_file_find_files(const String& /*path*/, Vector<sys_file_find_t>& /*out*/)
{
	EXIT("not implemented\n");
}

void sys_file_get_dents(const String& /*path*/, Kyty::Vector<sys_dir_entry_t>& /*out*/)
{
	EXIT("not implemented\n");
}

bool sys_file_copy_file(const String& /*src*/, const String& /*dst*/)
{
	EXIT("not implemented\n");
	return false;
}

bool sys_file_move_file(const String& /*src*/, const String& /*dst*/)
{
	EXIT("not implemented\n");
	return false;
}

void sys_file_remove_readonly(const String& /*name*/)
{
	EXIT("not implemented\n");
}

} // namespace Kyty

#endif
