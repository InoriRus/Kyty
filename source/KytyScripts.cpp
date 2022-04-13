#include "Kyty/Core/Common.h"
#include "Kyty/Core/Core.h"
#include "Kyty/Core/Debug.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Language.h"
#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Core/SDLSubsystem.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Math/MathAll.h"
#include "Kyty/Scripts/BuildTools.h"
#include "Kyty/Scripts/Scripts.h"
#include "Kyty/Sys/SysDbg.h"
#include "Kyty/Sys/SysSync.h"
#include "Kyty/UnitTest.h"

#include "Emulator/Emulator.h"

#include "KytyGitVersion.h"

using namespace Kyty;
using namespace Core;
using namespace Math;
using namespace Scripts;
using namespace Emulator;
using namespace BuildTools;
using namespace UnitTest;

#if KYTY_PLATFORM == KYTY_PLATFORM_ANDROID
#error "can't compile for android"
#endif

static String get_build_string()
{
	Date date = Date::FromMacros(String::FromUtf8(__DATE__));

#if KYTY_BUILD == KYTY_BUILD_DEBUG
	String type = U"Debug";
#elif KYTY_BUILD == KYTY_BUILD_RELEASE
	String type = U"Release";
#else
	String type = U"????";
#endif

#if KYTY_LUA_VER == KYTY_LUA_5_2
	String lua = U"5.2";
#elif KYTY_LUA_VER == KYTY_LUA_5_3
	String lua  = U"5.3";
#else
	String lua  = U"?.?";
#endif

	String compiler = Debug::GetCompiler() + U"-" + Debug::GetLinker() + U"-" + Debug::GetBitness();

	String str = String::FromPrintf("%s, %s, ver = %s, git = %s, lua = %s, date = %s", type.C_Str(), compiler.C_Str(), KYTY_VERSION,
	                                KYTY_GIT_VERSION, lua.C_Str(), date.ToString().C_Str());

	return str;
}

static void run_script(const String& lua_file_name, const StringList& args)
{
	RunString(U"arg = {}");
	FOR (i, args)
	{
		if (i >= 2)
		{
			RunString(String::FromPrintf("arg[%d] = '%s'", i - 1, args.At(i).C_Str()));
		}
	}

	bool   is_file = true;
	String statement;

	if (lua_file_name.StartsWith(U'{') && lua_file_name.EndsWith(U'}'))
	{
		statement = lua_file_name.Mid(1, lua_file_name.Size() - 2);
		is_file   = false;
	}

	ScriptError err = is_file ? RunFile(lua_file_name) : RunString(statement);

	if (err != ScriptError::Ok)
	{
		printf("can't run file %s\n", lua_file_name.utf8_str().GetData());
		switch (err)
		{
			case ScriptError::FileError: printf("file error\n"); break;
			case ScriptError::SyntaxError: printf("syntax error:\n%s\n", GetErrMsg().C_Str()); break;
			case ScriptError::RunError: printf("run error:\n%s\n", GetErrMsg().C_Str()); break;

			case ScriptError::UnknownError:
			default: printf("unknown error\n");
		}
	}
}

int main(int argc, char* argv[])
{
	mem_set_max_size(static_cast<size_t>(2048) * 1024 * 1024 - 1);

	auto& slist = *SubsystemsList::Instance();

	slist.SetArgs(argc, argv);

	auto Scripts  = ScriptsSubsystem::Instance();
	auto Core     = CoreSubsystem::Instance();
	auto UnitTest = UnitTestSubsystem::Instance();
#if KYTY_PROJECT != KYTY_PROJECT_BUILD_TOOLS
	auto Math     = MathSubsystem::Instance();
	auto SDL      = SDLSubsystem::Instance();
	auto Threads  = ThreadsSubsystem::Instance();
	auto Emulator = EmulatorSubsystem::Instance();
#endif
	auto BuildTools = BuildToolsSubsystem::Instance();

	slist.Add(Core, {});
	slist.Add(Scripts, {Core});
#if KYTY_PROJECT != KYTY_PROJECT_BUILD_TOOLS
	slist.Add(Math, {Core});
	slist.Add(SDL, {Core});
	slist.Add(Threads, {Core, SDL});
	slist.Add(Emulator, {Core, Scripts});
#endif
	slist.Add(BuildTools, {Core, Scripts});
	slist.Add(UnitTest, {Core});

	if (!slist.InitAll(false))
	{
		printf("Failed to initialize '%s' subsystem: %s\n", slist.GetFailName(), slist.GetFailMsg());
	} else
	{
		if (argc >= 2)
		{
			StringList args;
			for (int i = 0; i < argc; i++)
			{
				String str = String::FromUtf8(argv[i]);
				if (str.StartsWith(U"'") && str.EndsWith(U"'"))
				{
					// Eclipse bug: https://bugs.eclipse.org/bugs/show_bug.cgi?id=494246
					//				https://bugs.eclipse.org/bugs/show_bug.cgi?id=516027
					str = str.Mid(1, str.Size() - 2);
				}
				args.Add(str);
			}
			run_script(args.At(1), args);
		} else
		{
			printf("%s\n", get_build_string().C_Str());
			printf("fc_script <lua_script> args...\n\n");
			PrintHelp();
		}

		slist.DestroyAll(false);
	}

	return 0;
}
