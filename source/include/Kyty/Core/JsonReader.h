#ifndef INCLUDE_KYTY_CORE_JSONREADER_H_
#define INCLUDE_KYTY_CORE_JSONREADER_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/String.h"
#include "Kyty/Core/Vector.h"

namespace Kyty::Core {

enum JsonType
{
	JsonBool,
	JsonNULL,
	JsonNumber,
	JsonString,
	JsonArray,
	JsonObject,
};

class Json
{
public:
	using ListType = Vector<const Json*>;

	virtual ~Json();

	static void        Init();
	static const Json* Create(const String& str);
	static String      GetError();

	//	const Json* GetItem(const String &string) const;
	//	String GetString(const String &name, const String &default_value) const;
	//	double GetFloat(const String &name, double default_value) const;
	//	int64_t GetInt(const String &name, int64_t default_value) const;
	const Json*          GetItem(const char* string) const;
	String               GetString(const char* name, const String& default_value) const;
	String               GetString(const char* name) const;
	double               GetFloat(const char* name, double default_value) const;
	int64_t              GetInt(const char* name, int64_t default_value) const;
	bool                 GetBool(const char* name, bool default_value) const;
	[[nodiscard]] String GetName() const { return m_name; }

	[[nodiscard]] JsonType        GetType() const { return m_type; }
	[[nodiscard]] bool            IsNull() const { return m_type == JsonNULL; }
	[[nodiscard]] bool            IsNumber() const { return m_type == JsonNumber; }
	[[nodiscard]] bool            IsString() const { return m_type == JsonString; }
	[[nodiscard]] bool            IsObject() const { return m_type == JsonObject; }
	[[nodiscard]] bool            IsArray() const { return m_type == JsonArray; }
	[[nodiscard]] bool            IsBool() const { return m_type == JsonBool; }
	[[nodiscard]] String          ToString() const { return m_value_string; }
	[[nodiscard]] double          ToFloat() const { return m_value_double; }
	[[nodiscard]] int64_t         ToInt() const { return m_value_int; }
	[[nodiscard]] int64_t         ToBool() const { return static_cast<int64_t>(m_value_bool); }
	[[nodiscard]] const ListType& ToArray() const { return m_list; }

	[[nodiscard]] StringList DbgCheckList(const StringList& required, const StringList& optional) const;

	KYTY_CLASS_NO_COPY(Json);

private:
	Json() = default;

	const char32_t* parse_value(const char32_t* value);
	const char32_t* parse_array(const char32_t* value);
	const char32_t* parse_object(const char32_t* value);
	const char32_t* parse_string(const char32_t* str);
	const char32_t* parse_number(const char32_t* value);

	ListType m_list;
	JsonType m_type = JsonNULL;
	String   m_value_string;
	int64_t  m_value_int    = 0;
	double   m_value_double = 0.0;
	bool     m_value_bool   = false;
	String   m_name;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_JSONREADER_H_ */
