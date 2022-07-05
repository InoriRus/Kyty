#include "Emulator/Kernel/FileSystem.h"

#include "Kyty/Core/Common.h"
#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Core/Vector.h"

#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"

#include <atomic>
#include <climits>

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs::LibKernel::FileSystem {

LIB_NAME("libkernel", "libkernel");

constexpr int DESCRIPTOR_MIN = 3;

class MountPoints
{
public:
	struct MountPair
	{
		String dir;
		String point;
	};

	MountPoints() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~MountPoints() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(MountPoints);

	void Mount(const String& folder, const String& point);
	void Umount(const String& folder_or_point);

	[[nodiscard]] String GetRealFilename(const String& mounted_file_name);
	[[nodiscard]] String GetRealDirectory(const String& mounted_directory);

private:
	Vector<MountPair> m_mount_pairs;
	Core::Mutex       m_mutex;
};

struct File
{
	Core::File                   f;
	String                       name;
	String                       real_name;
	std::atomic_bool             opened;
	std::atomic_bool             directory;
	Core::Mutex                  mutex;
	Vector<Core::File::DirEntry> dents;
	uint32_t                     dents_index;
};

class FileDescriptors
{
public:
	FileDescriptors() { EXIT_NOT_IMPLEMENTED(!Core::Thread::IsMainThread()); }
	virtual ~FileDescriptors() { KYTY_NOT_IMPLEMENTED; }

	KYTY_CLASS_NO_COPY(FileDescriptors);

	int   CreateDescriptor();
	void  DeleteDescriptor(int d);
	File* GetFile(int d);
	File* GetFile(const String& real_name);
	void  CloseAll();

private:
	Vector<File*> m_files;
	Core::Mutex   m_mutex;
};

static MountPoints*     g_mount_points = nullptr;
static FileDescriptors* g_files        = nullptr;

static void sec_to_timespec(KernelTimespec* ts, double sec)
{
	ts->tv_sec  = static_cast<int64_t>(sec);
	ts->tv_nsec = static_cast<int64_t>((sec - static_cast<double>(ts->tv_sec)) * 1000000000.0);
}

int FileDescriptors::CreateDescriptor()
{
	Core::LockGuard lock(m_mutex);

	auto* file      = new File {};
	file->opened    = false;
	file->directory = false;

	int files_num = static_cast<int>(m_files.Size());
	for (int index = 0; index < files_num; index++)
	{
		if (m_files.At(index) == nullptr)
		{
			m_files[index] = file;
			return index + DESCRIPTOR_MIN;
		}
	}

	m_files.Add(file);
	return static_cast<int>(m_files.Size()) + DESCRIPTOR_MIN - 1;
}

void FileDescriptors::DeleteDescriptor(int d)
{
	Core::LockGuard lock(m_mutex);

	auto index = static_cast<uint32_t>(d - DESCRIPTOR_MIN);

	EXIT_IF(!m_files.IndexValid(index));
	EXIT_IF(m_files.At(index) == nullptr);
	EXIT_IF(m_files.At(index)->opened);

	delete m_files.At(index);
	m_files[index] = nullptr;
}

File* FileDescriptors::GetFile(int d)
{
	Core::LockGuard lock(m_mutex);

	auto index = static_cast<uint32_t>(d - DESCRIPTOR_MIN);

	EXIT_IF(!m_files.IndexValid(index));

	return m_files.At(index);
}

File* FileDescriptors::GetFile(const String& real_name)
{
	Core::LockGuard lock(m_mutex);

	for (auto* f: m_files)
	{
		if (f != nullptr && f->real_name == real_name)
		{
			return f;
		}
	}

	return nullptr;
}

void FileDescriptors::CloseAll()
{
	Core::LockGuard lock(m_mutex);

	for (auto& f: m_files)
	{
		if (f != nullptr && f->opened)
		{
			f->f.Close();
			delete f;
			f = nullptr;
		}
	}
}

void MountPoints::Mount(const String& folder, const String& point)
{
	Core::LockGuard lock(m_mutex);

	auto folder_str = folder.FixDirectorySlash();
	auto point_str  = point.FixDirectorySlash();

	Umount(folder_str);
	Umount(point_str);

	MountPair p;
	p.dir   = folder_str;
	p.point = point_str;

	m_mount_pairs.Add(p);
}

void MountPoints::Umount(const String& folder_or_point)
{
	Core::LockGuard lock(m_mutex);

	auto folder_or_point_str = folder_or_point.FixDirectorySlash();

	if (auto index =
	        m_mount_pairs.Find(folder_or_point_str, [](const MountPair& p, const String& s) { return p.dir == s || p.point == s; });
	    m_mount_pairs.IndexValid(index))
	{
		m_mount_pairs.RemoveAt(index);
	}
}

String MountPoints::GetRealFilename(const String& mounted_file_name)
{
	Core::LockGuard lock(m_mutex);

	auto mounted_path = mounted_file_name.FixFilenameSlash().DirectoryWithoutFilename();

	if (auto index = m_mount_pairs.Find(mounted_path, [](const MountPair& p, const String& s) { return s.StartsWith(p.point); });
	    m_mount_pairs.IndexValid(index))
	{
		const auto& p = m_mount_pairs.At(index);
		return p.dir + mounted_file_name.RemoveFirst(p.point.Size());
	}

	return mounted_file_name;
}

String MountPoints::GetRealDirectory(const String& mounted_directory)
{
	Core::LockGuard lock(m_mutex);

	auto mounted_path = mounted_directory.FixDirectorySlash();

	if (auto index = m_mount_pairs.Find(mounted_path, [](const MountPair& p, const String& s) { return s.StartsWith(p.point); });
	    m_mount_pairs.IndexValid(index))
	{
		const auto& p = m_mount_pairs.At(index);
		return p.dir + mounted_directory.RemoveFirst(p.point.Size());
	}

	return mounted_directory;
}

KYTY_SUBSYSTEM_INIT(FileSystem)
{
	g_mount_points = new MountPoints;
	g_files        = new FileDescriptors;
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(FileSystem)
{
	if (g_files != nullptr)
	{
		g_files->CloseAll();
	}
}

KYTY_SUBSYSTEM_DESTROY(FileSystem)
{
	if (g_files != nullptr)
	{
		g_files->CloseAll();
	}
}

void Mount(const String& folder, const String& point)
{
	EXIT_IF(g_mount_points == nullptr);

	g_mount_points->Mount(folder, point);
}

void Umount(const String& folder_or_point)
{
	EXIT_IF(g_mount_points == nullptr);

	g_mount_points->Umount(folder_or_point);
}

String GetRealFilename(const String& mounted_file_name)
{
	EXIT_IF(g_mount_points == nullptr);

	return g_mount_points->GetRealFilename(mounted_file_name);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int KYTY_SYSV_ABI KernelOpen(const char* path, int flags, uint16_t mode)
{
	PRINT_NAME();

	EXIT_IF(g_mount_points == nullptr || g_files == nullptr);

	if (path == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	auto flags_u = static_cast<uint32_t>(flags);

	printf("\t path = %s\n", path);
	printf("\t flags = %08" PRIx32 "\n", flags_u);
	printf("\t mode = %04" PRIx16 "\n", mode);

	bool nonblock  = (flags_u & 0x0004u) != 0;
	bool append    = (flags_u & 0x0008u) != 0;
	bool fsync     = (flags_u & 0x0080u) != 0;
	bool sync      = (flags_u & 0x0080u) != 0;
	bool creat     = (flags_u & 0x0200u) != 0;
	bool trunc     = (flags_u & 0x0400u) != 0;
	bool excl      = (flags_u & 0x0800u) != 0;
	bool dsync     = (flags_u & 0x1000u) != 0;
	bool direct    = (flags_u & 0x00010000u) != 0;
	bool directory = (flags_u & 0x00020000u) != 0;

	EXIT_NOT_IMPLEMENTED(append || fsync || sync || excl || dsync || direct);

	EXIT_NOT_IMPLEMENTED(nonblock && !directory);

	flags_u &= 0x3u;

	Core::File::Mode rw_mode = Core::File::Mode::Read;

	switch (flags_u)
	{
		case 0: rw_mode = Core::File::Mode::Read; break;
		case 1: rw_mode = Core::File::Mode::Write; break;
		case 2: rw_mode = Core::File::Mode::ReadWrite; break;
		default: EXIT("invalid flag_u: %u\n", flags_u);
	}

	EXIT_NOT_IMPLEMENTED(directory && rw_mode != Core::File::Mode::Read);
	EXIT_NOT_IMPLEMENTED(directory && (trunc || creat));

	int   descriptor = g_files->CreateDescriptor();
	auto* file       = g_files->GetFile(descriptor);

	EXIT_IF(file == nullptr || file->opened || file->directory);

	file->name      = path;
	file->real_name = (directory ? g_mount_points->GetRealDirectory(file->name) : g_mount_points->GetRealFilename(file->name));

	if (trunc && rw_mode == Core::File::Mode::Read)
	{
		return KERNEL_ERROR_EACCES;
	}

	bool dir_exist = Core::File::IsDirectoryExisting(file->real_name);

	if (directory || dir_exist)
	{
		if (!dir_exist)
		{
			g_files->DeleteDescriptor(descriptor);
			return KERNEL_ERROR_ENOTDIR;
		}

		EXIT_NOT_IMPLEMENTED(!directory && rw_mode != Core::File::Mode::Read);
		EXIT_NOT_IMPLEMENTED(!directory && (trunc || creat));

		file->dents       = Core::File::GetDirEntries(file->real_name);
		file->dents_index = 0;
		file->directory   = true;

		printf("\tOpen dir: " FG_WHITE BOLD "%s" DEFAULT ", entries = %" PRIu32 ", " FG_GREEN "[ok]" FG_DEFAULT "\n",
		       file->real_name.C_Str(), file->dents.Size());

		for (const auto& f: file->dents)
		{
			printf("\t\t%s %s\n", f.is_file ? "[file]" : "[dir ]", f.name.C_Str());
		}
	} else
	{
		bool result = false;

		if (creat)
		{
			result = file->f.Create(file->real_name);

			printf("\tCreate: " FG_WHITE BOLD "%s" DEFAULT ", %s\n", file->real_name.C_Str(),
			       (result ? FG_GREEN "[ok]" FG_DEFAULT : FG_RED "[fail]" FG_DEFAULT));
		} else
		{
			result = file->f.Open(file->real_name, rw_mode);

			printf("\tOpen: " FG_WHITE BOLD "%s" DEFAULT ", %s\n", file->real_name.C_Str(),
			       (result ? FG_GREEN "[ok]" FG_DEFAULT : FG_RED "[fail]" FG_DEFAULT));
		}

		EXIT_NOT_IMPLEMENTED(creat && !trunc);

		if (result && trunc)
		{
			result = file->f.Truncate(0);
		}

		if (!result || file->f.IsInvalid())
		{
			g_files->DeleteDescriptor(descriptor);
			return KERNEL_ERROR_EACCES;
		}
	}

	file->opened = true;
	return descriptor;
}

int KYTY_SYSV_ABI KernelClose(int d)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_IF(!file->opened);

	if (!file->directory)
	{
		file->f.Close();
	}

	file->opened = false;

	printf("\tClose: " FG_WHITE BOLD "%s" DEFAULT "\n", file->real_name.C_Str());

	g_files->DeleteDescriptor(d);

	return OK;
}

int64_t KYTY_SYSV_ABI KernelRead(int d, void* buf, size_t nbytes)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (buf == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_NOT_IMPLEMENTED(file->directory);

	EXIT_IF(!file->opened);

	EXIT_NOT_IMPLEMENTED(nbytes > UINT_MAX);

	file->mutex.Lock();

	bool     is_invalid = file->f.IsInvalid();
	uint32_t bytes_read = 0;
	file->f.Read(buf, static_cast<uint32_t>(nbytes), &bytes_read);

	file->mutex.Unlock();

	if (is_invalid)
	{
		printf("\tfile is invalid\n");
		return KERNEL_ERROR_EIO;
	}

	printf("\tRead %u bytes from: " FG_WHITE BOLD "%s" DEFAULT "\n", bytes_read, file->real_name.C_Str());

	return bytes_read;
}

int64_t KYTY_SYSV_ABI KernelWrite(int d, const void* buf, size_t nbytes)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (buf == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_NOT_IMPLEMENTED(file->directory);

	EXIT_IF(!file->opened);

	EXIT_NOT_IMPLEMENTED(nbytes > UINT_MAX);

	file->mutex.Lock();

	bool     is_invalid    = file->f.IsInvalid();
	uint32_t bytes_written = 0;
	file->f.Write(buf, static_cast<uint32_t>(nbytes), &bytes_written);

	file->mutex.Unlock();

	if (is_invalid)
	{
		printf("\tfile is invalid\n");
		return KERNEL_ERROR_EIO;
	}

	printf("\tWrite %u bytes to: " FG_WHITE BOLD "%s" DEFAULT "\n", bytes_written, file->real_name.C_Str());

	return bytes_written;
}

int64_t KYTY_SYSV_ABI KernelPread(int d, void* buf, size_t nbytes, int64_t offset)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (buf == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	if (offset < 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_NOT_IMPLEMENTED(file->directory);

	EXIT_IF(!file->opened);

	EXIT_NOT_IMPLEMENTED(nbytes > UINT_MAX);

	file->mutex.Lock();

	bool     is_invalid = file->f.IsInvalid();
	uint32_t bytes_read = 0;
	if(file->f.Tell() != offset){
		file->f.Seek(offset);
	}

	file->f.Read(buf, static_cast<uint32_t>(nbytes), &bytes_read);

	file->mutex.Unlock();

	if (is_invalid)
	{
		printf("\tfile is invalid\n");
		return KERNEL_ERROR_EIO;
	}

	printf("\tRead %u bytes (pos = %" PRId64 ") from: " FG_WHITE BOLD "%s" DEFAULT "\n", bytes_read, offset, file->real_name.C_Str());

	return bytes_read;
}

int64_t KYTY_SYSV_ABI KernelPwrite(int d, const void* buf, size_t nbytes, int64_t offset)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (buf == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	if (offset < 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_NOT_IMPLEMENTED(file->directory);

	EXIT_IF(!file->opened);

	EXIT_NOT_IMPLEMENTED(nbytes > UINT_MAX);

	file->mutex.Lock();

	bool     is_invalid    = file->f.IsInvalid();
	auto     pos           = file->f.Tell();
	uint32_t bytes_written = 0;
	file->f.Seek(offset);
	file->f.Write(buf, static_cast<uint32_t>(nbytes), &bytes_written);
	file->f.Seek(pos);

	file->mutex.Unlock();

	if (is_invalid)
	{
		printf("\tfile is invalid\n");
		return KERNEL_ERROR_EIO;
	}

	printf("\tWrite %u bytes (pos = %" PRId64 ") to: " FG_WHITE BOLD "%s" DEFAULT "\n", bytes_written, offset, file->real_name.C_Str());

	return bytes_written;
}

int64_t KYTY_SYSV_ABI KernelLseek(int d, int64_t offset, int whence)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_NOT_IMPLEMENTED(file->directory);

	EXIT_IF(!file->opened);

	file->mutex.Lock();

	bool is_invalid = file->f.IsInvalid();

	if (whence == 1)
	{
		offset = static_cast<int64_t>(file->f.Tell()) + offset;
		whence = 0;
	}

	if (whence == 2)
	{
		offset = static_cast<int64_t>(file->f.Size()) + offset;
		whence = 0;
	}

	EXIT_NOT_IMPLEMENTED(whence != 0);

	if (offset < 0)
	{
		return KERNEL_ERROR_EINVAL;
	}

	file->f.Seek(offset);
	auto pos = static_cast<int64_t>(file->f.Tell());

	EXIT_IF(pos != offset);

	file->mutex.Unlock();

	if (is_invalid)
	{
		printf("\tfile is invalid\n");
		return KERNEL_ERROR_EIO;
	}

	printf("\tLseek (pos = %" PRId64 ") to: " FG_WHITE BOLD "%s" DEFAULT "\n", offset, file->real_name.C_Str());

	return pos;
}

int KYTY_SYSV_ABI KernelStat(const char* path, FileStat* sb)
{
	PRINT_NAME();

	EXIT_IF(g_mount_points == nullptr);

	if (path == nullptr || sb == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	printf("\t KernelStat: %s\n", path);

	String path_s         = String::FromUtf8(path);
	auto   real_file_name = g_mount_points->GetRealFilename(path_s);
	auto   real_directory = g_mount_points->GetRealDirectory(path_s);

	bool is_dir  = Core::File::IsDirectoryExisting(real_file_name) || Core::File::IsDirectoryExisting(real_directory);
	bool is_file = Core::File::IsFileExisting(real_file_name);

	if (!is_dir && !is_file)
	{
		printf("\t file not found\n");
		return KERNEL_ERROR_ENOENT;
	}

	EXIT_NOT_IMPLEMENTED(is_dir && is_file);

	memset(sb, 0, sizeof(FileStat));

	sb->st_mode = 0000777u | (is_dir ? 0040000u : 0100000u);

	Core::DateTime at;
	Core::DateTime wt;

	if (is_dir)
	{
		sb->st_size    = 0;
		sb->st_blksize = 512;
		sb->st_blocks  = 0;
	} else
	{
		sb->st_size    = static_cast<int64_t>(Core::File::Size(real_file_name));
		sb->st_blksize = 512;
		sb->st_blocks  = (sb->st_size + 511) / 512;

		Core::File::GetLastAccessAndWriteTimeUTC(real_file_name, &at, &wt);
	}

	sec_to_timespec(&sb->st_atim, at.ToUnix());
	sec_to_timespec(&sb->st_mtim, wt.ToUnix());
	sb->st_ctim     = sb->st_atim;
	sb->st_birthtim = sb->st_mtim;

	return OK;
}

int KYTY_SYSV_ABI KernelFstat(int d, FileStat* sb)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (d < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (sb == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	auto* file = g_files->GetFile(d);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	EXIT_IF(!file->opened);

	printf("\tKernelFstat: %s\n", file->real_name.C_Str());

	memset(sb, 0, sizeof(FileStat));

	sb->st_mode = 0000777u | (file->directory ? 0040000u : 0100000u);

	Core::DateTime at;
	Core::DateTime wt;

	if (!file->directory)
	{
		file->mutex.Lock();

		bool is_invalid = file->f.IsInvalid();
		auto size       = file->f.Size();
		file->f.GetLastAccessAndWriteTimeUTC(&at, &wt);

		file->mutex.Unlock();

		if (is_invalid)
		{
			printf("\tfile is invalid\n");
			return KERNEL_ERROR_EIO;
		}

		sb->st_size    = static_cast<int64_t>(size);
		sb->st_blksize = 512;
		sb->st_blocks  = (sb->st_size + 511) / 512;
	} else
	{
		sb->st_size    = 0;
		sb->st_blksize = 512;
		sb->st_blocks  = 0;
	}

	sec_to_timespec(&sb->st_atim, at.ToUnix());
	sec_to_timespec(&sb->st_mtim, wt.ToUnix());
	sb->st_ctim     = sb->st_atim;
	sb->st_birthtim = sb->st_mtim;

	return OK;
}

int KYTY_SYSV_ABI KernelUnlink(const char* path)
{
	PRINT_NAME();

	EXIT_IF(g_mount_points == nullptr);
	EXIT_IF(g_files == nullptr);

	if (path == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	auto path_s         = String::FromUtf8(path);
	auto real_file_name = g_mount_points->GetRealFilename(path_s);
	auto real_directory = g_mount_points->GetRealDirectory(path_s);

	EXIT_NOT_IMPLEMENTED(g_files->GetFile(real_file_name) != nullptr);
	EXIT_NOT_IMPLEMENTED(g_files->GetFile(real_directory) != nullptr);

	bool is_dir  = Core::File::IsDirectoryExisting(real_file_name) || Core::File::IsDirectoryExisting(real_directory);
	bool is_file = Core::File::IsFileExisting(real_file_name);

	if (is_dir)
	{
		return KERNEL_ERROR_EPERM;
	}

	if (!is_file)
	{
		return KERNEL_ERROR_ENOENT;
	}

	bool ok = Core::File::DeleteFile(real_file_name);

	if (!ok)
	{
		return KERNEL_ERROR_EIO;
	}

	printf("\tKernelUnlink: %s\n", path);

	return OK;
}

int KYTY_SYSV_ABI KernelGetdirentries(int fd, char* buf, int nbytes, int64_t* basep)
{
	PRINT_NAME();

	EXIT_IF(g_files == nullptr);

	if (fd < DESCRIPTOR_MIN)
	{
		return KERNEL_ERROR_EBADF;
	}

	if (buf == nullptr)
	{
		return KERNEL_ERROR_EFAULT;
	}

	auto* file = g_files->GetFile(fd);

	if (file == nullptr)
	{
		return KERNEL_ERROR_EBADF;
	}

	if (!file->directory || nbytes < 512 || file->dents_index > file->dents.Size())
	{
		return KERNEL_ERROR_EINVAL;
	}

	EXIT_IF(!file->opened);

	printf("\t dir    = %s\n", file->real_name.C_Str());
	printf("\t nbytes = %d\n", nbytes);
	printf("\t index = %d\n", file->dents_index);

	if (basep != nullptr)
	{
		*basep = file->dents_index;
	}

	if (file->dents_index == file->dents.Size())
	{
		return 0;
	}

	const auto& entry = file->dents.At(file->dents_index++);

	auto str      = entry.name.utf8_str();
	auto str_size = str.Size() - 1;
	EXIT_NOT_IMPLEMENTED(str_size > 255);

	printf("\t name  = %s\n", str.GetDataConst());

	*reinterpret_cast<uint32_t*>(buf + 0) = entry.name.Hash();
	*reinterpret_cast<uint16_t*>(buf + 4) = 512;
	*reinterpret_cast<uint8_t*>(buf + 6)  = (entry.is_file ? 8 : 4);
	*reinterpret_cast<uint8_t*>(buf + 7)  = static_cast<uint8_t>(str_size);
	strncpy(buf + 8, str.GetDataConst(), 255);
	buf[8 + 255] = '\0';

	return 512;
}

int KYTY_SYSV_ABI KernelGetdents(int fd, char* buf, int nbytes)
{
	PRINT_NAME();

	return KernelGetdirentries(fd, buf, nbytes, nullptr);
}

int KYTY_SYSV_ABI KernelMkdir(const char* path, uint16_t mode)
{
	PRINT_NAME();

	EXIT_IF(g_mount_points == nullptr || g_files == nullptr);

	if (path == nullptr)
	{
		return KERNEL_ERROR_EINVAL;
	}

	printf("\t path = %s\n", path);
	printf("\t mode = %04" PRIx16 "\n", mode);

	String real_name = g_mount_points->GetRealDirectory(String::FromUtf8(path));

	if (Core::File::IsDirectoryExisting(real_name))
	{
		return KERNEL_ERROR_EEXIST;
	}

	if (!Core::File::CreateDirectory(real_name))
	{
		return KERNEL_ERROR_EIO;
	}

	if (!Core::File::IsDirectoryExisting(real_name))
	{
		return KERNEL_ERROR_ENOENT;
	}

	return OK;
}

} // namespace Kyty::Libs::LibKernel::FileSystem

#endif // KYTY_EMU_ENABLED
