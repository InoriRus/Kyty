#ifndef INCLUDE_KYTY_SCRIPTS_SCRIPTS_H_
#define INCLUDE_KYTY_SCRIPTS_SCRIPTS_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Subsystems.h"
#include "Kyty/Core/Vector.h"

#if KYTY_PLATFORM == KYTY_PLATFORM_WINDOWS
//#include <string.h>
#endif

#define KYTY_LUA_5_2 1
#define KYTY_LUA_5_3 2
#define KYTY_LUA_VER KYTY_LUA_5_2

namespace Kyty::Scripts {

using LuaState = void;

KYTY_SUBSYSTEM_DEFINE(Scripts);

enum class ScriptError
{
	Ok,
	SyntaxError,
	FileError,
	RunError,
	UnknownError
};

using script_func_t = int (*)(LuaState*);
using help_func_t   = void (*)();

#define KYTY_SCRIPT_NAME(name) name([[maybe_unused]] Kyty::Scripts::LuaState* LS)
//#define STATIC_SCRIPT_FUNC(name) static int SCRIPT_NAME(name)
#define KYTY_SCRIPT_FUNC(name) int KYTY_SCRIPT_NAME(name)

#define KYTY_STATIC_SCRIPT_FUNC(name)                                                                                                      \
	static int name##_private([[maybe_unused]] Kyty::Scripts::LuaState* LS, [[maybe_unused]] Scripts::ScriptFuncResult* script_result);    \
	static int name(Kyty::Scripts::LuaState* LS)                                                                                           \
	{                                                                                                                                      \
		Scripts::ScriptFuncResult script_result;                                                                                           \
		int                       r = name##_private(LS, &script_result);                                                                  \
		script_result.ThrowError();                                                                                                        \
		return r;                                                                                                                          \
	}                                                                                                                                      \
	static int name##_private([[maybe_unused]] Kyty::Scripts::LuaState* LS, [[maybe_unused]] Scripts::ScriptFuncResult* script_result)

ScriptError   RunFile(const String& file_name);
ScriptError   RunString(const String& source);
const String& GetErrMsg();
void          SetErrMsg(const String& msg);
void          ResetErrMsg();
void          PushString(const String& str);
void          PushDouble(double d);
#if KYTY_LUA_VER == KYTY_LUA_5_3
void PushInteger(int64_t i);
#else
void PushInteger(int32_t i);
#endif

void RegisterFunc(const char* name, script_func_t func, help_func_t help);
void RegisterSystemFunc(const char* name, script_func_t func);
void UnregisterFunc(const char* name);

int  ArgGetVarCount();
void ArgDbgDump();

void PrintHelp();

class ScriptVar;

class ScriptPair final
{
public:
	ScriptPair(const ScriptVar& key, const ScriptVar& value);
	~ScriptPair();

	ScriptPair(const ScriptPair& src);

	ScriptPair& operator=(const ScriptPair&) = delete;
	ScriptPair(ScriptPair&&) noexcept        = delete;
	ScriptPair& operator=(ScriptPair&&) noexcept = delete;

	[[nodiscard]] const ScriptVar& GetKey() const { return *m_key; }

	[[nodiscard]] const ScriptVar& GetValue() const { return *m_value; }

private:
	ScriptVar* m_key;
	ScriptVar* m_value;
};

class ScriptTable final
{
public:
	using List = Vector<ScriptPair>;

	void Add(const ScriptVar& k, const ScriptVar& v);

	void DbgPrint(int depth) const;

#if KYTY_LUA_VER == KYTY_LUA_5_3
	ScriptVar At(int64_t m_key) const;
#endif
	[[nodiscard]] ScriptVar At(const String& key) const;
	[[nodiscard]] ScriptVar At(double key) const;

	[[nodiscard]] uint32_t  Count() const { return m_pairs.Size(); }
	[[nodiscard]] ScriptVar GetKey(uint32_t index) const;
	[[nodiscard]] ScriptVar GetValue(uint32_t index) const;

	[[nodiscard]] const List& GetList() const { return m_pairs; }

private:
	List m_pairs;
};

class ScriptFunction final
{
public:
	void LoadFromStack();

	[[nodiscard]] String ToDbgString() const;

private:
	static int LuaWriter(LuaState* ls, const void* p, size_t sz, void* ud);

	Vector<uint8_t> m_dump;
};

class ScriptVar
{
public:
	ScriptVar();

	virtual ~ScriptVar();

	ScriptVar(const ScriptVar& src);
	ScriptVar(ScriptVar&& src) noexcept;
	ScriptVar& operator=(const ScriptVar& src);
	ScriptVar& operator=(ScriptVar&& src) noexcept;

	void DbgPrint(int depth) const;

	static ScriptVar ReadVar(int index, bool with_metatable_index);

	[[nodiscard]] bool IsTable() const;
	[[nodiscard]] bool IsNil() const;
	[[nodiscard]] bool IsFunction() const;
	[[nodiscard]] bool IsCFunction() const;
	[[nodiscard]] bool IsDouble() const;
#if KYTY_LUA_VER == KYTY_LUA_5_3
	bool IsInteger() const;
#else
	[[nodiscard]] bool    IsInteger() const;
#endif
	[[nodiscard]] bool IsString() const;
	[[nodiscard]] bool IsUserdata() const;

#if KYTY_LUA_VER == KYTY_LUA_5_3
	int64_t ToInteger() const;
#else
	[[nodiscard]] int32_t ToInteger() const;
#endif
	[[nodiscard]] double                   ToDouble() const;
	[[nodiscard]] float                    ToFloat() const;
	[[nodiscard]] bool                     ToBool() const;
	[[nodiscard]] String                   ToString() const;
	[[nodiscard]] void*                    ToUserdata() const;
	[[nodiscard]] const ScriptTable&       ToTable() const;
	[[nodiscard]] const ScriptTable::List& GetPairs() const;

	[[nodiscard]] ScriptVar At(int64_t key) const;
	[[nodiscard]] ScriptVar At(const String& key) const;
	[[nodiscard]] ScriptVar At(const char* key) const;
	[[nodiscard]] ScriptVar At(double key) const;

	[[nodiscard]] uint32_t  Count() const;
	[[nodiscard]] uint32_t  Size() const;
	[[nodiscard]] ScriptVar GetKey(uint32_t index) const;
	[[nodiscard]] ScriptVar GetValue(uint32_t index) const;

private:
	class ScriptVarPrivate;
	ScriptVarPrivate* m_p = {nullptr};
};

ScriptVar GlobalGetVar(const String& var_name);
ScriptVar GlobalGetVarWithParent(const String& var_name);
ScriptVar ArgGetVar(int index);
ScriptVar ArgGetVarWithParent(int index);

constexpr size_t SCRIPT_FUNC_ERR_SIZE = 1024;

class ScriptFuncResult
{
public:
	ScriptFuncResult() = default;

	void SetError(const String& msg);

	void ThrowError();

private:
	bool m_ok                            = {true};
	char m_msg[SCRIPT_FUNC_ERR_SIZE + 1] = {0};
};

#define KYTY_SCRIPT_FUNC_BEGIN() [[maybe_unused]] auto* L = static_cast<struct lua_State*>(LS);
#define KYTY_SCRIPT_THROW_ERROR(msg)                                                                                                       \
	{                                                                                                                                      \
		script_result->SetError(msg);                                                                                                      \
		return 0;                                                                                                                          \
	}

} // namespace Kyty::Scripts

#endif /* INCLUDE_KYTY_SCRIPTS_SCRIPTS_H_ */
