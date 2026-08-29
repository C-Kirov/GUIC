#ifndef LUNAR_H
#define LUNAR_H

#include <string>

struct LunarDate {
    int year;           // 农历年
    int month;          // 农历月 (1-12, 负数为闰月)
    int day;            // 农历日
    bool isLeapMonth;   // 是否闰月
};

struct SolarDate {
    int year;
    int month;
    int day;
};

// 公历转农历
LunarDate SolarToLunar(const SolarDate& solar);

// 农历转字符串（如 "甲辰年腊月初三"）
std::string LunarToString(const LunarDate& lunar);

// 获取天干地支纪年
std::string GetTianGanDiZhi(int lunarYear);

// 获取生肖
std::string GetShengXiao(int lunarYear);

#endif // LUNAR_H
