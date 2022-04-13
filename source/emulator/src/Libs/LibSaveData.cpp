#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("SaveData", 1, "SaveData", 1, 1);

namespace SaveData {

struct SceSaveDataDirName
{
	char data[32];
};

struct SaveDataMountPoint
{
	char data[16];
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

int KYTY_SYSV_ABI SaveDataMount2(const SaveDataMount2* mount, SaveDataMountResult* mount_result)
{
	PRINT_NAME();

	EXIT_NOT_IMPLEMENTED(mount == nullptr);
	EXIT_NOT_IMPLEMENTED(mount_result == nullptr);

	printf("\t user_id    = %d\n", mount->user_id);
	printf("\t dir_name   = %s\n", mount->dir_name->data);
	printf("\t blocks     = %" PRIu64 "\n", mount->blocks);
	printf("\t mount_mode = %" PRIu32 "\n", mount->mount_mode);

	if (mount->mount_mode == 1)
	{
		return SAVE_DATA_ERROR_NOT_FOUND;
	} else // NOLINT
	{
		EXIT("unknown mount mode: %u", mount->mount_mode);
	}

	strcpy(mount_result->mount_point.data, "/savedata0");

	mount_result->required_blocks = 0;
	mount_result->mount_status    = 1;

	return OK;
}

} // namespace SaveData

LIB_DEFINE(InitSaveData_1)
{
	LIB_FUNC("ZkZhskCPXFw", SaveData::SaveDataInitialize);
	LIB_FUNC("l1NmDeDpNGU", SaveData::SaveDataInitialize2);
	LIB_FUNC("TywrFKCoLGY", SaveData::SaveDataInitialize3);
	LIB_FUNC("0z45PIH+SNI", SaveData::SaveDataMount2);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
