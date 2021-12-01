#include "Kyty/Core/Language.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Core/Hashmap.h"
#include "Kyty/Core/Vector.h"

namespace Kyty::Core {

static const char32_t* g_list_numeric       = U"0123456789";
static const char32_t* g_list_punctuation   = U" !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
static const char32_t* g_list_punctuation_2 = U"ªº¡¿";

static const char32_t* g_alphabet_english_1 = U"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char32_t* g_alphabet_english_2 = U"abcdefghijklmnopqrstuvwxyz";

static const char32_t* g_alphabet_russian_1 = U"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЫЪЭЮЯ";
static const char32_t* g_alphabet_russian_2 = U"абвгдеёжзийклмнопрстуфхцчшщьыъэюя";

static const char32_t* g_alphabet_german_1 = U"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜẞ";
static const char32_t* g_alphabet_german_2 = U"abcdefghijklmnopqrstuvwxyzäöüß";

static const char32_t* g_alphabet_french_1 = U"ABCDEFGHIJKLMNOPQRSTUVWXYZÀÂÆÈÉÊËÎÏÔŒÙÛÜŸÇ";
static const char32_t* g_alphabet_french_2 = U"abcdefghijklmnopqrstuvwxyzàâæèéêëîïôœùûüÿç";

static const char32_t* g_alphabet_italian_1 = U"ABCDEFGHILMNOPQRSTUVÀÈÉÌÍÏÒÓÙÚ";
static const char32_t* g_alphabet_italian_2 = U"abcdefghilmnopqrstuvàèéìíïòóùú";

static const char32_t* g_alphabet_spanish_1 = U"ABCDEFGHIJKLMNÑOPQRSTUVWXYZÁÉÍÓÚÜ";
static const char32_t* g_alphabet_spanish_2 = U"abcdefghijklmnñopqrstuvwxyzáéíóúü";

static const char32_t* g_alphabet_portuguese_1 = U"ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÉÊÍÒÓÔÕÚÜÇ";
static const char32_t* g_alphabet_portuguese_2 = U"abcdefghijklmnopqrstuvwxyzàáâãéêíòóôõúüç";

static const char32_t* g_short_month_english[] = {U"Jan", U"Feb", U"Mar", U"Apr", U"May", U"Jun",
                                                  U"Jul", U"Aug", U"Sep", U"Oct", U"Nov", U"Dec"};

static const char32_t* g_month_english[] = {U"January", U"February", U"March",     U"April",   U"May",      U"June",
                                            U"July",    U"August",   U"September", U"October", U"November", U"December"};

static const char32_t* g_short_month_russian[] = {U"Янв", U"Фев", U"Мар", U"Апр", U"Май", U"Июн",
                                                  U"Июл", U"Авг", U"Сен", U"Окт", U"Ноя", U"Дек"};

static const char32_t* g_month_russian[] = {U"Январь", U"Февраль", U"Март",     U"Апрель",  U"Май",    U"Июнь",
                                            U"Июль",   U"Август",  U"Сентябрь", U"Октябрь", U"Ноябрь", U"Декабрь"};

static const char32_t* g_short_day_english[] = {U"Mon", U"Tue", U"Wed", U"Thu", U"Fri", U"Sat", U"Sun"};

static const char32_t* g_day_english[] = {U"Monday", U"Tuesday", U"Wednesday", U"Thursday", U"Friday", U"Saturday", U"Sunday"};

static const char32_t* g_short_day_russian[] = {U"Пнд", U"Втн", U"Срд", U"Чтв", U"Птн", U"Сбт", U"Вск"};

static const char32_t* g_day_russian[] = {U"Понедельник", U"Вторник", U"Среда", U"Четверг", U"Пятница", U"Суббота", U"Воскресенье"};

Hashmap<String, LanguageId>* g_lang_map = nullptr;

void Language::Init()
{
	g_lang_map = new Hashmap<String, LanguageId>;

	g_lang_map->Put(U"de", LanguageId::German);
	g_lang_map->Put(U"en", LanguageId::English);
	g_lang_map->Put(U"fr", LanguageId::French);
	g_lang_map->Put(U"it", LanguageId::Italian);
	g_lang_map->Put(U"pt", LanguageId::Portuguese);
	g_lang_map->Put(U"ru", LanguageId::Russian);
	g_lang_map->Put(U"es", LanguageId::Spanish);
}



static String string_remove_duplicates(const String& s)
{
	Vector<char32_t> r;

	for (auto ch: s)
	{
		if (!r.Contains(ch))
		{
			r.Add(ch);
		}
	}

	r.Sort();

	r.Add(U'\0');

	return r.GetDataConst();
}

String Language::GetLettersList(LanguageId lang_id)
{
	String ret = U"";

	switch (lang_id)
	{
		case LanguageId::English:
			ret += g_alphabet_english_1;
			ret += g_alphabet_english_2;
			break;

		case LanguageId::Russian:
			ret += g_alphabet_russian_1;
			ret += g_alphabet_russian_2;
			break;

		case LanguageId::German:
			ret += g_alphabet_german_1;
			ret += g_alphabet_german_2;
			break;

		case LanguageId::French:
			ret += g_alphabet_french_1;
			ret += g_alphabet_french_2;
			break;

		case LanguageId::Italian:
			ret += g_alphabet_italian_1;
			ret += g_alphabet_italian_2;
			break;

		case LanguageId::Spanish:
			ret += g_alphabet_spanish_1;
			ret += g_alphabet_spanish_2;
			break;

		case LanguageId::Portuguese:
			ret += g_alphabet_portuguese_1;
			ret += g_alphabet_portuguese_2;
			break;

		case LanguageId::Unknown: EXIT("unknown language\n");
	}

	return string_remove_duplicates(ret);
}

String Language::GetLettersList(const String& id)
{
	return GetLettersList(GetId(id));
}

String Language::GetNumericList(const String& /*id*/)
{
	String ret = U"";

	ret += g_list_numeric;

	return string_remove_duplicates(ret);
}

String Language::GetPunctuationList(const String& id)
{
	String ret = U"";

	ret += g_list_punctuation;

	if (GetId(id) == LanguageId::Spanish)
	{
		ret += g_list_punctuation_2;
	}

	return string_remove_duplicates(ret);
}

String Language::GetCharList(const String& id)
{
	String ret = U"";

	ret += g_list_numeric;
	ret += g_list_punctuation;

	switch (GetId(id))
	{
		case LanguageId::English:
			ret += g_alphabet_english_1;
			ret += g_alphabet_english_2;
			break;

		case LanguageId::Russian:
			ret += g_alphabet_russian_1;
			ret += g_alphabet_russian_2;
			break;

		case LanguageId::German:
			ret += g_alphabet_german_1;
			ret += g_alphabet_german_2;
			break;

		case LanguageId::French:
			ret += g_alphabet_french_1;
			ret += g_alphabet_french_2;
			break;

		case LanguageId::Italian:
			ret += g_alphabet_italian_1;
			ret += g_alphabet_italian_2;
			break;

		case LanguageId::Spanish:
			ret += g_list_punctuation_2;
			ret += g_alphabet_spanish_1;
			ret += g_alphabet_spanish_2;
			break;

		case LanguageId::Portuguese:
			ret += g_alphabet_portuguese_1;
			ret += g_alphabet_portuguese_2;
			break;

		case LanguageId::Unknown: EXIT("unknown language\n");
	}

	return string_remove_duplicates(ret);
}

String Language::GetCharListAll()
{
	String ret = U"";

	ret += g_list_numeric;
	ret += g_list_punctuation;
	ret += g_list_punctuation_2;
	ret += g_alphabet_english_1;
	ret += g_alphabet_english_2;
	ret += g_alphabet_russian_1;
	ret += g_alphabet_russian_2;
	ret += g_alphabet_german_1;
	ret += g_alphabet_german_2;
	ret += g_alphabet_french_1;
	ret += g_alphabet_french_2;
	ret += g_alphabet_italian_1;
	ret += g_alphabet_italian_2;
	ret += g_alphabet_spanish_1;
	ret += g_alphabet_spanish_2;
	ret += g_alphabet_portuguese_1;
	ret += g_alphabet_portuguese_2;

	return string_remove_duplicates(ret);
}

String Language::GetNameOfMonth(int month, LanguageId lang_id)
{
	EXIT_IF(month < 1 || month > 12);

	switch (lang_id)
	{
		case LanguageId::English: return g_month_english[month - 1]; break;

		case LanguageId::Russian: return g_month_russian[month - 1]; break;

		case LanguageId::Unknown:
		case LanguageId::German:
		case LanguageId::French:
		case LanguageId::Italian:
		case LanguageId::Portuguese:
		case LanguageId::Spanish: EXIT("unknown language\n");
	}

	return U"";
}

String Language::GetNameOfMonthShort(int month, LanguageId lang_id)
{
	EXIT_IF(month < 1 || month > 12);

	switch (lang_id)
	{
		case LanguageId::English: return g_short_month_english[month - 1]; break;

		case LanguageId::Russian: return g_short_month_russian[month - 1]; break;

		case LanguageId::Unknown:
		case LanguageId::German:
		case LanguageId::French:
		case LanguageId::Italian:
		case LanguageId::Portuguese:
		case LanguageId::Spanish: EXIT("unknown language\n");
	}

	return U"";
}

String Language::GetNameOfDay(int day, LanguageId lang_id)
{
	EXIT_IF(day < 1 || day > 7);

	switch (lang_id)
	{
		case LanguageId::English: return g_day_english[day - 1]; break;

		case LanguageId::Russian: return g_day_russian[day - 1]; break;

		case LanguageId::Unknown:
		case LanguageId::German:
		case LanguageId::French:
		case LanguageId::Italian:
		case LanguageId::Portuguese:
		case LanguageId::Spanish: EXIT("unknown language\n");
	}

	return U"";
}

LanguageId Language::GetId(const String& id)
{
	return g_lang_map->Get(id, LanguageId::Unknown);
}

StringList Language::GetLanguages()
{
	StringList ret;

	FOR_HASH (*g_lang_map)
	{
		ret.Add(g_lang_map->Key());
	}

	return ret;
}

String Language::GetNameOfDayShort(int day, LanguageId lang_id)
{
	EXIT_IF(day < 1 || day > 7);

	switch (lang_id)
	{
		case LanguageId::English: return g_short_day_english[day - 1]; break;

		case LanguageId::Russian: return g_short_day_russian[day - 1]; break;

		case LanguageId::Unknown:
		case LanguageId::German:
		case LanguageId::French:
		case LanguageId::Italian:
		case LanguageId::Portuguese:
		case LanguageId::Spanish: EXIT("unknown language\n");
	}

	return U"";
}

} // namespace Kyty::Core
