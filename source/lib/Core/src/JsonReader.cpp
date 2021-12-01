#include "Kyty/Core/JsonReader.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/SafeDelete.h"

namespace Kyty::Core {

static String* g_json_error = nullptr;
static Json*   g_null       = nullptr;

static void set_error(const String& str)
{
	if (g_json_error == nullptr)
	{
		g_json_error = new String;
	}
	*g_json_error = str;
}

static const char32_t* skip(const char32_t* in)
{
	if (in == nullptr)
	{
		return nullptr;
	}

	while ((*in != 0u) && Char::IsSpace(*in))
	{
		in++;
	}

	return in;
}

static const char32_t* skip_number(const char32_t* in)
{
	if (in == nullptr)
	{
		return nullptr;
	}

	while ((*in != 0u) && (Char::IsDecimal(*in) || *in == U'-' || *in == U'+' || *in == U'e' || *in == U'E' || *in == U'.'))
	{
		in++;
	}

	return in;
}

const char32_t* Json::parse_value(const char32_t* value)
{
	EXIT_IF(!value);

	switch (*value)
	{
		case U'n':
		{
			if (Char::EqualAsciiN(value, "null", 4))
			{
				m_type = JsonNULL;
				return value + 4;
			}
			break;
		}
		case U'f':
		{
			if (Char::EqualAsciiN(value, "false", 5))
			{
				m_type = JsonBool;
				return value + 5;
			}
			break;
		}
		case U't':
		{
			if (Char::EqualAsciiN(value, "true", 4))
			{
				m_type       = JsonBool;
				m_value_bool = true;
				m_value_int  = 1;
				return value + 4;
			}
			break;
		}
		case U'\"': return parse_string(value);
		case U'[': return parse_array(value);
		case U'{': return parse_object(value);
		case U'-':
		case U'0':
		case U'1':
		case U'2':
		case U'3':
		case U'4':
		case U'5':
		case U'6':
		case U'7':
		case U'8':
		case U'9': return parse_number(value);
		default: return value; break;
	}

	set_error(value);
	return nullptr;
}

const char32_t* Json::parse_array(const char32_t* value)
{
	if (*value != U'[')
	{
		set_error(value);
		return nullptr;
	}

	m_type = JsonArray;

	value = skip(value + 1);

	if (*value == U']')
	{
		return value + 1;
	}

	Json* child = new Json();

	value = skip(child->parse_value(skip(value)));

	if (value == nullptr)
	{
		Delete(child);
		return nullptr;
	}

	m_list.Add(child);

	while (*value == U',')
	{
		child = new Json();

		value = skip(child->parse_value(skip(value + 1)));

		if (value == nullptr)
		{
			Delete(child);
			return nullptr;
		}

		m_list.Add(child);
	}

	if (*value == ']')
	{
		return value + 1;
	}

	set_error(value);
	return nullptr;
}

const char32_t* Json::parse_object(const char32_t* value)
{
	if (*value != U'{')
	{
		set_error(value);
		return nullptr;
	}

	m_type = JsonObject;

	value = skip(value + 1);

	if (*value == U'}')
	{
		return value + 1;
	}

	Json* child = new Json();

	value = skip(child->parse_value(skip(value)));

	if (value == nullptr)
	{
		Delete(child);
		return nullptr;
	}

	child->m_name         = child->m_value_string;
	child->m_value_string = U"";

	if (*value != U':')
	{
		Delete(child);
		set_error(value);
		return nullptr;
	}

	value = skip(child->parse_value(skip(value + 1)));

	if (value == nullptr)
	{
		Delete(child);
		return nullptr;
	}

	m_list.Add(child);

	while (*value == U',')
	{
		child = new Json();

		value = skip(child->parse_value(skip(value + 1)));

		if (value == nullptr)
		{
			Delete(child);
			return nullptr;
		}

		child->m_name         = child->m_value_string;
		child->m_value_string = U"";

		if (*value != U':')
		{
			Delete(child);
			set_error(value);
			return nullptr;
		}

		value = skip(child->parse_value(skip(value + 1)));

		if (value == nullptr)
		{
			Delete(child);
			return nullptr;
		}

		m_list.Add(child);
	}

	if (*value == U'}')
	{
		return value + 1;
	}

	set_error(value);
	return nullptr;
}

const char32_t* Json::parse_number(const char32_t* value)
{
	const char32_t* end = skip_number(value);

	uint32_t len = end - value;

	if (len != 0u)
	{
		String str(U' ', len);
		std::memcpy(str.GetData(), value, len * sizeof(char32_t));
		m_type         = JsonNumber;
		m_value_double = str.ToDouble();
		m_value_int    = static_cast<int64_t>(m_value_double);
		m_value_string = str;
		return end;
	}

	set_error(value);
	return nullptr;
}

const char32_t* Json::parse_string(const char32_t* str)
{
	if (*str != U'\"')
	{
		set_error(str);
		return nullptr;
	}

	const char32_t* ptr = str + 1;

	uint32_t len = 0;
	for (;; len++, ptr++)
	{
		if (*ptr == U'\"' || *ptr == 0)
		{
			break;
		}
		if (*ptr == U'\\')
		{
			ptr++;
		}
	}

	String out(' ', len);

	ptr            = str + 1;
	char32_t* ptr2 = out.GetData();
	while (*ptr != '\"' && *ptr != 0)
	{
		if (*ptr != '\\')
		{
			*ptr2++ = *ptr++;
		} else
		{
			ptr++;
			switch (*ptr)
			{
				case U'b': *ptr2++ = U'\b'; break;
				case U'f': *ptr2++ = U'\f'; break;
				case U'n': *ptr2++ = U'\n'; break;
				case U'r': *ptr2++ = U'\r'; break;
				case U't': *ptr2++ = U'\t'; break;
				case U'u':
					set_error(ptr);
					return nullptr;
					break;
				default: *ptr2++ = *ptr; break;
			}
			ptr++;
		}
	}

	if (*ptr == U'\"')
	{
		ptr++;
	}

	len = ptr2 - out.GetDataConst();

	m_value_string = out.Left(len);
	m_type         = JsonString;

	return ptr;
}

// Json::Json()
//{
//	m_value_int    = 0;
//	m_value_double = 0.0;
//	m_value_bool   = false;
//	m_type         = JsonNULL;
//}

Json::~Json()
{
	FOR (i, m_list)
	{
		Delete(m_list[i]);
	}
}

const Json* Json::Create(const String& str)
{
	Json* c = new Json();

	const char32_t* value = c->parse_value(skip(str.GetDataConst()));

	if (value == nullptr)
	{
		Delete(c);
		return nullptr;
	}

	return c;
}

String Json::GetError()
{
	if (g_json_error != nullptr)
	{
		return *g_json_error;
	}

	return U"";
}

void Json::Init()
{
	if (g_null == nullptr)
	{
		g_null = new Json();
	}
}

// const Json* Json::GetItem(const String& string) const
const Json* Json::GetItem(const char* string) const
{
	FOR (i, m_list)
	{
		const Json* n = m_list.At(i);
		if (n->m_name.EqualAsciiNoCase(string))
		{
			return n;
		}
	}

	Init();

	return g_null;
}

// String Json::GetString(const String& name, const String& default_value) const
String Json::GetString(const char* name, const String& default_value) const
{
	const Json* n = GetItem(name);
	if (!n->IsNull())
	{
		return n->m_value_string;
	}
	return default_value;
}

String Json::GetString(const char* name) const
{
	const Json* n = GetItem(name);
	if (!n->IsNull())
	{
		return n->m_value_string;
	}
	return U"";
}

// double Json::GetFloat(const String& name, double default_value) const
double Json::GetFloat(const char* name, double default_value) const
{
	const Json* n = GetItem(name);
	if (!n->IsNull())
	{
		return n->m_value_double;
	}
	return default_value;
}

// int64_t Json::GetInt(const String& name, int64_t default_value) const
int64_t Json::GetInt(const char* name, int64_t default_value) const
{
	const Json* n = GetItem(name);
	if (!n->IsNull())
	{
		return n->m_value_int;
	}
	return default_value;
}

bool Json::GetBool(const char* name, bool default_value) const
{
	const Json* n = GetItem(name);
	if (!n->IsNull())
	{
		return n->m_value_bool;
	}
	return default_value;
}

StringList Json::DbgCheckList(const StringList& required, const StringList& optional) const
{
	StringList errors;

	FOR (i, required)
	{
		if (GetItem(required.At(i).C_Str())->m_name.IsEmpty())
		{
			errors.Add(U"missing: " + required.At(i));
		}
	}

	FOR (i, m_list)
	{
		const Json* n = m_list.At(i);
		if (!n->m_name.IsEmpty() && !(required.Contains(n->m_name) || optional.Contains(n->m_name)))
		{
			errors.Add(U"unknown: " + n->m_name);
		}
	}

	return errors;
}

} // namespace Kyty::Core
