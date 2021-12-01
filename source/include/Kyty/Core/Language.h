#ifndef INCLUDE_KYTY_CORE_LANGUAGE_H_
#define INCLUDE_KYTY_CORE_LANGUAGE_H_

#include "Kyty/Core/String.h"

namespace Kyty::Core {

// SUBSYSTEM_DEFINE(Language);

enum class LanguageId
{
	Unknown,
	//	Arabic,
	//	Chinese,
	German,
	English,
	French,
	//	Hindi,
	Italian,
	//	Japanese,
	//	Korean,
	Portuguese,
	Russian,
	Spanish
};

namespace Language {
void Init();

StringList GetLanguages();
LanguageId GetId(const String& id);
String     GetCharList(const String& id);
String     GetLettersList(const String& id);
String     GetLettersList(LanguageId lang_id);
String     GetNumericList(const String& id);
String     GetPunctuationList(const String& id);
String     GetCharListAll();
String     GetNameOfMonth(int month, LanguageId lang_id);
String     GetNameOfMonthShort(int month, LanguageId lang_id);
String     GetNameOfDay(int day, LanguageId lang_id);
String     GetNameOfDayShort(int day, LanguageId lang_id);
} // namespace Language

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_LANGUAGE_H_ */
