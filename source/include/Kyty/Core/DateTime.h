#ifndef INCLUDE_KYTY_CORE_DATETIME_H_
#define INCLUDE_KYTY_CORE_DATETIME_H_

#include "Kyty/Core/Common.h"
#include "Kyty/Core/Language.h"
#include "Kyty/Core/String.h"

namespace Kyty::Core {

using jd_t = int32_t;

constexpr jd_t DATE_JD_INVALID = (INT32_MIN);
constexpr int  TIME_MS_INVALID = (-1);
constexpr int  TIME_MS_IN_DAY  = (24 * 3600 * 1000);

constexpr int MONTH_JANUARY   = 1;
constexpr int MONTH_FEBRUARY  = 2;
constexpr int MONTH_MARCH     = 3;
constexpr int MONTH_APRIL     = 4;
constexpr int MONTH_MAY       = 5;
constexpr int MONTH_JUNE      = 6;
constexpr int MONTH_JULY      = 7;
constexpr int MONTH_AUGUST    = 8;
constexpr int MONTH_SEPTEMBER = 9;
constexpr int MONTH_OCTOBER   = 10;
constexpr int MONTH_NOVEMBER  = 11;
constexpr int MONTH_DECEMBER  = 12;

class Date final
{
public:
	Date() = default;
	explicit Date(jd_t d): m_jd(d) {}
	Date(const Date& d)     = default;
	Date(Date&& d) noexcept = default;
	explicit Date(int year, int month, int day); // month in [1...12], day in [1...31]
	~Date() = default;

	static Date FromSystem();
	static Date FromSystemUTC();
	static Date FromMacros(const String& date);

	void Set(int year, int month, int day);
	void Set(jd_t jd) { this->m_jd = jd; }
	void Get(int* year, int* month, int* day) const;

	[[nodiscard]] bool IsInvalid() const { return m_jd == DATE_JD_INVALID; }
	[[nodiscard]] int  DaysInMonth() const;
	[[nodiscard]] int  DaysInYear() const;
	[[nodiscard]] bool IsLeapYear() const;
	[[nodiscard]] int  Year() const;
	[[nodiscard]] int  Month() const;
	[[nodiscard]] int  Day() const;
	[[nodiscard]] jd_t JulianDay() const { return m_jd; }
	[[nodiscard]] int  DayOfWeek() const;
	[[nodiscard]] int  DayOfYear() const;
	[[nodiscard]] int  QuarterOfYear() const;

	String ToString(const char* format = "YYYY.MM.DD", LanguageId lang_id = LanguageId::English) const;

	static bool IsValid(int year, int month, int day);
	static int  DaysInMonth(int month);
	static int  DaysInYear(int year);
	static bool IsLeapYear(int year);

	bool operator==(const Date& other) const { return m_jd == other.m_jd; }
	bool operator!=(const Date& other) const { return m_jd != other.m_jd; }
	bool operator<(const Date& other) const { return m_jd < other.m_jd; }
	bool operator<=(const Date& other) const { return m_jd <= other.m_jd; }
	bool operator>(const Date& other) const { return m_jd > other.m_jd; }
	bool operator>=(const Date& other) const { return m_jd >= other.m_jd; }

	Date& operator=(const Date& other)     = default;
	Date& operator=(Date&& other) noexcept = default;
	Date  operator+(int days) const { return Date(m_jd + days); }
	Date  operator-(int days) const { return Date(m_jd - days); }
	Date  operator+=(int days)
	{
		m_jd += days;
		return *this;
	}
	Date operator-=(int days)
	{
		m_jd -= days;
		return *this;
	}

private:
	jd_t m_jd {DATE_JD_INVALID};
};

class Time final
{
public:
	Time() = default;
	explicit Time(int msec): m_ms(msec)
	{
		if (m_ms < 0 || m_ms >= TIME_MS_IN_DAY)
		{
			m_ms = TIME_MS_INVALID;
		}
	}
	Time(const Time& t)     = default;
	Time(Time&& t) noexcept = default;
	explicit Time(int hour24, int minute, int second, int msec = 0);
	~Time() = default;

	static Time FromSystem();
	static Time FromSystemUTC();

	void Set(int hour24, int minute, int second, int msec = 0);
	void Set(int msec) { m_ms = msec; }
	void Get(int* hour24, int* minute, int* second, int* msec = nullptr) const;

	[[nodiscard]] bool IsInvalid() const { return m_ms < 0 || m_ms >= TIME_MS_IN_DAY; }
	[[nodiscard]] int  Hour12() const;
	[[nodiscard]] int  Hour24() const;
	[[nodiscard]] bool IsAM() const;
	[[nodiscard]] bool IsPM() const;
	[[nodiscard]] int  Minute() const;
	[[nodiscard]] int  Second() const;
	[[nodiscard]] int  Msec() const;
	[[nodiscard]] int  MsecTotal() const { return m_ms; }

	String ToString(const char* format = "HH24:MI:SS") const;

	static bool IsValid(int hour, int minute, int second, int msec = 0);

	bool operator==(const Time& other) const { return m_ms == other.m_ms; }
	bool operator!=(const Time& other) const { return m_ms != other.m_ms; }
	bool operator<(const Time& other) const { return m_ms < other.m_ms; }
	bool operator<=(const Time& other) const { return m_ms <= other.m_ms; }
	bool operator>(const Time& other) const { return m_ms > other.m_ms; }
	bool operator>=(const Time& other) const { return m_ms >= other.m_ms; }

	Time& operator=(const Time& other)     = default;
	Time& operator=(Time&& other) noexcept = default;
	Time  operator+(int secs) const;
	Time  operator-(int secs) const;
	Time  operator+=(int secs);
	Time  operator-=(int secs);

private:
	int m_ms {TIME_MS_INVALID};
};

class DateTime final
{
public:
	DateTime() = default;
	explicit DateTime(const Date& d): m_date(d) {}
	explicit DateTime(const Time& t): m_time(t) {}
	explicit DateTime(const Date& d, const Time& t): m_date(d), m_time(t) {}
	explicit DateTime(const Time& t, const Date& d): m_date(d), m_time(t) {}
	DateTime(const DateTime& dt)     = default;
	DateTime(DateTime&& dt) noexcept = default;
	~DateTime()                      = default;

	static DateTime FromSystem();
	static DateTime FromSystemUTC();

	static DateTime       FromSQLiteJulian(double jd);
	[[nodiscard]] double  ToSQLiteJulian() const;
	[[nodiscard]] int64_t ToSQLiteJulianInt64() const;
	static DateTime       FromUnix(double seconds);
	[[nodiscard]] double  ToUnix() const;

	[[nodiscard]] uint64_t DistanceMs(const DateTime& other) const;

	[[nodiscard]] bool IsInvalid() const { return m_date.IsInvalid() || m_time.IsInvalid(); }

	void                      SetDate(const Date& d) { m_date = d; }
	void                      SetTime(const Time& t) { m_time = t; }
	Date&                     GetDate() { return m_date; }
	Time&                     GetTime() { return m_time; }
	[[nodiscard]] const Date& GetDate() const { return m_date; }
	[[nodiscard]] const Time& GetTime() const { return m_time; }

	String ToString(const char* format = "YYYY.MM.DD HH24:MI:SS", LanguageId lang_id = LanguageId::English) const;

	DateTime& operator=(const DateTime& other)     = default;
	DateTime& operator=(DateTime&& other) noexcept = default;

	bool operator==(const DateTime& other) const { return m_date == other.m_date && m_time == other.m_time; }
	bool operator!=(const DateTime& other) const { return !(*this == other); }
	bool operator<(const DateTime& other) const { return m_date < other.m_date || (m_date == other.m_date && m_time < other.m_time); }
	bool operator<=(const DateTime& other) const { return !(other < *this); }
	bool operator>(const DateTime& other) const { return other < *this; }
	bool operator>=(const DateTime& other) const { return !(*this < other); }

private:
	Date m_date;
	Time m_time;
};

} // namespace Kyty::Core

#endif /* INCLUDE_KYTY_CORE_DATETIME_H_ */
