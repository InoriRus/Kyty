#ifndef SYS_WIN32_INCLUDE_KYTY_SYSFILEIO_H_
#define SYS_WIN32_INCLUDE_KYTY_SYSFILEIO_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS
//#error "KYTY_PLATFORM != KYTY_PLATFORM_WINDOWS"
#else

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Sys/SysTimer.h"

namespace Kyty {

using KYTY_HANDLE = void*;

template <typename T>
class Vector;

// NOLINTNEXTLINE(readability-identifier-naming)
enum sys_file_type_t
{
	SYS_FILE_ERROR,       // NOLINT(readability-identifier-naming)
	SYS_FILE_MEMORY_STAT, // NOLINT(readability-identifier-naming)
	SYS_FILE_FILE,        // NOLINT(readability-identifier-naming)
	SYS_FILE_MEMORY_DYN   // NOLINT(readability-identifier-naming)
};

// NOLINTNEXTLINE(readability-identifier-naming)
enum sys_file_cache_type_t
{
	SYS_FILE_CACHE_AUTO            = 0, // NOLINT(readability-identifier-naming)
	SYS_FILE_CACHE_RANDOM_ACCESS   = 1, // NOLINT(readability-identifier-naming)
	SYS_FILE_CACHE_SEQUENTIAL_SCAN = 2  // NOLINT(readability-identifier-naming)
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct sys_file_mem_buf_t
{
	uint8_t* base;
	uint8_t* ptr;
	uint32_t size;
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct sys_file_t
{
	sys_file_type_t type;
	union
	{
		KYTY_HANDLE         handle;
		sys_file_mem_buf_t* buf;
	};
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct sys_file_find_t
{
	String            path_with_name;
	SysFileTimeStruct last_access_time;
	SysFileTimeStruct last_write_time;
	uint64_t          size;
};

// NOLINTNEXTLINE(readability-identifier-naming)
struct sys_dir_entry_t
{
	String name;
	bool   is_file;
};

void sys_file_read(void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_read = nullptr);           // NOLINT(google-runtime-references)
void sys_file_write(const void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_written = nullptr); // NOLINT(google-runtime-references)
void sys_file_read_r(void* data, uint32_t size, sys_file_t& f);                                         // NOLINT(google-runtime-references)
void sys_file_write_r(const void* data, uint32_t size, sys_file_t& f);                                  // NOLINT(google-runtime-references)
sys_file_t*       sys_file_create(const String& file_name);
sys_file_t*       sys_file_open_r(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
sys_file_t*       sys_file_open_w(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
sys_file_t*       sys_file_open(uint8_t* buf, uint32_t buf_size);
sys_file_t*       sys_file_create();
sys_file_t*       sys_file_open_rw(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
void              sys_file_close(sys_file_t* f);
uint64_t          sys_file_size(sys_file_t& f);                    // NOLINT(google-runtime-references)
bool              sys_file_seek(sys_file_t& f, uint64_t offset);   // NOLINT(google-runtime-references)
uint64_t          sys_file_tell(sys_file_t& f);                    // NOLINT(google-runtime-references)
bool              sys_file_truncate(sys_file_t& f, uint64_t size); // NOLINT(google-runtime-references)
void              sys_file_write(uint32_t n, sys_file_t& f);       // NOLINT(google-runtime-references)
void              sys_file_write_r(uint32_t n, sys_file_t& f);     // NOLINT(google-runtime-references)
uint64_t          sys_file_size(const String& file_name);
bool              sys_file_is_error(sys_file_t& f); // NOLINT(google-runtime-references)
bool              sys_file_io_init();
bool              sys_file_is_directory_existing(const String& path);
bool              sys_file_is_file_existing(const String& name);
bool              sys_file_create_directory(const String& path);
bool              sys_file_delete_directory(const String& path);
bool              sys_file_delete_file(const String& name);
bool              sys_file_flush(sys_file_t& f); // NOLINT(google-runtime-references)
SysFileTimeStruct sys_file_get_last_access_time_utc(const String& name);
SysFileTimeStruct sys_file_get_last_write_time_utc(const String& name);
// NOLINTNEXTLINE(google-runtime-references)
void sys_file_get_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& a, SysFileTimeStruct& w);
// NOLINTNEXTLINE(google-runtime-references)
void sys_file_get_last_access_and_write_time_utc(sys_file_t& f, SysFileTimeStruct& a, SysFileTimeStruct& w);
// NOLINTNEXTLINE(google-runtime-references)
bool sys_file_set_last_access_time_utc(const String& name, SysFileTimeStruct& access);
// NOLINTNEXTLINE(google-runtime-references)
bool sys_file_set_last_write_time_utc(const String& name, SysFileTimeStruct& write);
// NOLINTNEXTLINE(google-runtime-references)
bool sys_file_set_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& access, SysFileTimeStruct& write);
// NOLINTNEXTLINE(google-runtime-references)
void sys_file_find_files(const String& path, Kyty::Vector<sys_file_find_t>& out);
// NOLINTNEXTLINE(google-runtime-references)
void sys_file_get_dents(const String& path, Kyty::Vector<sys_dir_entry_t>& out);
bool sys_file_copy_file(const String& src, const String& dst);
bool sys_file_move_file(const String& src, const String& dst);
void sys_file_remove_readonly(const String& name);

} // namespace Kyty

#endif

#endif /* SYS_WIN32_INCLUDE_KYTY_SYSFILEIO_H_ */
