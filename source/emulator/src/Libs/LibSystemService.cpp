#include "Kyty/Core/Common.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/String.h"

#include "Emulator/Common.h"
#include "Emulator/Libs/Errno.h"
#include "Emulator/Libs/Libs.h"
#include "Emulator/Loader/SymbolDatabase.h"

#ifdef KYTY_EMU_ENABLED

namespace Kyty::Libs {

LIB_VERSION("SystemService", 1, "SystemService", 1, 1);

namespace SystemService {

[[maybe_unused]] constexpr int PARAM_ID_LANG                = 1;
[[maybe_unused]] constexpr int PARAM_ID_DATE_FORMAT         = 2;
[[maybe_unused]] constexpr int PARAM_ID_TIME_FORMAT         = 3;
[[maybe_unused]] constexpr int PARAM_ID_TIME_ZONE           = 4;
[[maybe_unused]] constexpr int PARAM_ID_SUMMERTIME          = 5;
[[maybe_unused]] constexpr int PARAM_ID_SYSTEM_NAME         = 6;
[[maybe_unused]] constexpr int PARAM_ID_GAME_PARENTAL_LEVEL = 7;
[[maybe_unused]] constexpr int PARAM_ID_ENTER_BUTTON_ASSIGN = 1000;

[[maybe_unused]] constexpr int PARAM_LANG_JAPANESE      = 0;
[[maybe_unused]] constexpr int PARAM_LANG_ENGLISH_US    = 1;
[[maybe_unused]] constexpr int PARAM_LANG_FRENCH        = 2;
[[maybe_unused]] constexpr int PARAM_LANG_SPANISH       = 3;
[[maybe_unused]] constexpr int PARAM_LANG_GERMAN        = 4;
[[maybe_unused]] constexpr int PARAM_LANG_ITALIAN       = 5;
[[maybe_unused]] constexpr int PARAM_LANG_DUTCH         = 6;
[[maybe_unused]] constexpr int PARAM_LANG_PORTUGUESE_PT = 7;
[[maybe_unused]] constexpr int PARAM_LANG_RUSSIAN       = 8;
[[maybe_unused]] constexpr int PARAM_LANG_KOREAN        = 9;
[[maybe_unused]] constexpr int PARAM_LANG_CHINESE_T     = 10;
[[maybe_unused]] constexpr int PARAM_LANG_CHINESE_S     = 11;
[[maybe_unused]] constexpr int PARAM_LANG_FINNISH       = 12;
[[maybe_unused]] constexpr int PARAM_LANG_SWEDISH       = 13;
[[maybe_unused]] constexpr int PARAM_LANG_DANISH        = 14;
[[maybe_unused]] constexpr int PARAM_LANG_NORWEGIAN     = 15;
[[maybe_unused]] constexpr int PARAM_LANG_POLISH        = 16;
[[maybe_unused]] constexpr int PARAM_LANG_PORTUGUESE_BR = 17;
[[maybe_unused]] constexpr int PARAM_LANG_ENGLISH_GB    = 18;
[[maybe_unused]] constexpr int PARAM_LANG_TURKISH       = 19;
[[maybe_unused]] constexpr int PARAM_LANG_SPANISH_LA    = 20;
[[maybe_unused]] constexpr int PARAM_LANG_ARABIC        = 21;
[[maybe_unused]] constexpr int PARAM_LANG_FRENCH_CA     = 22;
[[maybe_unused]] constexpr int PARAM_LANG_CZECH         = 23;
[[maybe_unused]] constexpr int PARAM_LANG_HUNGARIAN     = 24;
[[maybe_unused]] constexpr int PARAM_LANG_GREEK         = 25;
[[maybe_unused]] constexpr int PARAM_LANG_ROMANIAN      = 26;
[[maybe_unused]] constexpr int PARAM_LANG_THAI          = 27;
[[maybe_unused]] constexpr int PARAM_LANG_VIETNAMESE    = 28;
[[maybe_unused]] constexpr int PARAM_LANG_INDONESIAN    = 29;

[[maybe_unused]] constexpr int PARAM_DATE_FORMAT_YYYYMMDD = 0;
[[maybe_unused]] constexpr int PARAM_DATE_FORMAT_DDMMYYYY = 1;
[[maybe_unused]] constexpr int PARAM_DATE_FORMAT_MMDDYYYY = 2;

[[maybe_unused]] constexpr int PARAM_TIME_FORMAT_12HOUR = 0;
[[maybe_unused]] constexpr int PARAM_TIME_FORMAT_24HOUR = 1;

[[maybe_unused]] constexpr int MAX_SYSTEM_NAME_LENGTH = 65;

[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_OFF     = 0;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL01 = 1;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL02 = 2;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL03 = 3;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL04 = 4;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL05 = 5;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL06 = 6;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL07 = 7;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL08 = 8;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL09 = 9;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL10 = 10;
[[maybe_unused]] constexpr int PARAM_GAME_PARENTAL_LEVEL11 = 11;

[[maybe_unused]] constexpr int PARAM_ENTER_BUTTON_ASSIGN_CIRCLE = 0;
[[maybe_unused]] constexpr int PARAM_ENTER_BUTTON_ASSIGN_CROSS  = 1;

struct SystemServiceStatus
{
	int32_t event_num                     = 0;
	bool    is_system_ui_overlaid         = false;
	bool    is_in_background_execution    = false;
	bool    is_cpu_mode_7cpu_normal       = true;
	bool    is_game_live_streaming_on_air = false;
	bool    is_out_of_vr_play_area        = false;
};

struct SystemServiceDisplaySafeAreaInfo
{
	float   ratio;
	uint8_t reserved[128];
};

static int KYTY_SYSV_ABI SystemServiceHideSplashScreen()
{
	PRINT_NAME();

	return OK;
}

static int KYTY_SYSV_ABI SystemServiceParamGetInt(int param_id, int* value)
{
	PRINT_NAME();

	if (value == nullptr)
	{
		return SYSTEM_SERVICE_ERROR_PARAMETER;
	}

	int v = 0;

	switch (param_id)
	{
		case PARAM_ID_LANG: v = PARAM_LANG_ENGLISH_US; break;
		case PARAM_ID_DATE_FORMAT: v = PARAM_DATE_FORMAT_DDMMYYYY; break;
		case PARAM_ID_TIME_FORMAT: v = PARAM_TIME_FORMAT_24HOUR; break;
		case PARAM_ID_TIME_ZONE: v = +180; break;
		case PARAM_ID_SUMMERTIME: v = 0; break;
		case PARAM_ID_GAME_PARENTAL_LEVEL: v = PARAM_GAME_PARENTAL_OFF; break;
		case PARAM_ID_ENTER_BUTTON_ASSIGN: v = PARAM_ENTER_BUTTON_ASSIGN_CROSS; break;
		default: EXIT("unknown param_id: %d\n", param_id);
	}

	printf(" %d = %d\n", param_id, v);

	*value = v;

	return OK;
}

static int KYTY_SYSV_ABI SystemServiceGetStatus(SystemServiceStatus* status)
{
	PRINT_NAME();

	if (status == nullptr)
	{
		return SYSTEM_SERVICE_ERROR_PARAMETER;
	}

	*status = SystemServiceStatus();

	return OK;
}

static int KYTY_SYSV_ABI SystemServiceGetDisplaySafeAreaInfo(SystemServiceDisplaySafeAreaInfo* info)
{
	PRINT_NAME();

	if (info == nullptr)
	{
		return SYSTEM_SERVICE_ERROR_PARAMETER;
	}

	info->ratio = 1.0f;

	return OK;
}

} // namespace SystemService

LIB_DEFINE(InitSystemService_1)
{
	LIB_FUNC("Vo5V8KAwCmk", SystemService::SystemServiceHideSplashScreen);
	LIB_FUNC("fZo48un7LK4", SystemService::SystemServiceParamGetInt);
	LIB_FUNC("rPo6tV8D9bM", SystemService::SystemServiceGetStatus);
	LIB_FUNC("1n37q1Bvc5Y", SystemService::SystemServiceGetDisplaySafeAreaInfo);
}

} // namespace Kyty::Libs

#endif // KYTY_EMU_ENABLED
