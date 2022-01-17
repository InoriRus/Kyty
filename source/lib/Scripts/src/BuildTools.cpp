#include "Kyty/Scripts/BuildTools.h"

#include "Kyty/Core/ByteBuffer.h"
#include "Kyty/Core/Common.h"
#include "Kyty/Core/Database.h"
#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Debug.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/String.h"
#include "Kyty/Scripts/Scripts.h"

namespace Kyty::BuildTools {

KYTY_SCRIPT_FUNC(call_func)
{

	return 0;
}

void call_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(rd_func)
{

	return 0;
}

void rd_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(copy_func)
{
	String src;
	String dst;

	if (Scripts::ArgGetVarCount() == 2)
	{
		src = Scripts::ArgGetVar(0).ToString();
		dst = Scripts::ArgGetVar(1).ToString();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::File::CreateDirectories(dst.DirectoryWithoutFilename());
	Core::File::CopyFile(src, dst);

	return 0;
}

void copy_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(move_func)
{
	String src;
	String dst;

	if (Scripts::ArgGetVarCount() == 2)
	{
		src = Scripts::ArgGetVar(0).ToString();
		dst = Scripts::ArgGetVar(1).ToString();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::File::CreateDirectories(dst.DirectoryWithoutFilename());
	if (!Core::File::MoveFile(src, dst))
	{
		EXIT("move\n");
	}

	return 0;
}

void move_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(sync_func)
{
	String src;
	String dst;
	int    del = 1;

	if (Scripts::ArgGetVarCount() == 2)
	{
		src = Scripts::ArgGetVar(0).ToString();
		dst = Scripts::ArgGetVar(1).ToString();
	} else if (Scripts::ArgGetVarCount() == 3)
	{
		src = Scripts::ArgGetVar(0).ToString();
		dst = Scripts::ArgGetVar(1).ToString();
		del = Scripts::ArgGetVar(1).ToInteger();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::File::SyncDirectories(src, dst, del != 0);

	return 0;
}

void sync_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(str_to_file_func)
{
	String str;
	String dst;

	if (Scripts::ArgGetVarCount() == 2)
	{
		str = Scripts::ArgGetVar(0).ToString();
		dst = Scripts::ArgGetVar(1).ToString();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::File f;
	f.Create(dst);
	f.SetEncoding(Core::File::Encoding::Utf8);

	if (!f.IsInvalid())
	{
		f.Write(str);
	}

	f.Close();

	return 0;
}

void str_to_file_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(replace_str_func)
{
	String str1;
	String str2;
	String dst;

	if (Scripts::ArgGetVarCount() == 3)
	{
		str1 = Scripts::ArgGetVar(0).ToString();
		str2 = Scripts::ArgGetVar(1).ToString();
		dst  = Scripts::ArgGetVar(2).ToString();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::DateTime at;
	Core::DateTime wt;
	Core::File::GetLastAccessAndWriteTimeUTC(dst, &at, &wt);

	Core::File f;
	f.Open(dst, Core::File::Mode::Read);
	f.SetEncoding(Core::File::Encoding::Utf8);

	bool ok = false;

	if (!f.IsInvalid())
	{
		String s = f.ReadWholeString();
		s        = s.ReplaceStr(str1, str2, String::Case::Insensitive);

		f.Close();
		f.Create(dst);
		f.SetEncoding(Core::File::Encoding::Utf8);

		if (!f.IsInvalid())
		{
			f.Write(s);
			ok = true;
		}
	}

	f.Close();

	if (ok)
	{
		Core::File::SetLastAccessAndWriteTimeUTC(dst, at, wt);
	}

	return 0;
}

void replace_str_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(map_to_csv_func)
{
	String mode;
	String src;
	String dst;

	if (Scripts::ArgGetVarCount() == 3)
	{
		mode = Scripts::ArgGetVar(0).ToString();
		src  = Scripts::ArgGetVar(1).ToString();
		dst  = Scripts::ArgGetVar(2).ToString();
	} else
	{
		EXIT("invalid args\n");
	}

	Core::DebugMap map;

	if (Core::File::IsFileExisting(src))
	{
		Core::File::DeleteFile(dst);

		printf("[ map] %s: %s -> %s\n", mode.C_Str(), src.C_Str(), dst.C_Str());

		if (mode.EqualNoCase(U"mingw_ld_32") || mode.EqualNoCase(U"clang_ld_32"))
		{
			map.LoadGnuLd(src, 32);
			map.DumpMap(dst);
		} else if (mode.EqualNoCase(U"mingw_ld_64") || mode.EqualNoCase(U"clang_ld_64"))
		{
			map.LoadGnuLd(src, 64);
			map.DumpMap(dst);
		} else if (mode.EqualNoCase(U"clang_lld_64"))
		{
			map.LoadLlvmLld(src, 64);
			map.DumpMap(dst);
		} else if (mode.EqualNoCase(U"msvc_link_32"))
		{
			map.LoadMsvcLink(src, 32);
			map.DumpMap(dst);
		} else if (mode.EqualNoCase(U"msvc_link_64"))
		{
			map.LoadMsvcLink(src, 64);
			map.DumpMap(dst);
		} else if (mode.EqualNoCase(U"clang_lld_link_64"))
		{
			map.LoadMsvcLldLink(src, 64);
			map.DumpMap(dst);
		} else
		{
			printf("unknown map: %s\n", mode.C_Str());
		}
	}

	return 0;
}

void map_to_csv_help()
{
	// TODO(#108)
}

void atlas_repack_help()
{
	// TODO(#108)
}

KYTY_SCRIPT_FUNC(db_key_func)
{
	if (Scripts::ArgGetVarCount() != 3)
	{
		EXIT("invalid args\n");
	}

	String database = Scripts::ArgGetVar(0).ToString();
	String password = Scripts::ArgGetVar(1).ToString();
	int    legacy   = Scripts::ArgGetVar(2).ToInteger();

	Core::Database::Connection db;
	db.Open(database, Core::Database::Connection::Mode::ReadOnly);

	if (db.IsInvalid() || db.IsError())
	{
		EXIT("Can't open file: %s\n", database.C_Str());
	}

	db.SetPassword(password, legacy);
	auto key = db.GetKey();

	FOR (i, key)
	{
		printf("%s0x%02" PRIx8 "%s", (i > 0 ? ", " : "{ "), std::to_integer<uint8_t>(key.At(i)), (i == key_size_ - 1 ? " }\n" : ""));
	}

	db.Close();

	return 0;
}

void db_key_help()
{
	// TODO(#108)
}

void Init()
{
	Scripts::RegisterFunc("sync", BuildTools::sync_func, BuildTools::sync_help);
	Scripts::RegisterFunc("copy", BuildTools::copy_func, BuildTools::copy_help);
	Scripts::RegisterFunc("map_to_csv", BuildTools::map_to_csv_func, BuildTools::map_to_csv_help);
	Scripts::RegisterFunc("str_to_file", BuildTools::str_to_file_func, BuildTools::str_to_file_help);
	Scripts::RegisterFunc("replace_str", BuildTools::replace_str_func, BuildTools::replace_str_help);
	Scripts::RegisterFunc("db_key", BuildTools::db_key_func, BuildTools::db_key_help);
}

KYTY_SUBSYSTEM_INIT(BuildTools)
{
	BuildTools::Init();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(BuildTools) {}

KYTY_SUBSYSTEM_DESTROY(BuildTools) {}

} // namespace Kyty::BuildTools
