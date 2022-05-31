#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Kernel/FileSystem.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("SaveData", 1, "SaveData", 1, 1);

namespace SaveData {

// TODO(): specify dir at launcher
static constexpr char32_t SAVE_DATA_DIR[]   = U"_SaveData";
static constexpr char32_t SAVE_DATA_POINT[] = U"/savedata0";

struct SceSaveDataDirName
{
	char data[32];
};

struct SaveDataMountPoint
{
	char data[16];
};

struct SaveDataMount
{
	int         user_id;
	int         pad;
	const char* title_id;
	const char* dir_name;
	const char* fingerprint;
	uint64_t    blocks;
	uint32_t    mount_mode;
	uint8_t     reserved[32];
};

struct SaveDataMount2
{
	int                       user_id;
	int                       pad;
	const SceSaveDataDirName* dir_name;
	uint64_t                  blocks;
	uint32_t                  mount_mode;
	uint8_t                   reserved[32];
	int                       pad2;
};

struct SaveDataMountResult
{
	SaveDataMountPoint mount_point;
	uint64_t           required_blocks;
	uint32_t           unused;
	uint32_t           mount_status;
	uint8_t            reserved[28];
	int                pad;
};

struct SaveDataParam
{
	char     title[128];
	char     sub_title[128];
	char     detail[1024];
	uint32_t user_param;
	int      pad;
	int64_t  mtime;
	uint8_t  reserved[32];
};

struct SaveDataMountInfo
{
	uint64_t blocks;
	uint64_t free_blocks;
	uint8_t  reserved[32];
};

struct SaveDataIcon
{
	void*   buf;
	size_t  buf_size;
	size_t  data_size;
	uint8_t reserved[32];
};

int KYTY_SYSV_ABI SaveDataInitialize(const void* /*init*/)
{
	PRINT_NAME();

	// EXIT_NOT_IMPLEMENTED(init != nullptr);

	return OK;
}

int KYTY_SYSV_ABI SaveDataInitialize2(const void* /*init*/)
{
	PRINT_NAME();

	// EXIT_NOT_IMPLEMENTED(init != nullptr);

	return OK;
}

int KYTY_SYSV_ABI SaveDataInitialize3(const void* /*init*/)
{
	PRINT_NAME();

	// EXIT_NOT_IMPLEMENTED(init != nullptr);

	return OK;
}

int KYTY_SYSV_ABI SaveDataMount(const SaveDataMount* mount, SaveDataMountResult* mount_result)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount == nullptr);
	EXIT_NOT_IMPLEMENTED(mount_result == nullptr);

	printf("\t user_id     = %d\n", mount->user_id);
	printf("\t title_id    = %s\n", mount->title_id);
	printf("\t dir_name    = %s\n", mount->dir_name);
	printf("\t fingerprint = %s\n", mount->fingerprint);
	printf("\t blocks      = %" PRIu64 "\n", mount->blocks);
	printf("\t mount_mode  = %" PRIu32 "\n", mount->mount_mode);

	String mount_dir   = String(SAVE_DATA_DIR) + U"/" + String::FromUtf8(mount->dir_name);
	String mount_point = SAVE_DATA_POINT;
	bool   create      = (mount->mount_mode == 6 || mount->mount_mode == 22);
	bool   open        = (mount->mount_mode == 1 || mount->mount_mode == 2);

	if (!create && !open)
	{
		EXIT("unknown mount mode: %u", mount->mount_mode);
	}

	if (open)
	{
		EXIT_NOT_IMPLEMENTED(create);

		if (!Core::File::IsDirectoryExisting(mount_dir))
		{
			return SAVE_DATA_ERROR_NOT_FOUND;
		}

		LibKernel::FileSystem::Mount(mount_dir, mount_point);

		mount_result->mount_status = 0;
	}

	if (create)
	{
		EXIT_NOT_IMPLEMENTED(open);

		if (Core::File::IsDirectoryExisting(mount_dir))
		{
			return SAVE_DATA_ERROR_EXISTS;
		}

		Core::File::CreateDirectories(mount_dir);

		EXIT_NOT_IMPLEMENTED((!Core::File::IsDirectoryExisting(mount_dir)));

		LibKernel::FileSystem::Mount(mount_dir, mount_point);

		mount_result->mount_status = 1;
	}

	int s = snprintf(mount_result->mount_point.data, 16, "%s", mount_point.C_Str());

	EXIT_NOT_IMPLEMENTED(s >= 16);

	mount_result->required_blocks = 0;

	return OK;
}

int KYTY_SYSV_ABI SaveDataMount2(const SaveDataMount2* mount, SaveDataMountResult* mount_result)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount == nullptr);
	EXIT_NOT_IMPLEMENTED(mount_result == nullptr);

	printf("\t user_id    = %d\n", mount->user_id);
	printf("\t dir_name   = %s\n", mount->dir_name->data);
	printf("\t blocks     = %" PRIu64 "\n", mount->blocks);
	printf("\t mount_mode = %" PRIu32 "\n", mount->mount_mode);

	String mount_dir   = String(SAVE_DATA_DIR) + U"/" + String::FromUtf8(mount->dir_name->data);
	String mount_point = SAVE_DATA_POINT;
	bool   create      = (mount->mount_mode == 6 || mount->mount_mode == 22);
	bool   open        = (mount->mount_mode == 1 || mount->mount_mode == 2);

	if (!create && !open)
	{
		EXIT("unknown mount mode: %u", mount->mount_mode);
	}

	if (open)
	{
		EXIT_NOT_IMPLEMENTED(create);

		if (!Core::File::IsDirectoryExisting(mount_dir))
		{
			return SAVE_DATA_ERROR_NOT_FOUND;
		}

		LibKernel::FileSystem::Mount(mount_dir, mount_point);

		mount_result->mount_status = 0;
	}

	if (create)
	{
		EXIT_NOT_IMPLEMENTED(open);

		if (Core::File::IsDirectoryExisting(mount_dir))
		{
			return SAVE_DATA_ERROR_EXISTS;
		}

		Core::File::CreateDirectories(mount_dir);

		EXIT_NOT_IMPLEMENTED((!Core::File::IsDirectoryExisting(mount_dir)));

		LibKernel::FileSystem::Mount(mount_dir, mount_point);

		mount_result->mount_status = 1;
	}

	int s = snprintf(mount_result->mount_point.data, 16, "%s", mount_point.C_Str());

	EXIT_NOT_IMPLEMENTED(s >= 16);

	mount_result->required_blocks = 0;

	return OK;
}

int KYTY_SYSV_ABI SaveDataUmount(const SaveDataMountPoint* mount_point)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount_point == nullptr);

	printf("\t mount_point = %s\n", mount_point->data);

	LibKernel::FileSystem::Umount(String::FromUtf8(mount_point->data));

	return OK;
}

int KYTY_SYSV_ABI SaveDataSetParam(const SaveDataMountPoint* mount_point, uint32_t param_type, const void* param_buf, size_t param_buf_size)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount_point == nullptr);

	printf("\t mount_point    = %s\n", mount_point->data);
	printf("\t param_type     = %u\n", param_type);
	printf("\t param_buf_size = %" PRIu64 "\n", param_buf_size);

	if (param_type == 0)
	{
		const auto* p = static_cast<const SaveDataParam*>(param_buf);

		printf("\t title      = %s\n", p->title);
		printf("\t sub_title  = %s\n", p->sub_title);
		printf("\t detail     = %s\n", p->detail);
		printf("\t user_param = %u\n", p->user_param);
	} else
	{
		KYTY_NOT_IMPLEMENTED;
	}

	return OK;
}

int KYTY_SYSV_ABI SaveDataGetMountInfo(const SaveDataMountPoint* mount_point, SaveDataMountInfo* info)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount_point == nullptr);
	EXIT_NOT_IMPLEMENTED(info == nullptr);

	info->blocks      = 100000;
	info->free_blocks = 100000;

	return OK;
}

int KYTY_SYSV_ABI SaveDataSaveIcon(const SaveDataMountPoint* mount_point, const SaveDataIcon* icon)
{
	EXIT_NOT_IMPLEMENTED(mount_point == nullptr);
	EXIT_NOT_IMPLEMENTED(icon == nullptr);

	printf("\t buf       = %016" PRIx64 "\n", reinterpret_cast<uint64_t>(icon->buf));
	printf("\t buf_size  = %" PRIu64 "\n", icon->buf_size);
	printf("\t data_size = %" PRIu64 "\n", icon->data_size);

	return OK;
}

} // namespace SaveData

LIB_DEFINE(InitSaveData_1)
{
	LIB_FUNC("ZkZhskCPXFw", SaveData::SaveDataInitialize);
	LIB_FUNC("l1NmDeDpNGU", SaveData::SaveDataInitialize2);
	LIB_FUNC("TywrFKCoLGY", SaveData::SaveDataInitialize3);
	LIB_FUNC("32HQAQdwM2o", SaveData::SaveDataMount);
	LIB_FUNC("0z45PIH+SNI", SaveData::SaveDataMount2);
	LIB_FUNC("BMR4F-Uek3E", SaveData::SaveDataUmount);
	LIB_FUNC("85zul--eGXs", SaveData::SaveDataSetParam);
	LIB_FUNC("65VH0Qaaz6s", SaveData::SaveDataGetMountInfo);
	LIB_FUNC("c88Yy54Mx0w", SaveData::SaveDataSaveIcon);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
