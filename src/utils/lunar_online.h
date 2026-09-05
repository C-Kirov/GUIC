#ifndef LUNAR_ONLINE_H
#define LUNAR_ONLINE_H

#include <windows.h>
#include <string>

// 在线农历数据（来源：香港天文台 HKO 官方《公历与农历日期对照表》按年文本文件）
struct LunarOnlineData {
    bool        valid;         // 是否成功获取并解析
    int         dataYear;      // 在线数据所属公历年（-1 = 无）
    SYSTEMTIME  fetchTime;     // 获取时刻（用于判定是否当天数据）
    int         lunarYear;     // 农历年（干支/生肖据此推算）
    int         lunarMonth;    // 农历月 1-12
    int         lunarDay;      // 农历日 1-30
    bool        isLeapMonth;   // 是否闰月
};

// 在线获取指定公历日期对应的农历（HKO 年度对照表，HTTPS）。
// 需在独立线程中调用；返回 false 表示不可用，调用方应回退本地计算。
bool FetchOnlineLunar(const SYSTEMTIME& st, LunarOnlineData& out);

// 解析 HKO 英文年度文本文件（如 T2026e.txt）内容，查询其中某公历日期的农历。
// textYear 必须与文件头行标注年份一致；out 仅在匹配到行时置位。
bool ParseHkoCalendarText(int textYear, int queryYear, int queryMonth, int queryDay,
                          const std::string& text, LunarOnlineData& out);

#endif // LUNAR_ONLINE_H
