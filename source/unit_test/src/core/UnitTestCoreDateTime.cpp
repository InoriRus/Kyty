#include "Kyty/Core/DateTime.h"
#include "Kyty/Core/Language.h"
#include "Kyty/Core/String.h"
#include "Kyty/UnitTest.h"

#include <cmath>
#include <cstdlib>

UT_BEGIN(CoreDateTime);

namespace Language = Core::Language;
using Core::Date;
using Core::DateTime;
using Core::jd_t;
using Core::Time;

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct TestDateT
{
	int  year;
	int  month;
	int  day;
	bool Invalid;       // NOLINT(readability-identifier-naming)
	int  Day;           // NOLINT(readability-identifier-naming)
	int  Month;         // NOLINT(readability-identifier-naming)
	int  Year;          // NOLINT(readability-identifier-naming)
	jd_t Jd;            // NOLINT(readability-identifier-naming)
	int  DaysInMonth;   // NOLINT(readability-identifier-naming)
	bool LeapYear;      // NOLINT(readability-identifier-naming)
	int  DaysInYear;    // NOLINT(readability-identifier-naming)
	int  DayOfWeek;     // NOLINT(readability-identifier-naming)
	int  DayOfYear;     // NOLINT(readability-identifier-naming)
	int  QuarterOfYear; // NOLINT(readability-identifier-naming)
};

// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct TestTimeT
{
	int  hour;
	int  minute;
	int  second;
	int  msec;
	bool Invalid; // NOLINT(readability-identifier-naming)
	int  Hour24;  // NOLINT(readability-identifier-naming)
	int  Hour12;  // NOLINT(readability-identifier-naming)
	bool IsAM;    // NOLINT(readability-identifier-naming)
	bool IsPM;    // NOLINT(readability-identifier-naming)
	int  Minute;  // NOLINT(readability-identifier-naming)
	int  Second;  // NOLINT(readability-identifier-naming)
	int  Msec;    // NOLINT(readability-identifier-naming)
	int  Ms;      // NOLINT(readability-identifier-naming)
};

static TestDateT g_data_date[] = {{-100, 1, 21, false, 21, 1, -100, 1684921, 31, false, 365, 1, 21, 1},
                                  {-1, 2, 21, false, 21, 2, -1, 1721111, 29, true, 366, 1, 52, 1},
                                  {0, 3, 21, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {1, 4, 21, false, 21, 4, 1, 1721536, 30, false, 365, 6, 111, 2},
                                  {100, 5, 21, false, 21, 5, 100, 1757725, 31, false, 365, 5, 141, 2},
                                  {2000, 6, 30, false, 30, 6, 2000, 2451726, 30, true, 366, 5, 182, 2},
                                  {2004, 7, 21, false, 21, 7, 2004, 2453208, 31, true, 366, 3, 203, 3},
                                  {2100, 8, 21, false, 21, 8, 2100, 2488302, 31, false, 365, 6, 233, 3},
                                  {2004, 0, 21, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {2004, 13, 21, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {2004, -1, 21, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {2000, 6, 0, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {2000, 6, -1, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0},
                                  {2000, 6, 31, true, 0, 0, 0, Core::DATE_JD_INVALID, 0, false, 0, 0, 0, 0}};
//#define data_date_num (int)(sizeof(data_date) / sizeof(data_date[0]))

static TestTimeT g_data_time[] = {
    {10, 20, 30, 40, false, 10, 10, true, false, 20, 30, 40, 37230040},
    {20, 20, 30, 40, false, 20, 8, false, true, 20, 30, 40, 73230040},
    {13, 20, 30, 40, false, 13, 1, false, true, 20, 30, 40, 48030040},
    {12, 20, 30, 40, false, 12, 12, false, true, 20, 30, 40, 44430040},
    {-1, 20, 30, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {0, 20, 30, 40, false, 0, 12, true, false, 20, 30, 40, 1230040},
    {24, 20, 30, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, -1, 30, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, 0, 30, 40, false, 10, 10, true, false, 0, 30, 40, 36030040},
    {10, 60, 30, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, 20, -1, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, 20, 0, 40, false, 10, 10, true, false, 20, 0, 40, 37200040},
    {10, 20, 60, 40, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, 20, 30, -1, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
    {10, 20, 30, 0, false, 10, 10, true, false, 20, 30, 0, 37230000},
    {10, 20, 30, 1000, true, -1, -1, false, false, -1, -1, -1, Core::TIME_MS_INVALID},
};
//#define data_time_num (int)(sizeof(data_time) / sizeof(data_time[0]))

static void test_date()
{
	Date invalid;
	EXPECT_TRUE(invalid.IsInvalid());
	EXPECT_EQ(invalid.Day(), 0);
	EXPECT_EQ(invalid.Month(), 0);
	EXPECT_EQ(invalid.Year(), 0);
	EXPECT_EQ(invalid.JulianDay(), (jd_t)Core::DATE_JD_INVALID);
	EXPECT_EQ(invalid.DaysInMonth(), 0);
	EXPECT_FALSE(invalid.IsLeapYear());

	for (auto& i: g_data_date)
	{
		Date d1(i.year, i.month, i.day);
		Date d2;
		d2.Set(i.year, i.month, i.day);
		Date d3(i.Jd);
		Date d4;
		d4.Set(i.Jd);

		EXPECT_EQ(d1, d2);
		EXPECT_EQ(d1, d3);
		EXPECT_EQ(d1, d4);

		EXPECT_EQ(d1.IsInvalid(), i.Invalid);
		EXPECT_EQ(d1.Day(), i.Day);
		EXPECT_EQ(d1.Month(), i.Month);
		EXPECT_EQ(d1.Year(), i.Year);
		EXPECT_EQ(d1.JulianDay(), i.Jd);
		EXPECT_EQ(d1.DaysInMonth(), i.DaysInMonth);
		EXPECT_EQ(d1.IsLeapYear(), i.LeapYear);
		EXPECT_EQ(d1.DaysInYear(), i.DaysInYear);
		EXPECT_EQ(d1.DayOfWeek(), i.DayOfWeek);
		EXPECT_EQ(d1.DayOfYear(), i.DayOfYear);
		EXPECT_EQ(d1.QuarterOfYear(), i.QuarterOfYear);

		int y = 0;
		int m = 0;
		int d = 0;
		d4.Get(&y, &m, &d);
		EXPECT_EQ(d, i.Day);
		EXPECT_EQ(m, i.Month);
		EXPECT_EQ(y, i.Year);
	}

	EXPECT_EQ(Date::DaysInMonth(Core::MONTH_SEPTEMBER), 30);
	EXPECT_TRUE(Date::IsLeapYear(2016));
	EXPECT_FALSE(Date::IsLeapYear(1981));
	EXPECT_EQ(Date::DaysInYear(2016), 366);
	EXPECT_EQ(Date::DaysInYear(1981), 365);
	EXPECT_TRUE(Date::IsValid(2016, 1, 28));
	EXPECT_FALSE(Date::IsValid(2016, 28, 1));

	EXPECT_TRUE(Date(1981, 4, 21) == Date(1981, 4, 21));
	EXPECT_TRUE(Date(1981, 4, 21) != Date(1981, 4, 22));
	EXPECT_TRUE(Date(1981, 4, 21) < Date(1982, 4, 21));
	EXPECT_TRUE(Date(1981, 4, 21) <= Date(1981, 4, 21));
	EXPECT_TRUE(Date(1981, 4, 21) <= Date(1982, 4, 21));
	EXPECT_TRUE(Date(1981, 4, 21) > Date(1981, 4, 20));
	EXPECT_TRUE(Date(1981, 4, 21) >= Date(1981, 4, 21));
	EXPECT_TRUE(Date(1981, 4, 21) >= Date(1981, 4, 20));

	EXPECT_EQ(Date(1981, 4, 21).ToString(), String(U"1981.04.21"));

	EXPECT_EQ(Date(1981, 4, 21).ToString("YYYY YYY YY Y Q MM MON MONTH D DAY DY DD DDD J"),
	          U"1981 981 81 1 2 04 Apr April 2 Tuesday Tue 21 111 2444716");

	EXPECT_EQ(Date(1981, 4, 21).ToString("YYYY YYY YY Y Q MM MON MONTH D DAY DY DD DDD J", Language::GetId(U"ru")),
	          U"1981 981 81 1 2 04 Апр Апрель 2 Вторник Втн 21 111 2444716");

	Date s(1981, 4, 21);
	s += 20;
	s -= 10;
	Date ss;
	ss = s;
	EXPECT_EQ(ss, Date(1981, 5, 1));
	EXPECT_EQ(ss - 10, Date(1981, 4, 21));
	EXPECT_EQ(ss + 10, Date(1981, 5, 11));

	Date s1 = Date::FromSystem();
	Date s2 = Date::FromSystemUTC();

	EXPECT_TRUE(s1 < Date(2023, 1, 1));
	EXPECT_TRUE(s1 > Date(2021, 12, 31));
	EXPECT_TRUE(s2 < Date(2023, 1, 1));
	EXPECT_TRUE(s2 > Date(2021, 12, 31));

	EXPECT_TRUE(!Date::FromMacros(U"" __DATE__).IsInvalid());
	EXPECT_EQ(Date::FromMacros(U"Dec 07 2017"), Date(2017, 12, 7));

	// printf("sysdate (loc) = %s\n", s1.ToString().C_Str());
	// printf("sysdate (utc) = %s\n", s2.ToString().C_Str());
}

static void test_time()
{
	Time invalid;
	EXPECT_TRUE(invalid.IsInvalid());

	Time invalid2(24 * 3600 * 1000 + 1);
	EXPECT_TRUE(invalid2.IsInvalid());

	Time invalid3(-1);
	EXPECT_TRUE(invalid3.IsInvalid());

	for (auto& i: g_data_time)
	{
		Time t1(i.hour, i.minute, i.second, i.msec);
		Time t2;
		t2.Set(i.hour, i.minute, i.second, i.msec);
		Time t3(i.Ms);
		Time t4;
		t4.Set(i.Ms);

		EXPECT_EQ(t1, t2);
		EXPECT_EQ(t1, t3);
		EXPECT_EQ(t1, t4);

		EXPECT_EQ(t1.IsInvalid(), i.Invalid);
		EXPECT_EQ(t1.Hour24(), i.Hour24);
		EXPECT_EQ(t1.Hour12(), i.Hour12);
		EXPECT_EQ(t1.IsAM(), i.IsAM);
		EXPECT_EQ(t1.IsPM(), i.IsPM);
		EXPECT_EQ(t1.Minute(), i.Minute);
		EXPECT_EQ(t1.Second(), i.Second);
		EXPECT_EQ(t1.Msec(), i.Msec);
		EXPECT_EQ(t1.MsecTotal(), i.Ms);

		int h  = 0;
		int m  = 0;
		int s  = 0;
		int ms = 0;
		t4.Get(&h, &m, &s, &ms);
		EXPECT_EQ(h, i.Hour24);
		EXPECT_EQ(m, i.Minute);
		EXPECT_EQ(s, i.Second);
		EXPECT_EQ(ms, i.Msec);
	}

	EXPECT_TRUE(Time::IsValid(14, 59, 25));
	EXPECT_FALSE(Time::IsValid(25, 59, 25));

	EXPECT_TRUE(Time(14, 59, 25) == Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 24) != Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 26) > Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 26) >= Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 25) >= Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 24) < Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 24) <= Time(14, 59, 25));
	EXPECT_TRUE(Time(14, 59, 24) <= Time(14, 59, 24));

	EXPECT_EQ(Time(1, 2, 3).ToString("HH HH12 HH24 MI SS SSSSS AM A.M."), U"01 01 01 02 03 03723 AM A.M.");
	EXPECT_EQ(Time(13, 2, 3).ToString("HH HH12 HH24 MI SS SSSSS AM A.M."), U"01 01 13 02 03 46923 PM P.M.");

	// printf("%s\n", Time(1, 2, 3).ToString("HH HH12 HH24 MI SS SSSSS AM A.M.").C_Str());
	// printf("%s\n", Time(13, 2, 3).ToString("HH HH12 HH24 MI SS SSSSS AM A.M.").C_Str());

	{
		Time s(15, 17, 55);
		s += 20;
		s -= 10;
		Time ss;
		ss = s;
		EXPECT_EQ(ss, Time(15, 18, 05));
		EXPECT_EQ(ss - 10, Time(15, 17, 55));
		EXPECT_EQ(ss + 10, Time(15, 18, 15));
	}

	{
		Time s(23, 59, 55);
		s += 20;
		s -= 10;
		Time ss;
		ss = s;
		EXPECT_EQ(ss, Time(0, 0, 5));
		EXPECT_EQ(ss - 10, Time(23, 59, 55));
	}

	{
		Time s(0, 0, 5);
		s -= 20;
		s += 10;
		Time ss;
		ss = s;
		EXPECT_EQ(ss, Time(23, 59, 55));
		EXPECT_EQ(ss + 10, Time(0, 0, 5));
	}

	Time t1 = Time::FromSystem();
	Time t2 = Time::FromSystemUTC();

	// EXPECT_TRUE(t1 != t2);

	EXPECT_TRUE(t1 < Time(23, 59, 59));
	EXPECT_TRUE(t1 > Time(0, 0, 0));
	EXPECT_TRUE(t2 < Time(23, 59, 59));
	EXPECT_TRUE(t2 > Time(0, 0, 0));

	// printf("systime (loc) = %s\n", t1.ToString().C_Str());
	// printf("systime (utc) = %s\n", t2.ToString().C_Str());
}

static void test_datetime()
{
	DateTime dt1;
	DateTime dt2(Date(2016, 2, 1));
	DateTime dt3(Time(12, 4, 7));
	DateTime dt4(Date(2016, 2, 1), Time(12, 4, 7));
	DateTime dt5(Time(12, 4, 7), Date(2016, 2, 1));

	EXPECT_TRUE(dt1.IsInvalid());
	EXPECT_TRUE(dt2.IsInvalid());
	EXPECT_TRUE(dt3.IsInvalid());
	EXPECT_TRUE(!dt4.IsInvalid());
	EXPECT_TRUE(!dt5.IsInvalid());

	EXPECT_EQ(dt4, dt5);

	dt2.SetTime(dt4.GetTime());
	dt3.SetDate(dt5.GetDate());

	EXPECT_EQ(dt4, dt2);
	EXPECT_EQ(dt4, dt3);

	DateTime sdt1 = DateTime::FromSystem();
	DateTime sdt2 = DateTime::FromSystemUTC();

	EXPECT_TRUE(!sdt1.IsInvalid());
	EXPECT_TRUE(!sdt2.IsInvalid());

	// EXPECT_TRUE(sdt1 != sdt2);

	EXPECT_TRUE(sdt1.GetTime() < Time(23, 59, 59));
	EXPECT_TRUE(sdt1.GetTime() > Time(0, 0, 0));
	EXPECT_TRUE(sdt2.GetTime() < Time(23, 59, 59));
	EXPECT_TRUE(sdt2.GetTime() > Time(0, 0, 0));

	EXPECT_TRUE(sdt1.GetDate() < Date(2023, 1, 1));
	EXPECT_TRUE(sdt1.GetDate() > Date(2021, 12, 31));
	EXPECT_TRUE(sdt2.GetDate() < Date(2023, 1, 1));
	EXPECT_TRUE(sdt2.GetDate() > Date(2021, 12, 31));

	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) == DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) != DateTime(Date(2015, 12, 31), Time(14, 59, 24)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 26)) > DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 26)) > DateTime(Date(2015, 12, 30), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) >= DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 26)) >= DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) >= DateTime(Date(2015, 12, 30), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) < DateTime(Date(2015, 12, 31), Time(14, 59, 26)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 30), Time(14, 59, 25)) < DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) <= DateTime(Date(2015, 12, 31), Time(14, 59, 25)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 31), Time(14, 59, 25)) <= DateTime(Date(2015, 12, 31), Time(14, 59, 26)));
	EXPECT_TRUE(DateTime(Date(2015, 12, 30), Time(14, 59, 25)) <= DateTime(Date(2015, 12, 31), Time(14, 59, 25)));

	// printf("sysdate (loc) = %s(%d)\n", sdt1.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt1.GetTime().Msec());
	// printf("sysdate (utc) = %s(%d)\n", sdt2.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt2.GetTime().Msec());

	auto sdt1_s = DateTime::FromSQLiteJulian(sdt1.ToSQLiteJulian());
	auto sdt2_s = DateTime::FromSQLiteJulian(sdt2.ToSQLiteJulian());

	EXPECT_LE(abs(sdt1_s.GetTime().MsecTotal() - sdt1.GetTime().MsecTotal()), 2);
	EXPECT_LE(abs(sdt2_s.GetTime().MsecTotal() - sdt2.GetTime().MsecTotal()), 2);
	EXPECT_EQ(sdt1_s.GetDate(), sdt1.GetDate());
	EXPECT_EQ(sdt2_s.GetDate(), sdt2.GetDate());

	// printf("sqldate (loc) = %s(%d)\n", sdt1_s.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt1_s.GetTime().Msec());
	// printf("sqldate (utc) = %s(%d)\n", sdt2_s.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt2_s.GetTime().Msec());

	sdt1_s = DateTime::FromUnix(sdt1.ToUnix());
	sdt2_s = DateTime::FromUnix(sdt2.ToUnix());

	EXPECT_LE(abs(sdt1_s.GetTime().MsecTotal() - sdt1.GetTime().MsecTotal()), 2);
	EXPECT_LE(abs(sdt2_s.GetTime().MsecTotal() - sdt2.GetTime().MsecTotal()), 2);
	EXPECT_EQ(sdt1_s.GetDate(), sdt1.GetDate());
	EXPECT_EQ(sdt2_s.GetDate(), sdt2.GetDate());

	// printf("Unix (loc) = %s(%d)\n", sdt1_s.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt1_s.GetTime().Msec());
	// printf("Unix (utc) = %s(%d)\n", sdt2_s.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), sdt2_s.GetTime().Msec());

	DateTime current(Date(2021, 6, 14), Time(4, 35, 0, 0));
	auto     unix = current.ToUnix();
	EXPECT_LE(fabs(unix - 1623645300.0), 1.0);
	// printf("Unix = %s(%d)\n", current.ToString("YYYY.MM.DD HH24:MI:SS").C_Str(), current.GetTime().Msec());
	// printf("Unix = %f\n", unix);
}

void test_dist()
{
	DateTime t1 = DateTime::FromSystem();

	DateTime t2 = t1;

	EXPECT_EQ(t1.DistanceMs(t2), 0U);

	t2.GetTime() += 5;

	EXPECT_EQ(t1.DistanceMs(t2), 5000U);
	EXPECT_EQ(t2.DistanceMs(t1), 5000U);

	t2.GetDate() += 1;

	EXPECT_EQ(t1.DistanceMs(t2), Core::TIME_MS_IN_DAY + 5000U);
	EXPECT_EQ(t2.DistanceMs(t1), Core::TIME_MS_IN_DAY + 5000U);

	t2.GetDate() += 1;

	EXPECT_EQ(t1.DistanceMs(t2), Core::TIME_MS_IN_DAY * 2 + 5000U);
	EXPECT_EQ(t2.DistanceMs(t1), Core::TIME_MS_IN_DAY * 2 + 5000U);
}

TEST(Core, DateTime)
{
	UT_MEM_CHECK_INIT();

	test_date();
	test_time();
	test_datetime();
	test_dist();

	UT_MEM_CHECK();
}

UT_END();
