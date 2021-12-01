#include "Kyty/Scripts/ScriptsLoader.h"

#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Scripts/Scripts.h"

namespace Kyty::Scripts {

static String* g_load_err = nullptr;

void SetLoadError(const String& err)
{
	Core::mem_tracker_disable();

	if (g_load_err == nullptr)
	{
		g_load_err = new String;
	}

	String s = err.IsEmpty() ? U"" : String::FromPrintf("%s", err.C_Str());

	if (g_load_err->IsEmpty())
	{
		*g_load_err = s;
	} else
	{
		*g_load_err = *g_load_err + U"\n" + s;
	}

	Core::mem_tracker_enable();
}

void ResetLoadError()
{
	Core::mem_tracker_disable();

	if (g_load_err == nullptr)
	{
		g_load_err = new String;
	}

	*g_load_err = U"";

	Core::mem_tracker_enable();
}

String GetLoadError()
{
	return ((g_load_err != nullptr) && !g_load_err->IsEmpty()) ? String::FromPrintf("%s", (*g_load_err).C_Str()) : U"";
}

bool RunScript(const String& lua_file_name)
{
	Scripts::ScriptError err = Scripts::RunFile(lua_file_name);
	if (err != Scripts::ScriptError::Ok)
	{
		String err_str = String::FromPrintf("can't run file %s\n", lua_file_name.C_Str());
		switch (err)
		{
			case Scripts::ScriptError::FileError: err_str += String::FromPrintf("file error\n"); break;
			case Scripts::ScriptError::SyntaxError:
				err_str += String::FromPrintf("syntax error:\n%s\n", Scripts::GetErrMsg().C_Str());
				break;
			case Scripts::ScriptError::RunError:
				err_str += String::FromPrintf("run error:\n%s\n", Scripts::GetErrMsg().C_Str());
				break;

			case Scripts::ScriptError::UnknownError:
			default: err_str += U"unknown error\n";
		}

		SetLoadError(err_str);
		return false;
	}
	return true;
}

} // namespace Kyty::Scripts
