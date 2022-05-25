#include "Kyty/Scripts/Scripts.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/File.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/MemoryAlloc.h"
#include "Kyty/Core/RefCounter.h"
#include "Kyty/Core/SafeDelete.h"
#include "Kyty/Core/Threads.h"
#include "Kyty/Scripts/LuaCpp.h"

// IWYU pragma: no_include <sec_api/string_s.h>

namespace Kyty::Scripts {

using Core::File;
using Core::Hashmap;
using Core::mem_tracker_disable;
using Core::mem_tracker_enable;

static thread_local lua_State* g_lua_state = nullptr;
static thread_local String*    g_lua_error = nullptr;

struct HelpFuncList
{
	Hashmap<String, help_func_t> map;
};

static thread_local HelpFuncList* g_help_list = nullptr;

static const luaL_Reg g_loadedlibs[] = {{"_G", luaopen_base},
                                        {LUA_LOADLIBNAME, luaopen_package},
                                        {LUA_COLIBNAME, luaopen_coroutine},
                                        {LUA_TABLIBNAME, luaopen_table},
                                        {LUA_IOLIBNAME, luaopen_io},
                                        {LUA_OSLIBNAME, luaopen_os},
                                        {LUA_STRLIBNAME, luaopen_string},
                                        {LUA_BITLIBNAME, luaopen_bit32},
                                        {LUA_MATHLIBNAME, luaopen_math},
                                        {LUA_DBLIBNAME, luaopen_debug},
                                        {nullptr, nullptr}};

static void lua_my_openlibs(lua_State* l)
{
	for (const luaL_Reg* lib = g_loadedlibs; lib->func != nullptr; lib++)
	{
		luaL_requiref(l, lib->name, lib->func, 1);
		lua_pop(l, 1);
	}
}

void scripts_file_lib_reg();

static void load_libs()
{
	lua_my_openlibs(g_lua_state);

	scripts_file_lib_reg();
}

static void lua_init()
{
	if (g_lua_state == nullptr)
	{
		mem_tracker_disable();

		g_lua_state = luaL_newstate();

		if (g_lua_state == nullptr)
		{
			EXIT("Lua state is NULL");
		}

		load_libs();

		g_help_list = new HelpFuncList;

		mem_tracker_enable();
	}
}

KYTY_SUBSYSTEM_INIT(Scripts)
{
	g_lua_state = nullptr;
	g_lua_error = nullptr;
	g_help_list = nullptr;

	if (RunString(U"_script_check_a = _script_check_b") != ScriptError::Ok || RunString(U"_a _b _c _d") != ScriptError::SyntaxError ||
	    RunString(U"_script_check_a = _script_check_b[1]") != ScriptError::RunError)
	{
		KYTY_SUBSYSTEM_FAIL("Can't run Lua scripts");
	}

	ResetErrMsg();
}

KYTY_SUBSYSTEM_UNEXPECTED_SHUTDOWN(Scripts) {}

KYTY_SUBSYSTEM_DESTROY(Scripts)
{
	// lua_close(g_lua_state);
}

void script_dbg_dump_stack(const String& prefix)
{
	lua_init();

	printf("%s\n", prefix.C_Str());
	int top = lua_gettop(g_lua_state);
	for (int i = 1; i <= top; i++)
	{
		int t = lua_type(g_lua_state, i);
		printf("%d: %s, n: %g\n", i, lua_typename(g_lua_state, t), lua_tonumber(g_lua_state, i));
	}
}

ScriptError LoadString(const String& source)
{
	lua_init();

	int err = luaL_loadstring(g_lua_state, source.C_Str());

	//	printf("source:\n%s\n", source.C_Str());
	//	printf("binary:\n");
	//	ScriptVar::ReadVar(-1).DbgPrint(0);

	if (err != LUA_OK)
	{
		ScriptVar s = ScriptVar::ReadVar(-1, false);

		SetErrMsg(s.ToString());

		lua_pop(g_lua_state, 1);
	}

	if (err == LUA_ERRSYNTAX)
	{
		return ScriptError::SyntaxError;
	}

	if (err != LUA_OK)
	{
		return ScriptError::UnknownError;
	}

	return ScriptError::Ok;
}

ScriptError Run()
{
	lua_init();

	int err = lua_pcall(g_lua_state, 0, LUA_MULTRET, 0);

	// int top = lua_gettop(g_lua_state);

	if (err != LUA_OK)
	{
		// script_dbg_dump_stack("");

		ScriptVar s = ScriptVar::ReadVar(-1, false);

		SetErrMsg(s.ToString());

		lua_pop(g_lua_state, 1);
	}

	// EXIT_IF(lua_gettop(g_lua_state));

	if (err == LUA_ERRRUN)
	{
		return ScriptError::RunError;
	}

	if (err != LUA_OK)
	{
		return ScriptError::UnknownError;
	}

	return ScriptError::Ok;
}

ScriptError RunFile(const String& file_name)
{
	lua_init();
	//	EXIT_IF(lua_gettop(g_lua_state) > 0);

	File f(file_name, File::Mode::Read);
	f.SetEncoding(File::Encoding::Utf8);

	if (f.IsInvalid())
	{
		return ScriptError::FileError;
	}

	String str = f.ReadWholeString();
	f.Close();

	// TODO(#117): load and run binary files

	ScriptError status = LoadString(str);

	if (status != ScriptError::Ok)
	{
		return status;
	}

	status = Run();

	return status;
}

ScriptError RunString(const String& source)
{
	lua_init();
	//	EXIT_IF(lua_gettop(g_lua_state) > 0);

	ScriptError status = LoadString(source);

	if (status != ScriptError::Ok)
	{
		return status;
	}

	status = Run();

	return status;
}

void RegisterSystemFunc(const char* name, script_func_t func)
{
	lua_init();
	// EXIT_IF(GlobalGetVar(name).IsNil());

	lua_register(g_lua_state, name, reinterpret_cast<lua_CFunction>(func));
}

void RegisterFunc(const char* name, script_func_t func, help_func_t help)
{
	lua_init();

	String n = String::FromUtf8(name);
	EXIT_IF(!GlobalGetVar(n).IsNil());
	EXIT_IF(g_help_list->map.Contains(n));

	lua_register(g_lua_state, name, reinterpret_cast<lua_CFunction>(func));

	g_help_list->map.Put(n, help);
}

void UnregisterFunc(const char* name)
{
	lua_init();

	String n = String::FromUtf8(name);
	EXIT_IF(!GlobalGetVar(n).IsCFunction());
	EXIT_IF(!g_help_list->map.Contains(n));

	RunString(n + U" = nil");
	g_help_list->map.Remove(n);
}

int ArgGetVarCount()
{
	lua_init();

	return lua_gettop(g_lua_state);
}

void ArgDbgDump()
{
	lua_init();

	int num = ArgGetVarCount();
	for (int i = 0; i < num; i++)
	{
		printf("--[%d]--\n", i);
		ArgGetVar(i).DbgPrint(0);
	}
}

void PrintHelp()
{
	lua_init();

	FOR_HASH (g_help_list->map)
	{
		printf("Lua function: %s\n", g_help_list->map.Key().C_Str());
		auto f = g_help_list->map.Value();
		f();
	}
}

class ScriptVar::ScriptVarPrivate: public Core::RefCounter<Core::DummyMutex>
{
public:
	ScriptVarPrivate() = default;

	bool is_table      = {false};
	bool is_nil        = {true};
	bool is_function   = {false};
	bool is_c_function = {false};
	bool is_double     = {false};
#if KYTY_LUA_VER == KYTY_LUA_5_3
	bool is_integer = {false};
#endif
	bool   is_string   = {false};
	bool   is_userdata = {false};
	double val_double  = {0.0};
	bool   val_bool    = {false};
#if KYTY_LUA_VER == KYTY_LUA_5_3
	int64_t val_integer = {0};
#endif
	String         val_string;
	ScriptTable    val_table;
	ScriptFunction val_function;
	void*          val_userdata = {nullptr};
};

// ScriptVar::ScriptVarPrivate::ScriptVarPrivate()
//    : is_table(false), is_nil(true), is_function(false), is_c_function(false), is_double(false),
//#if KYTY_LUA_VER == KYTY_LUA_5_3
//      is_integer(false),
//#endif
//      is_string(false), is_userdata(false), val_double(0.0), val_bool(false),
//#if KYTY_LUA_VER == KYTY_LUA_5_3
//      val_integer(0),
//#endif
//      val_userdata(nullptr)
//{
//}

void ScriptVar::DbgPrint(int depth) const
{
	lua_init();

	for (int i = 0; i < depth; i++)
	{
		printf("  ");
	}
	if (m_p->is_nil)
	{
		printf("nil\n");
	} else if (m_p->is_table)
	{
		printf("Table:\n");
		m_p->val_table.DbgPrint(depth + 1);
	} else if (m_p->is_c_function)
	{
		printf("CFunction\n");
	} else if (m_p->is_function)
	{
		printf("Function:\n");
		printf("%s\n", m_p->val_function.ToDbgString().C_Str());
	} else if (m_p->is_userdata)
	{
		printf("Userdata: %016" PRIx64 "\n", reinterpret_cast<uint64_t>(m_p->val_userdata));
	} else
	{
#if KYTY_LUA_VER == KYTY_LUA_5_3
		printf("d: %g, i: %" PRIi64 " s: %s\n", val_double, val_integer, val_string.C_Str());
#else
		printf("d: %g, s: %s\n", m_p->val_double, m_p->val_string.C_Str());
#endif
	}
}

ScriptVar::ScriptVar(): m_p(new ScriptVarPrivate)
{
	// m_p = new ScriptVarPrivate;
}

ScriptVar::~ScriptVar()
{
	if (m_p != nullptr)
	{
		m_p->Release();
	}
}

ScriptVar::ScriptVar(const ScriptVar& src)
{
	src.m_p->CopyPtr(&m_p, src.m_p);
}

ScriptVar::ScriptVar(ScriptVar&& src) noexcept: m_p(src.m_p)
{
	//	m_p     = src.m_p;
	src.m_p = nullptr;
}

ScriptVar& ScriptVar::operator=(const ScriptVar& src)
{
	if (this != &src && m_p != src.m_p)
	{
		if (m_p != nullptr)
		{
			m_p->Release();
		}

		src.m_p->CopyPtr(&m_p, src.m_p);
	}
	return *this;
}

ScriptVar& ScriptVar::operator=(ScriptVar&& src) noexcept
{
	if (m_p != src.m_p)
	{
		if (m_p != nullptr)
		{
			m_p->Release();
		}

		m_p     = src.m_p;
		src.m_p = nullptr;
	}
	return *this;
}

ScriptVar ScriptVar::ReadVar(int index, bool with_metatable_index)
{
	lua_init();

	ScriptVar r;

	EXIT_IF(index == 0);

	// script_dbg_dump_stack(String::FromPrintf("script_read_var( %d )", index));

	if (index < 0)
	{
		index = lua_gettop(g_lua_state) + index + 1;
	}

	lua_pushvalue(g_lua_state, index);

	// printf("sizeof(lua_Integer) = %d\n", (int)sizeof(lua_Integer));
	// printf("LUA_INTEGER_FRMLEN = %s\n", LUA_INTEGER_FRMLEN);
	// printf("LUA_MAXINTEGER = %" PRIi64"\n", (int64_t)LUA_MAXINTEGER);
	// printf("LUA_MININTEGER = %" PRIi64"\n", (int64_t)LUA_MININTEGER);

	r.m_p->val_double = lua_tonumber(g_lua_state, -1);
	r.m_p->val_bool   = (lua_toboolean(g_lua_state, -1) != 0);
#if KYTY_LUA_VER == KYTY_LUA_5_3
	Read.m_p->val_integer = lua_tointeger(g_lua_state, -1);
#endif
	r.m_p->val_string   = String::FromUtf8(lua_tostring(g_lua_state, -1));
	r.m_p->val_userdata = lua_touserdata(g_lua_state, -1);

	r.m_p->is_c_function = (lua_iscfunction(g_lua_state, -1) != 0);
	r.m_p->is_double     = (lua_isnumber(g_lua_state, -1) != 0);
	r.m_p->is_function   = lua_isfunction(g_lua_state, -1);
#if KYTY_LUA_VER == KYTY_LUA_5_3
	Read.is_integer = lua_isinteger(g_lua_state, -1);
	if (!Read.is_integer)
	{
		Read.is_integer = lua_tointeger(g_lua_state, -1) != 0 || lua_tostring(g_lua_state, -1) == String("0");
	}
#endif
	r.m_p->is_nil      = lua_isnil(g_lua_state, -1);
	r.m_p->is_string   = (lua_isstring(g_lua_state, -1) != 0);
	r.m_p->is_table    = lua_istable(g_lua_state, -1);
	r.m_p->is_userdata = (lua_isuserdata(g_lua_state, -1) != 0);

	if (r.m_p->is_function)
	{
		r.m_p->val_function.LoadFromStack();
	}

	lua_pop(g_lua_state, 1);

	if (r.m_p->is_table)
	{
		lua_pushnil(g_lua_state); /* first key */

		// script_dbg_dump_stack(String::FromPrintf("is_table"));

		while (lua_next(g_lua_state, index) != 0)
		{
			// script_dbg_dump_stack(String::FromPrintf("after_next"));

			int top = lua_gettop(g_lua_state);

			/* uses 'key' (at index top-2+1) and 'value' (at index top-1+1) */

			ScriptVar k = ReadVar(top - 2 + 1, with_metatable_index);
			ScriptVar v = ReadVar(top - 1 + 1, with_metatable_index);

			r.m_p->val_table.Add(k, v);

			/* removes 'value'; keeps 'key' for next iteration */
			lua_pop(g_lua_state, 1);

			// script_dbg_dump_stack(String::FromPrintf("end_of_while"));
		}

		// lua_pop(g_lua_state, 1);

		// script_dbg_dump_stack(String::FromPrintf("last_pop"));

		if (with_metatable_index)
		{
			if (lua_getmetatable(g_lua_state, index) != 0)
			{
				ScriptVar metatable = ReadVar(-1, true);

				ScriptVar parent = metatable.At(U"__index");

				if (parent.IsTable())
				{
					// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
					for (const auto& p: parent.GetPairs())
					{
						if (r.At(p.GetKey().ToString()).IsNil())
						{
							r.m_p->val_table.Add(p.GetKey(), p.GetValue());
						}
					}
				}

				lua_pop(g_lua_state, 1);
			}
		}
	}

	return r;
}

bool ScriptVar::IsTable() const
{
	return m_p->is_table;
}
bool ScriptVar::IsNil() const
{
	return m_p->is_nil;
}
bool ScriptVar::IsFunction() const
{
	return m_p->is_function;
}
bool ScriptVar::IsCFunction() const
{
	return m_p->is_c_function;
}
bool ScriptVar::IsDouble() const
{
	return m_p->is_double;
}
#if KYTY_LUA_VER == KYTY_LUA_5_3
bool ScriptVar::IsInteger() const
{
	return m_p->is_integer;
}
#else
bool ScriptVar::IsInteger() const
{
	return m_p->is_double && static_cast<double>(static_cast<int32_t>(m_p->val_double)) == m_p->val_double;
}
#endif
bool ScriptVar::IsString() const
{
	return m_p->is_string;
}
bool ScriptVar::IsUserdata() const
{
	return m_p->is_string;
}

#if KYTY_LUA_VER == KYTY_LUA_5_3
int64_t ScriptVar::ToInteger() const
{
	return val_integer;
}
#else
int32_t ScriptVar::ToInteger() const
{
	return IsInteger() ? static_cast<int32_t>(m_p->val_double) : 0;
}
#endif
double ScriptVar::ToDouble() const
{
	return m_p->val_double;
}
float ScriptVar::ToFloat() const
{
	return static_cast<float>(m_p->val_double);
}
bool ScriptVar::ToBool() const
{
	return m_p->val_bool;
}
String ScriptVar::ToString() const
{
	return m_p->val_string;
}
void* ScriptVar::ToUserdata() const
{
	return m_p->val_userdata;
}
const ScriptTable& ScriptVar::ToTable() const
{
	return m_p->val_table;
}
const ScriptTable::List& ScriptVar::GetPairs() const
{
	return m_p->val_table.GetList();
}

ScriptVar ScriptVar::At(int64_t key) const
{
	return m_p->val_table.At(static_cast<double>(key));
}
ScriptVar ScriptVar::At(const String& key) const
{
	return m_p->val_table.At(key);
}
ScriptVar ScriptVar::At(const char* key) const
{
	return m_p->val_table.At(String::FromUtf8(key));
}
ScriptVar ScriptVar::At(double key) const
{
	return m_p->val_table.At(key);
}

uint32_t ScriptVar::Count() const
{
	return m_p->val_table.Count();
}
uint32_t ScriptVar::Size() const
{
	return m_p->val_table.Count();
}
ScriptVar ScriptVar::GetKey(uint32_t index) const
{
	return m_p->val_table.GetKey(index);
}
ScriptVar ScriptVar::GetValue(uint32_t index) const
{
	return m_p->val_table.GetValue(index);
}

void script_global_dbg_dump_var(const String& var_name)
{
	lua_init();

	lua_getglobal(g_lua_state, var_name.C_Str());

	ScriptVar r = ScriptVar::ReadVar(-1, false);
	r.DbgPrint(0);

	lua_pop(g_lua_state, 1);
}

ScriptVar GlobalGetVar(const String& var_name)
{
	lua_init();

	lua_getglobal(g_lua_state, var_name.C_Str());
	ScriptVar ret = ScriptVar::ReadVar(-1, false);
	lua_pop(g_lua_state, 1);
	return ret;
}

ScriptVar GlobalGetVarWithParent(const String& var_name)
{
	lua_init();

	lua_getglobal(g_lua_state, var_name.C_Str());
	ScriptVar ret = ScriptVar::ReadVar(-1, true);
	lua_pop(g_lua_state, 1);
	return ret;
}

void SetErrMsg(const String& msg)
{
	lua_init();

	if (g_lua_error == nullptr)
	{
		g_lua_error = new String();
	}

	*g_lua_error = msg;
}

const String& GetErrMsg()
{
	lua_init();

	if (g_lua_error == nullptr)
	{
		g_lua_error = new String();
	}

	return *g_lua_error;
}

void ResetErrMsg()
{
	lua_init();

	delete g_lua_error;
	g_lua_error = nullptr;
}

ScriptVar ArgGetVar(int index)
{
	lua_init();

	ScriptVar ret = ScriptVar::ReadVar(index + 1, false);
	return ret;
}

ScriptVar ArgGetVarWithParent(int index)
{
	lua_init();

	ScriptVar ret = ScriptVar::ReadVar(index + 1, true);
	return ret;
}

void PushString(const String& str)
{
	lua_init();

	lua_pushstring(g_lua_state, str.C_Str());
}

void PushDouble(double d)
{
	lua_init();

	lua_pushnumber(g_lua_state, static_cast<lua_Number>(d));
}

#if KYTY_LUA_VER == KYTY_LUA_5_3
void PushInteger(int64_t i)
{
	lua_init();

	lua_pushinteger(g_lua_state, i);
}
#else
void PushInteger(int32_t i)
{
	lua_init();

	PushDouble(static_cast<double>(i));
}
#endif

#if KYTY_LUA_VER == KYTY_LUA_5_3
ScriptVar ScriptTable::At(int64_t m_key) const
{
	lua_init();

	FOR (i, keys)
	{
		const ScriptVar& v = keys.At(i);
		if (v.IsInteger() && v.ToInteger() == m_key)
		{
			return values.At(i);
		}
	}
	return ScriptVar();
}
#endif

ScriptVar ScriptTable::At(const String& key) const
{
	lua_init();

	// FOR(i, keys)
	for (const auto& p: m_pairs)
	{
		const ScriptVar& v = p.GetKey();
		if (v.IsString() && v.ToString() == key)
		{
			return p.GetValue();
		}
	}
	return {};
}

ScriptVar ScriptTable::At(double key) const
{
	lua_init();

	// FOR(i, keys)
	for (const auto& p: m_pairs)
	{
		const ScriptVar& v = p.GetKey();
		if (v.IsDouble() && v.ToDouble() == key)
		{
			return p.GetValue();
		}
	}
	return {};
}

void ScriptTable::DbgPrint(int depth) const
{
	lua_init();

	// FOR(k, keys)
	int k = 0;
	for (const auto& p: m_pairs)
	{
		for (int i = 0; i < depth; i++)
		{
			printf("  ");
		}
		printf("--- %d ---\n", k++);
		for (int i = 0; i < depth; i++)
		{
			printf("  ");
		}
		printf("Key:\n");
		p.GetKey().DbgPrint(depth);
		for (int i = 0; i < depth; i++)
		{
			printf("  ");
		}
		printf("Value:\n");
		p.GetValue().DbgPrint(depth);
	}
}

void ScriptTable::Add(const ScriptVar& k, const ScriptVar& v)
{
	lua_init();

	ScriptPair p(k, v);

	m_pairs.Add(p);
	// keys.Add(k);
	// values.Add(v);
}

ScriptVar ScriptTable::GetKey(uint32_t index) const
{
	lua_init();

	return m_pairs.At(index).GetKey();
}

ScriptVar ScriptTable::GetValue(uint32_t index) const
{
	lua_init();

	return m_pairs.At(index).GetValue();
}

void ScriptFuncResult::SetError(const String& msg)
{
	m_ok = false;
#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
	strncpy_s(m_msg, SCRIPT_FUNC_ERR_SIZE + 1, msg.C_Str(), SCRIPT_FUNC_ERR_SIZE);
#else
	strncpy(m_msg, msg.C_Str(), SCRIPT_FUNC_ERR_SIZE);
#endif
	m_msg[SCRIPT_FUNC_ERR_SIZE] = '\0';
}

void ScriptFuncResult::ThrowError()
{
	lua_init();

	if (!m_ok)
	{
		lua_pushstring(g_lua_state, m_msg);
		lua_error(g_lua_state);
	}
}

void ScriptFunction::LoadFromStack()
{
	lua_init();

#if KYTY_LUA_VER == KYTY_LUA_5_3
	lua_dump(g_lua_state, reinterpret_cast<lua_Writer>(LuaWriter), this, 0);
#else
	lua_dump(g_lua_state, reinterpret_cast<lua_Writer>(LuaWriter), this);
#endif
}

String ScriptFunction::ToDbgString() const
{
	lua_init();

	String str;

	FOR (i, m_dump)
	{
		str += String::FromPrintf("%02" PRIx8 "", m_dump.At(i));
	}

	return str;
}

int ScriptFunction::LuaWriter(LuaState* /*LS*/, const void* p, size_t sz, void* ud)
{
	lua_init();

	auto* f = static_cast<ScriptFunction*>(ud);
	EXIT_IF(sizeof(size_t) > 4 && (static_cast<uint64_t>(sz) >> 32u) > 0);
	f->m_dump.Add(static_cast<const uint8_t*>(p), static_cast<uint32_t>(sz));
	return 0;
}

ScriptPair::ScriptPair(const ScriptVar& key, const ScriptVar& value): m_key(new ScriptVar(key)), m_value(new ScriptVar(value)) {}

ScriptPair::~ScriptPair()
{
	Delete(m_key);
	Delete(m_value);
}

ScriptPair::ScriptPair(const ScriptPair& src): m_key(new ScriptVar(*src.m_key)), m_value(new ScriptVar(*src.m_value)) {}

} // namespace Kyty::Scripts
