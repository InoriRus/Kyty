#include "Kyty/Core/String.h"
#include "Kyty/Scripts/LuaCpp.h"
#include "Kyty/Scripts/Scripts.h"

namespace Kyty::Scripts {

// void script_dbg_dump_stack(const String &prefix);

static String create_error(ScriptError err, const String& msg1, const String& msg2)
{
	String err_str = String::FromPrintf("%s %s\n", msg1.C_Str(), msg2.C_Str());

	switch (err)
	{
		case ScriptError::FileError: err_str += String::FromPrintf("file error\n"); break;
		case ScriptError::SyntaxError: err_str += String::FromPrintf("syntax error:\n%s\n", GetErrMsg().C_Str()); break;
		case ScriptError::RunError: err_str += String::FromPrintf("run error:\n%s\n", GetErrMsg().C_Str()); break;

		case ScriptError::UnknownError:
		default: err_str += String::FromPrintf("unknown error\n");
	}

	return err_str;
}

KYTY_STATIC_SCRIPT_FUNC(dofile)
{
	KYTY_SCRIPT_FUNC_BEGIN();

	if (Scripts::ArgGetVarCount() != 1)
	{
		KYTY_SCRIPT_THROW_ERROR(U"invalid args\n");
	}

	String file_name = Scripts::ArgGetVar(0).ToString();

	ScriptError err = RunFile(file_name);

	if (err != ScriptError::Ok)
	{
		KYTY_SCRIPT_THROW_ERROR(create_error(err, U"can't run file", file_name));
	}

	return Scripts::ArgGetVarCount() - 1;
}

KYTY_STATIC_SCRIPT_FUNC(require)
{
	KYTY_SCRIPT_FUNC_BEGIN();

	if (Scripts::ArgGetVarCount() != 1)
	{
		KYTY_SCRIPT_THROW_ERROR(U"invalid args\n");
	}

	// s: name

	String file_name = Scripts::ArgGetVar(0).ToString();

	String::Utf8 name = file_name.utf8_str();

	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED"); // s: name, _LOADED
	lua_getfield(L, 2, name.GetData());            // s: name, _LOADED, module
	if (lua_toboolean(L, -1) != 0)
	{             /* is it there? */
		return 1; /* package is already loaded */
	}

	/* else must load package */
	lua_pop(L, 1); // s: name, _LOADED

	lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD"); // s: name, _LOADED, _PRELOADED
	lua_getfield(L, 3, name.GetData());             // s: name, _LOADED, _PRELOADED, loader

	if (!lua_isnil(L, -1))
	{
		lua_pop(L, 2); // s: name, _LOADED

		String rs = String::FromPrintf(R"(return package.preload["%s"]("%s"))", name.GetData(), name.GetData());

		ScriptError err = RunString(rs); // s: name, _LOADED, module

		if (err != ScriptError::Ok)
		{
			KYTY_SCRIPT_THROW_ERROR(create_error(err, U"can't run", rs));
		}
	} else
	{
		file_name = file_name.ReplaceChar(U'.', U'/') + U".lua";

		ScriptError err = RunFile(file_name); // s: name, _LOADED, module

		if (err != ScriptError::Ok)
		{
			KYTY_SCRIPT_THROW_ERROR(create_error(err, U"can't run file", file_name));
		}
	}

	if (lua_gettop(L) == 2)
	{
		lua_pushnil(L); // s: name, _LOADED, nil
	}

	if (!lua_isnil(L, -1))
	{                                                                    /* non-nil return? */
		lua_setfield(L, 2, name.GetData()); /* _LOADED[name] = module */ // s: name, _LOADED
	}

	lua_getfield(L, 2, name.GetData()); // s: name, _LOADED, module
	if (lua_isnil(L, -1))
	{                                                                  /* module set no value? */
		lua_pushboolean(L, 1); /* use true as result */                // s: name, _LOADED, module, true
		lua_pushvalue(L, -1); /* extra copy to be returned */          // s: name, _LOADED, true, true
		lua_setfield(L, 2, name.GetData()); /* _LOADED[name] = true */ // s: name, _LOADED, true
	}

	return 1;
}

void scripts_file_lib_reg()
{
	RegisterSystemFunc("dofile", dofile);
	RegisterSystemFunc("require", require);
}

} // namespace Kyty::Scripts
