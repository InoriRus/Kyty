#include "Kyty/Core/DateTime.h"

#include "Kyty/Core/DbgAssert.h"
#include "Kyty/Math/MathAll.h"
#include "Kyty/Sys/SysTimer.h"

namespace Kyty::Core {

static void ymd_to_jd(int year, int month, int day, jd_t* jd)
{
	jd_t t_year  = year;
	jd_t t_month = month;
	jd_t t_day   = day;

	if (t_year < 0)
	{
		t_year++;
	}

	jd_t a = Math::floordiv<jd_t>(14 - t_month, 12);
	jd_t y = t_year + 4800 - a;
	jd_t m = t_month + 12 * a - 3;
	*jd    = t_day + Math::floordiv<jd_t>(153 * m + 2, 5) + 365 * y + Math::floordiv<jd_t>(y, 4) - Math::floordiv<jd_t>(y, 100) +
	      Math::floordiv<jd_t>(y, 400) - 32045;
}

static void jd_to_ymd(int* year, int* month, int* day, jd_t jd)
{
	jd_t a = jd + 32044;
	jd_t b = Math::floordiv<jd_t>(4 * a + 3, 146097);
	jd_t c = a - Math::floordiv<jd_t>(146097 * b, 4);
	jd_t d = Math::floordiv<jd_t>(4 * c + 3, 1461);
	jd_t e = c - Math::floordiv<jd_t>(1461 * d, 4);
	jd_t m = Math::floordiv<jd_t>(5 * e + 2, 153);

	int t_day   = e - Math::floordiv<jd_t>(153 * m + 2, 5) + 1;
	int t_month = m + 3 - 12 * Math::floordiv<jd_t>(m, 10);
	int t_year  = 100 * b + d - 4800 + Math::floordiv<jd_t>(m, 10);

	if (t_year <= 0)
	{
		t_year--;
	}

	if (year != nullptr)
	{
		*year = t_year;
	}
	if (month != nullptr)
	{
		*month = t_month;
	}
	if (day != nullptr)
	{
		*day = t_day;
	}
}

static void hms_to_ms(int hour, int minute, int second, int msec, int* ms)
{
	*ms = hour * 60 * 60 * 1000 + minute * 60 * 1000 + second * 1000 + msec;
}

static void ms_to_hms(int* hour, int* minute, int* second, int* msec, int ms)
{
	if (hour != nullptr)
	{
		*hour = ms / (60 * 60 * 1000);
	}
	if (minute != nullptr)
	{
		*minute = (ms % (60 * 60 * 1000)) / (60 * 1000);
	}
	if (second != nullptr)
	{
		*second = (ms / 1000) % 60;
	}
	if (msec != nullptr)
	{
		*msec = ms % 1000;
	}
}

Date::Date(int year, int month, int day)
{
	Set(year, month, day);
}

bool Date::IsValid(int year, int month, int day)
{
	if (year == 0)
	{
		return false;
	}

	return (day >= 1) && (day <= DaysInMonth(month) || (day == 29 && month == 2 && IsLeapYear(year)));
}

int Date::DaysInMonth() const
{
	if (IsInvalid())
	{
		return 0;
	}

	int year  = 0;
	int month = 0;
	jd_to_ymd(&year, &month, nullptr, m_jd);

	if (month == 2 && IsLeapYear(year))
	{
		return 29;
	}

	return DaysInMonth(month);
}

bool Date::IsLeapYear() const
{
	if (IsInvalid())
	{
		return false;
	}

	int year = 0;
	jd_to_ymd(&year, nullptr, nullptr, m_jd);

	return IsLeapYear(year);
}

int Date::DaysInMonth(int month)
{
	static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (month < 1 || month > 12)
	{
		month = 0;
	}

	return days[month];
}

void Date::Set(int year, int month, int day)
{
	if (IsValid(year, month, day))
	{
		ymd_to_jd(year, month, day, &m_jd);
	} else
	{
		m_jd = DATE_JD_INVALID;
	}
}

void Date::Get(int* year, int* month, int* day) const
{
	if (IsInvalid())
	{
		if (year != nullptr)
		{
			*year = 0;
		}
		if (month != nullptr)
		{
			*month = 0;
		}
		if (day != nullptr)
		{
			*day = 0;
		}
	} else
	{
		jd_to_ymd(year, month, day, m_jd);
	}
}

bool Date::IsLeapYear(int year)
{
	if (year < 1)
	{
		year++;
	}

	return ((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0;
}

int Date::Year() const
{
	if (IsInvalid())
	{
		return 0;
	}

	int year = 0;
	jd_to_ymd(&year, nullptr, nullptr, m_jd);

	return year;
}

int Date::Month() const
{
	if (IsInvalid())
	{
		return 0;
	}

	int month = 0;
	jd_to_ymd(nullptr, &month, nullptr, m_jd);

	return month;
}

int Date::Day() const
{
	if (IsInvalid())
	{
		return 0;
	}

	int day = 0;
	jd_to_ymd(nullptr, nullptr, &day, m_jd);

	return day;
}

int Date::DaysInYear() const
{
	if (IsInvalid())
	{
		return 0;
	}

	return IsLeapYear() ? 366 : 365;
}

int Date::DayOfWeek() const
{
	if (IsInvalid())
	{
		return 0;
	}

	if (m_jd >= 0)
	{
		return (m_jd % 7) + 1;
	}

	return ((m_jd + 1) % 7) + 7;
}

int Date::DayOfYear() const
{
	if (IsInvalid())
	{
		return 0;
	}

	jd_t d = 0;
	ymd_to_jd(Year(), 1, 1, &d);

	return m_jd - d + 1;
}

int Date::DaysInYear(int year)
{
	return IsLeapYear(year) ? 366 : 365;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static bool format_date(const Date* d, const String& f, uint32_t* out_i, String* out_r, LanguageId lang_id)
{
	auto& i = *out_i;
	auto& r = *out_r;
	if (f.Mid(i, 4) == U"YYYY")
	{
		int y = d->Year();
		EXIT_IF(y < 0 || y > 9999);
		r += String::FromPrintf("%04d", y);
		i += 3;
	} else if (f.Mid(i, 3) == U"YYY")
	{
		int y = d->Year();
		EXIT_IF(y < 0 || y > 9999);
		r += String::FromPrintf("%04d", y).Mid(1);
		i += 2;
	} else if (f.Mid(i, 2) == U"YY")
	{
		int y = d->Year();
		EXIT_IF(y < 0 || y > 9999);
		r += String::FromPrintf("%04d", y).Mid(2);
		i += 1;
	} else if (f.Mid(i, 1) == U"Y")
	{
		int y = d->Year();
		EXIT_IF(y < 0 || y > 9999);
		r += String::FromPrintf("%04d", y).Mid(3);
	} else if (f.Mid(i, 1) == U"Q")
	{
		r += String::FromPrintf("%d", d->QuarterOfYear());
	} else if (f.Mid(i, 2) == U"MM")
	{
		r += String::FromPrintf("%02d", d->Month());
		i += 1;
	} else if (f.Mid(i, 5) == U"MONTH")
	{
		r += Language::GetNameOfMonth(d->Month(), lang_id);
		i += 4;
	} else if (f.Mid(i, 3) == U"MON")
	{
		r += Language::GetNameOfMonthShort(d->Month(), lang_id);
		i += 2;
	} else if (f.Mid(i, 3) == U"DDD")
	{
		r += String::FromPrintf("%03d", d->DayOfYear());
		i += 2;
	} else if (f.Mid(i, 2) == U"DD")
	{
		r += String::FromPrintf("%02d", d->Day());
		i += 1;
	} else if (f.Mid(i, 2) == U"DY")
	{
		r += Language::GetNameOfDayShort(d->DayOfWeek(), lang_id);
		i += 1;
	} else if (f.Mid(i, 3) == U"DAY")
	{
		r += Language::GetNameOfDay(d->DayOfWeek(), lang_id);
		i += 2;
	} else if (f.Mid(i, 1) == U"D")
	{
		r += String::FromPrintf("%d", d->DayOfWeek());
	} else if (f.Mid(i, 1) == U"J")
	{
		r += String::FromPrintf("%d", d->JulianDay());
	} else
	{
		return false;
	}

	return true;
}

/**
 * YYYY       - 4-digit year
 * YYY, YY, Y - Last 3, 2, or 1 digit(s) of year.
 * Q          - Quarter of year (1, 2, 3, 4; JAN-MAR = 1).
 * MM         - Month (01-12; JAN = 01).
 * MON        - Abbreviated name of month.
 * MONTH      - Name of month
 * D          - Day of week (1-7).
 * DAY        - Name of day.
 * DY         - Abbreviated name of day.
 * DD         - Day of month (01-31).
 * DDD        - Day of year (001-366).
 * J          - Julian day
 */
String Date::ToString(const char* format, LanguageId lang_id) const
{
	if (IsInvalid())
	{
		return U"";
	}

	String r;
	String f = String::FromUtf8(format);

	FOR (i, f)
	{
		if (!format_date(this, f, &i, &r, lang_id))
		{
			r += f.At(i);
		}
	}

	return r;
}

int Date::QuarterOfYear() const
{
	if (IsInvalid())
	{
		return 0;
	}

	return (Month() - 1) / 3 + 1;
}

Date Date::FromMacros(const String& date)
{
	StringList lst = date.Split(U" ", String::SplitType::SplitNoEmptyParts);

	if (lst.Size() != 3)
	{
		return {};
	}

	static const char* month_str[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	int month = -1;
	for (int i = 0; i < 12; i++)
	{
		if (lst.At(0).EqualAscii(month_str[i]))
		{
			month = i + 1;
			break;
		}
	}

	if (month < 0)
	{
		return {};
	}

	return Date(lst.At(2).ToInt32(), month, lst.At(1).ToInt32());
}

Date Date::FromSystem()
{
	SysTimeStruct t {};
	sys_get_system_time(t);

	return Date(t.Year, t.Month, t.Day);
}

Date Date::FromSystemUTC()
{
	SysTimeStruct t {};
	sys_get_system_time_utc(t);

	return Date(t.Year, t.Month, t.Day);
}

Time::Time(int hour, int minute, int second, int msec)
{
	Set(hour, minute, second, msec);
}

Time Time::FromSystem()
{
	SysTimeStruct t {};
	sys_get_system_time(t);

	return Time(t.Hour, t.Minute, t.Second, t.Milliseconds);
}

Time Time::FromSystemUTC()
{
	SysTimeStruct t {};
	sys_get_system_time_utc(t);

	return Time(t.Hour, t.Minute, t.Second, t.Milliseconds);
}

void Time::Set(int hour, int minute, int second, int msec)
{
	if (IsValid(hour, minute, second, msec))
	{
		hms_to_ms(hour, minute, second, msec, &m_ms);
	} else
	{
		m_ms = TIME_MS_INVALID;
	}
}

void Time::Get(int* hour, int* minute, int* second, int* msec) const
{
	if (IsInvalid())
	{
		if (hour != nullptr)
		{
			*hour = -1;
		}
		if (minute != nullptr)
		{
			*minute = -1;
		}
		if (second != nullptr)
		{
			*second = -1;
		}
		if (msec != nullptr)
		{
			*msec = -1;
		}
	} else
	{
		ms_to_hms(hour, minute, second, msec, m_ms);
	}
}

bool Time::IsValid(int hour, int minute, int second, int msec)
{
	return (hour >= 0 && hour <= 23) && (minute >= 0 && minute <= 59) && (second >= 0 && second <= 59) && (msec >= 0 && msec <= 999);
}

int Time::Hour24() const
{
	if (IsInvalid())
	{
		return -1;
	}

	int h = 0;
	ms_to_hms(&h, nullptr, nullptr, nullptr, m_ms);

	return h;
}

int Time::Hour12() const
{
	if (IsInvalid())
	{
		return -1;
	}

	int h = 0;
	ms_to_hms(&h, nullptr, nullptr, nullptr, m_ms);

	h = h % 12;

	return h == 0 ? 12 : h;
}

int Time::Minute() const
{
	if (IsInvalid())
	{
		return -1;
	}

	int m = 0;
	ms_to_hms(nullptr, &m, nullptr, nullptr, m_ms);

	return m;
}

int Time::Second() const
{
	if (IsInvalid())
	{
		return -1;
	}

	int s = 0;
	ms_to_hms(nullptr, nullptr, &s, nullptr, m_ms);

	return s;
}

int Time::Msec() const
{
	if (IsInvalid())
	{
		return -1;
	}

	int m = 0;
	ms_to_hms(nullptr, nullptr, nullptr, &m, m_ms);

	return m;
}

bool Time::IsAM() const
{
	if (IsInvalid())
	{
		return false;
	}

	int h = 0;
	ms_to_hms(&h, nullptr, nullptr, nullptr, m_ms);

	return h < 12;
}

bool Time::IsPM() const
{
	if (IsInvalid())
	{
		return false;
	}

	int h = 0;
	ms_to_hms(&h, nullptr, nullptr, nullptr, m_ms);

	return h >= 12;
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static bool format_time(const Time* t, const String& f, uint32_t* out_i, String* out_r)
{
	auto& i = *out_i;
	auto& r = *out_r;
	if (f.Mid(i, 4) == U"HH24")
	{
		r += String::FromPrintf("%02d", t->Hour24());
		i += 3;
	} else if (f.Mid(i, 4) == U"HH12")
	{
		r += String::FromPrintf("%02d", t->Hour12());
		i += 3;
	} else if (f.Mid(i, 2) == U"HH")
	{
		r += String::FromPrintf("%02d", t->Hour12());
		i += 1;
	} else if (f.Mid(i, 2) == U"MI")
	{
		r += String::FromPrintf("%02d", t->Minute());
		i += 1;
	} else if (f.Mid(i, 5) == U"SSSSS")
	{
		r += String::FromPrintf("%05d", t->MsecTotal() / 1000);
		i += 4;
	} else if (f.Mid(i, 2) == U"SS")
	{
		r += String::FromPrintf("%02d", t->Second());
		i += 1;
	} else if (f.Mid(i, 3) == U"FFF")
	{
		r += String::FromPrintf("%03d", t->Msec());
		i += 2;
	} else if (f.Mid(i, 2) == U"AM")
	{
		r += t->IsAM() ? U"AM" : U"PM";
		i += 1;
	} else if (f.Mid(i, 4) == U"A.M.")
	{
		r += t->IsAM() ? U"A.M." : U"P.M.";
		i += 3;
	} else
	{
		return false;
	}

	return true;
}

/**
 * HH    - Hour of day (1-12)
 * HH12  - Hour of day (1-12)
 * HH24  - Hour of day (0-23)
 * MI    - Minute (0-59)
 * SS    - Second (0-59)
 * SSSSS - Seconds past midnight (0-86399)
 * FFF   - Milliseconds
 * AM    - AM or PM
 * A.M.  - A.M. or P.M.
 */
String Time::ToString(const char* format) const
{
	if (IsInvalid())
	{
		return U"";
	}

	String r;
	String f = String::FromUtf8(format);

	FOR (i, f)
	{
		if (!format_time(this, f, &i, &r))
		{
			r += f.At(i);
		}
	}

	return r;
}

Time Time::operator+(int secs) const
{
	int ms_secs = secs * 1000;

	EXIT_IF(ms_secs > TIME_MS_IN_DAY || ms_secs < -TIME_MS_IN_DAY);

	if (IsInvalid())
	{
		return {};
	}

	int r = m_ms + ms_secs;

	if (r < 0)
	{
		r += TIME_MS_IN_DAY;
	}
	if (r >= TIME_MS_IN_DAY)
	{
		r -= TIME_MS_IN_DAY;
	}

	return Time(r);
}

Time Time::operator-(int secs) const
{
	int ms_secs = secs * 1000;

	EXIT_IF(ms_secs > TIME_MS_IN_DAY || ms_secs < -TIME_MS_IN_DAY);

	if (IsInvalid())
	{
		return {};
	}

	int r = m_ms - ms_secs;

	if (r < 0)
	{
		r += TIME_MS_IN_DAY;
	}
	if (r >= TIME_MS_IN_DAY)
	{
		r -= TIME_MS_IN_DAY;
	}

	return Time(r);
}

Time Time::operator+=(int secs)
{
	int ms_secs = secs * 1000;

	EXIT_IF(ms_secs > TIME_MS_IN_DAY || ms_secs < -TIME_MS_IN_DAY);

	if (!IsInvalid())
	{
		m_ms += ms_secs;

		if (m_ms < 0)
		{
			m_ms += TIME_MS_IN_DAY;
		}
		if (m_ms >= TIME_MS_IN_DAY)
		{
			m_ms -= TIME_MS_IN_DAY;
		}
	}

	return *this;
}

Time Time::operator-=(int secs)
{
	int ms_secs = secs * 1000;

	EXIT_IF(ms_secs > TIME_MS_IN_DAY || ms_secs < -TIME_MS_IN_DAY);

	if (!IsInvalid())
	{
		m_ms -= ms_secs;

		if (m_ms < 0)
		{
			m_ms += TIME_MS_IN_DAY;
		}
		if (m_ms >= TIME_MS_IN_DAY)
		{
			m_ms -= TIME_MS_IN_DAY;
		}
	}

	return *this;
}

DateTime DateTime::FromSystem()
{
	SysTimeStruct t {};
	sys_get_system_time(t);

	if (t.is_invalid)
	{
		return {};
	}

	return DateTime(Date(t.Year, t.Month, t.Day), Time(t.Hour, t.Minute, t.Second, t.Milliseconds));
}

DateTime DateTime::FromSystemUTC()
{
	SysTimeStruct t {};
	sys_get_system_time_utc(t);

	if (t.is_invalid)
	{
		return {};
	}

	return DateTime(Date(t.Year, t.Month, t.Day), Time(t.Hour, t.Minute, t.Second, t.Milliseconds));
}

/**
 * YYYY       - 4-digit year
 * YYY, YY, Y - Last 3, 2, or 1 digit(s) of year.
 * Q          - Quarter of year (1, 2, 3, 4; JAN-MAR = 1).
 * MM         - Month (01-12; JAN = 01).
 * MON        - Abbreviated name of month.
 * MONTH      - Name of month
 * D          - Day of week (1-7).
 * DAY        - Name of day.
 * DY         - Abbreviated name of day.
 * DD         - Day of month (01-31).
 * DDD        - Day of year (001-366).
 * J          - Julian day
 * HH         - Hour of day (1-12)
 * HH12       - Hour of day (1-12)
 * HH24       - Hour of day (0-23)
 * MI         - Minute (0-59)
 * SS         - Second (0-59)
 * SSSSS      - Seconds past midnight (0-86399)
 * FFF        - Milliseconds
 * AM         - AM or PM
 * A.M.       - A.M. or P.M.
 */
String DateTime::ToString(const char* format, LanguageId lang_id) const
{
	if (IsInvalid())
	{
		return U"";
	}

	String r;
	String f = String::FromUtf8(format);

	FOR (i, f)
	{
		if (!format_date(&m_date, f, &i, &r, lang_id) && !format_time(&m_time, f, &i, &r))
		{
			r += f.At(i);
		}
	}

	return r;
}

uint64_t DateTime::DistanceMs(const DateTime& other) const
{
	EXIT_IF(IsInvalid() || other.IsInvalid());

	if (other == *this)
	{
		return 0;
	}

	if (other > *this)
	{
		return other.DistanceMs(*this);
	}

	jd_t j1 = other.m_date.JulianDay();
	jd_t j2 = m_date.JulianDay();

	return static_cast<int64_t>(j2 - j1 - 1) * static_cast<int64_t>(TIME_MS_IN_DAY) +
	       (static_cast<int64_t>(TIME_MS_IN_DAY) - static_cast<int64_t>(other.m_time.MsecTotal())) +
	       static_cast<int64_t>(m_time.MsecTotal());
}

DateTime DateTime::FromSQLiteJulian(double jd)
{
	double i = NAN;
	double f = modf(jd + 0.5, &i);
	return DateTime(Date(static_cast<jd_t>(i)), Time(static_cast<int>(f * static_cast<double>(TIME_MS_IN_DAY))));
}

double DateTime::ToSQLiteJulian() const
{
	return -0.5 + static_cast<double>(GetDate().JulianDay()) +
	       static_cast<double>(GetTime().MsecTotal()) / static_cast<double>(TIME_MS_IN_DAY);
}

int64_t DateTime::ToSQLiteJulianInt64() const
{
	return -(static_cast<int64_t>(TIME_MS_IN_DAY)) / 2 +
	       static_cast<int64_t>(GetDate().JulianDay()) * (static_cast<int64_t>(TIME_MS_IN_DAY)) +
	       static_cast<int64_t>(GetTime().MsecTotal());
}

DateTime DateTime::FromUnix(double seconds)
{
	return FromSQLiteJulian(2440587.5 + seconds / 86400.0);
}

double DateTime::ToUnix() const
{
	return (ToSQLiteJulian() - 2440587.5) * 86400.0;
}

} // namespace Kyty::Core
