#ifndef SYS_LINUX_INCLUDE_KYTY_SYSFILEIO_H_
#define SYS_LINUX_INCLUDE_KYTY_SYSFILEIO_H_

// IWYU pragma: private

#if KYTY_PLATFORM != KYTY_PLATFORM_LINUX
//#error "KYTY_PLATFORM != KYTY_PLATFORM_LINUX"
#else

#include "Kyty/Core/String.h"
#include "Kyty/Sys/SysLinuxTimer.h"

namespace Kyty {

enum sys_file_type_t
{
	SYS_FILE_ERROR,
	SYS_FILE_MEMORY_STAT,
	SYS_FILE_FILE,
	SYS_FILE_MEMORY_DYN
};

enum sys_file_cache_type_t
{
	SYS_FILE_CACHE_AUTO            = 0,
	SYS_FILE_CACHE_RANDOM_ACCESS   = 1,
	SYS_FILE_CACHE_SEQUENTIAL_SCAN = 2
};

struct sys_file_mem_buf_t
{
	uint8_t* base;
	uint8_t* ptr;
	uint32_t size;
};

struct sys_file_t
{
	sys_file_type_t type;
	union
	{
		FILE*               f;
		sys_file_mem_buf_t* buf;
	};
};

struct sys_file_find_t
{
	String            path_with_name;
	SysFileTimeStruct last_access_time;
	SysFileTimeStruct last_write_time;
	uint64_t          size;
};

struct sys_dir_entry_t
{
	String name;
	bool   is_file;
};

void              sys_file_read(void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_read = 0);
void              sys_file_write(const void* data, uint32_t size, sys_file_t& f, uint32_t* bytes_written = 0);
void              sys_file_read_r(void* data, uint32_t size, sys_file_t& f);
void              sys_file_write_r(const void* data, uint32_t size, sys_file_t& f);
sys_file_t*       sys_file_create(const String& file_name);
sys_file_t*       sys_file_open_r(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
sys_file_t*       sys_file_open_w(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
sys_file_t*       sys_file_open(uint8_t* buf, uint32_t buf_size);
sys_file_t*       sys_file_create();
sys_file_t*       sys_file_open_rw(const String& file_name, sys_file_cache_type_t cache_type = SYS_FILE_CACHE_AUTO);
void              sys_file_close(sys_file_t* f);
uint64_t          sys_file_size(sys_file_t& f);
bool              sys_file_seek(sys_file_t& f, uint64_t offset);
uint64_t          sys_file_tell(sys_file_t& f);
bool              sys_file_truncate(sys_file_t& f, uint64_t size);
void              sys_file_write(uint32_t n, sys_file_t& f);
void              sys_file_write_r(uint32_t n, sys_file_t& f);
uint64_t          sys_file_size(const String& file_name);
bool              sys_file_is_error(sys_file_t& f);
bool              sys_file_io_init();
bool              sys_file_is_directory_existing(const String& path);
bool              sys_file_is_file_existing(const String& name);
bool              sys_file_create_directory(const String& path);
bool              sys_file_delete_directory(const String& path);
bool              sys_file_delete_file(const String& name);
bool              sys_file_flush(sys_file_t& f);
SysFileTimeStruct sys_file_get_last_access_time_utc(const String& name);
SysFileTimeStruct sys_file_get_last_write_time_utc(const String& name);
void              sys_file_get_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& a, SysFileTimeStruct& w);
void              sys_file_get_last_access_and_write_time_utc(sys_file_t& f, SysFileTimeStruct& a, SysFileTimeStruct& w);
bool              sys_file_set_last_access_time_utc(const String& name, SysFileTimeStruct& access);
bool              sys_file_set_last_write_time_utc(const String& name, SysFileTimeStruct& write);
bool              sys_file_set_last_access_and_write_time_utc(const String& name, SysFileTimeStruct& access, SysFileTimeStruct& write);
void              sys_file_find_files(const String& path, Kyty::Vector<sys_file_find_t>& out);
void              sys_file_get_dents(const String& path, Kyty::Vector<sys_dir_entry_t>& out);
bool              sys_file_copy_file(const String& src, const String& dst);
bool              sys_file_move_file(const String& src, const String& dst);
void              sys_file_remove_readonly(const String& name);

} // namespace Kyty

#endif

#endif /* SYS_LINUX_INCLUDE_KYTY_SYSFILEIO_H_ */
