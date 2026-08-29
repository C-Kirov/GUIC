#include "lunar.h"
#include <cstdio>
#include <cstring>

// 农历数据 (1900-2100)
// 改为 unsigned int —— 表中多处数值超过 65535（如 0x16554, 0x1d255, 0x1d0b6 等）
static const unsigned int g_lunarInfo[] = {
    // 1900
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0,
    0x09ad0, 0x055d2, 0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540,
    0x0d6a0, 0x0ada2, 0x095b0, 0x14977, 0x04970, 0x0a4b0, 0x0b4b5, 0x06a50,
    0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970, 0x06566, 0x0d4a0,
    0x0ea50, 0x16a95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2,
    0x0a950, 0x0b557, 0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573,
    0x052b0, 0x0a9a8, 0x0e950, 0x06aa0, 0x0aea6, 0x0ab50, 0x04b60, 0x0aae4,
    0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0, 0x096d0, 0x04dd5,
    0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b6a0, 0x195a6,
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46,
    0x0ab60, 0x09570, 0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58,
    0x05ac0, 0x0ab60, 0x096d5, 0x092e0, 0x0c960, 0x0d954, 0x0d4a0, 0x0da50,
    0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5, 0x0a950, 0x0b4a0,
    0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260,
    0x0ea65, 0x0d530, 0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0,
    0x1d0b6, 0x0d250, 0x0d520, 0x0dd45, 0x0b5a0, 0x056d0, 0x055b2, 0x049b0,
    0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0, 0x14b63, 0x09370,
    0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06aa0, 0x1a6c4, 0x0aae0,
    0x092e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0,
    0x0a6d0, 0x055d4, 0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50,
    0x055a0, 0x0aba4, 0x0a5b0, 0x052b0, 0x0b273, 0x06930, 0x07337, 0x06aa0,
    0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160, 0x0e968, 0x0d520,
    0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a4d0, 0x0d150, 0x0f252,
    0x0d520
};

// 天干
static const char* g_tianGan[] = {"甲","乙","丙","丁","戊","己","庚","辛","壬","癸"};
// 地支
static const char* g_diZhi[]   = {"子","丑","寅","卯","辰","巳","午","未","申","酉","戌","亥"};
// 生肖
static const char* g_shengXiao[] = {"鼠","牛","虎","兔","龙","蛇","马","羊","猴","鸡","狗","猪"};
// 农历月名
static const char* g_monthName[] = {"正","二","三","四","五","六","七","八","九","十","冬","腊"};
// 农历日名
static const char* g_dayName[] = {
    "","初一","初二","初三","初四","初五","初六","初七","初八","初九","初十",
    "十一","十二","十三","十四","十五","十六","十七","十八","十九","二十",
    "廿一","廿二","廿三","廿四","廿五","廿六","廿七","廿八","廿九","三十"
};

// 每一年编码:
//   bit 0-11:  1-12 月大小 (1=30天, 0=29天), bit0=正月
//   bit 12-15: 闰月月份 (0=无闰月, 1-12=闰该月)
//   bit 16:    如果是闰月年，此位为闰月大小 (1=30天, 0=29天)

static int GetLunarMonthDays(int lunarYear, int lunarMonth)
{
    int idx = lunarYear - 1900;
    if (idx < 0 || idx >= (int)(sizeof(g_lunarInfo)/sizeof(g_lunarInfo[0])))
        return 30;

    unsigned int info = g_lunarInfo[idx];
    int leapMonth = (info >> 12) & 0xF;
    bool isLeap = false;

    if (lunarMonth < 0) { isLeap = true; lunarMonth = -lunarMonth; }
    if (lunarMonth > 12) return 30;

    if (isLeap && lunarMonth == leapMonth) {
        return (info & 0x10000) ? 30 : 29;
    }

    int actualMonth = (lunarMonth > leapMonth && leapMonth > 0) ? lunarMonth - 1 : lunarMonth;
    return (info & (1u << (actualMonth - 1))) ? 30 : 29;
}

static int GetLunarYearDays(int lunarYear)
{
    int idx = lunarYear - 1900;
    if (idx < 0 || idx >= (int)(sizeof(g_lunarInfo)/sizeof(g_lunarInfo[0])))
        return 354;

    unsigned int info = g_lunarInfo[idx];
    int days = 0;
    int leapMonth = (info >> 12) & 0xF;

    for (int m = 1; m <= 12; m++) {
        days += (info & (1u << (m - 1))) ? 30 : 29;
    }
    if (leapMonth > 0) {
        days += (info & 0x10000) ? 30 : 29;
    }
    return days;
}

// 获取某年春节对应的公历日期
static SolarDate GetSpringFestival(int year)
{
    int idx = year - 1900;
    SolarDate sf = { year, 1, 1 };
    if (idx < 0 || idx >= (int)(sizeof(g_lunarInfo)/sizeof(g_lunarInfo[0])))
        return sf;

    static const unsigned char springFestivalDay[] = {
        31,19, 8,29,16, 4,25,13, 2,22,10,30,18, 6,26,14, 3,23,11,28,  // 1900-1919
        17, 8,28,16, 5,25,13, 2,23,10,30,17, 7,26,15, 4,24,11,31,19,  // 1920-1939
         8,27,15, 5,25,13, 2,22,10,29,17, 6,27,15, 3,24,12,31,18, 8,  // 1940-1959
        28,15, 5,25,13, 2,21, 9,30,17, 6,27,15, 3,23,11,31,18, 7,28,  // 1960-1979
        16, 5,25,13, 2,20, 9,29,17, 6,27,15, 4,23,10,31,19, 7,28,16,  // 1980-1999
         5,24,12, 1,22,10,29,18, 7,26,14, 3,23,10,31,19, 8,28,16, 5,  // 2000-2019
        25,12, 1,22,10,29,17, 6,26,13, 3,23,11,31,19, 8,28,15, 5,24,  // 2020-2039
        12, 1,21,10,29,18, 7,26,14, 3,22,10,30,18, 6,26,15, 4,23,11,  // 2040-2059
        29,19, 7,27,16, 5,24,12, 1,22, 9,29,18, 6,26,13, 3,22,11,30,  // 2060-2079
        18, 7,26,14, 3,24,12, 1,20, 9,29,17, 7,26,14, 3,23,10,31,19,  // 2080-2099
         8                                                                   // 2100
    };

    sf.month = 1;
    sf.day = springFestivalDay[idx];
    if (sf.day > 31) { sf.month = 2; sf.day -= 31; }
    return sf;
}

// 公历日期转年内第几天 (1-based)
static int DayOfYear(const SolarDate& s)
{
    static const int monthDays[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int d = s.day;
    for (int m = 1; m < s.month; m++) {
        d += monthDays[m];
        if (m == 2 && ((s.year % 4 == 0 && s.year % 100 != 0) || s.year % 400 == 0))
            d += 1;
    }
    return d;
}

LunarDate SolarToLunar(const SolarDate& solar)
{
    LunarDate result = {0, 0, 0, false};

    SolarDate sf = GetSpringFestival(solar.year);
    int sfDayOfYear = DayOfYear(sf);
    int solarDayOfYear = DayOfYear(solar);

    int daysDiff;
    int lunarYear;

    if (solarDayOfYear >= sfDayOfYear) {
        daysDiff = solarDayOfYear - sfDayOfYear;
        lunarYear = solar.year;
    } else {
        SolarDate sfPrev = GetSpringFestival(solar.year - 1);
        int sfPrevDOY = DayOfYear(sfPrev);
        int daysInPrevYear = ((solar.year - 1) % 4 == 0 && ((solar.year - 1) % 100 != 0))
                             || ((solar.year - 1) % 400 == 0) ? 366 : 365;
        daysDiff = solarDayOfYear + (daysInPrevYear - sfPrevDOY);
        lunarYear = solar.year - 1;
    }

    int idx = lunarYear - 1900;
    unsigned int info = g_lunarInfo[idx];
    int leapMonth = (info >> 12) & 0xF;
    int remaining = daysDiff;

    for (int m = 1; m <= 12; m++) {
        int md = (info & (1u << (m - 1))) ? 30 : 29;
        if (remaining < md) {
            result.year = lunarYear;
            result.month = m;
            result.day = remaining + 1;
            result.isLeapMonth = false;
            return result;
        }
        remaining -= md;

        if (m == leapMonth) {
            int leapDays = (info & 0x10000) ? 30 : 29;
            if (remaining < leapDays) {
                result.year = lunarYear;
                result.month = m;
                result.day = remaining + 1;
                result.isLeapMonth = true;
                return result;
            }
            remaining -= leapDays;
        }
    }

    result.year = lunarYear;
    result.month = 12;
    result.day = remaining + 1;
    return result;
}

std::string LunarToString(const LunarDate& lunar)
{
    char buf[64];
    const char* monthPrefix = lunar.isLeapMonth ? "闰" : "";
    sprintf(buf, "%s%s月%s",
            monthPrefix,
            g_monthName[(lunar.month - 1) % 12],
            g_dayName[lunar.day]);
    return std::string(buf);
}

std::string GetTianGanDiZhi(int lunarYear)
{
    int tg = (lunarYear - 4) % 10;
    if (tg < 0) tg += 10;
    int dz = (lunarYear - 4) % 12;
    if (dz < 0) dz += 12;
    char buf[8];
    sprintf(buf, "%s%s", g_tianGan[tg], g_diZhi[dz]);
    return std::string(buf);
}

std::string GetShengXiao(int lunarYear)
{
    int idx = (lunarYear - 4) % 12;
    if (idx < 0) idx += 12;
    return g_shengXiao[idx];
}
