#include "renderer.h"
#include "../config/config.h"
#include "utils.h"
#include "gdi_utils.h"
#include "lunar.h"
#include "lunar_online.h"
#include "weather.h"
#include "../main.h"
#include "../i18n.h"
#include <cstdio>

// 外部引用（来自 windowproc.cpp 的静态变量）
extern WeatherInfo g_weather;
extern bool        g_weatherLoading;
extern LunarOnlineData g_lunarOnline;

void Renderer::Draw(HWND hwnd, HDC hdc)
{
    AppConfig& cfg = AppConfig::GetInstance();

    ScopedFont hFont(CreateFont(
        cfg.fontSize, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, cfg.fontName.c_str()
    ));

    GDIObjectSelector fontSel(hdc, hFont);
    SetTextColor(hdc, cfg.fontColor);
    SetBkMode(hdc, TRANSPARENT);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientWidth  = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    ScopedBrush whiteBrush(CreateSolidBrush(RGB(255, 255, 255)));
    FillRect(hdc, &clientRect, whiteBrush);

    int y = 10;
    const int leftMargin = 10;
    int maxWidth = clientWidth - leftMargin - 10;

    // 1. 显示时钟
    if (cfg.showClock)
    {
        SYSTEMTIME st;
        cfg.GetCurrentDateTime(st);
        char timeStr[256];
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        std::string specialDay = cfg.GetSpecialDay(st.wYear, st.wMonth, st.wDay,
                                                   st.wHour, st.wMinute, st.wSecond);
        std::string displayStr(timeStr);
        if (!specialDay.empty()) {
            displayStr += " [" + specialDay + "]";
        }

        // 农历显示（在线同步优先，失败回退本地计算）
        if (cfg.showLunar) {
            std::string lunarStr, tdgz, sx;
            if (g_lunarOnline.valid &&
                g_lunarOnline.fetchTime.wYear == st.wYear &&
                g_lunarOnline.fetchTime.wMonth == st.wMonth &&
                g_lunarOnline.fetchTime.wDay == st.wDay) {
                LunarDate ld = { g_lunarOnline.lunarYear, g_lunarOnline.lunarMonth,
                                 g_lunarOnline.lunarDay, g_lunarOnline.isLeapMonth };
                lunarStr = LunarToString(ld);
                tdgz = GetTianGanDiZhi(ld.year);
                sx = GetShengXiao(ld.year);
            } else {
                SolarDate sd = { (int)st.wYear, (int)st.wMonth, (int)st.wDay };
                LunarDate ld = SolarToLunar(sd);
                lunarStr = LunarToString(ld);
                tdgz = GetTianGanDiZhi(ld.year);
                sx = GetShengXiao(ld.year);
            }
            char lunarBuf[128];
            sprintf(lunarBuf, "  [%s%s年 %s]", tdgz.c_str(), sx.c_str(), lunarStr.c_str());
            displayStr += lunarBuf;
        }

        TextOutA(hdc, leftMargin, y, displayStr.c_str(), (int)displayStr.length());
        y += cfg.autoLayout ? cfg.fontSize + 10 : 30;
    }

    // 2. 显示倒计时
    if (cfg.showCountdown)
    {
        SYSTEMTIME empty = {};
        if (memcmp(&cfg.countdownTarget.onceTime, &empty, sizeof(SYSTEMTIME)) != 0 ||
            cfg.countdownTarget.dailyTime.wHour != 0 ||
            cfg.countdownTarget.dailyTime.wMinute != 0)
        {
            SYSTEMTIME now;
            cfg.GetCurrentDateTime(now);
            if (cfg.countdownMode == COUNTDOWN_DAILY)
            {
                SYSTEMTIME todayTarget = now;
                todayTarget.wHour   = cfg.countdownTarget.dailyTime.wHour;
                todayTarget.wMinute = cfg.countdownTarget.dailyTime.wMinute;
                todayTarget.wSecond = cfg.countdownTarget.dailyTime.wSecond;
                todayTarget.wMilliseconds = 0;

                FILETIME ftNow, ftTarget;
                SystemTimeToFileTime(&now, &ftNow);
                SystemTimeToFileTime(&todayTarget, &ftTarget);
                ULARGE_INTEGER uNow, uTarget;
                uNow.LowPart   = ftNow.dwLowDateTime;
                uNow.HighPart  = ftNow.dwHighDateTime;
                uTarget.LowPart  = ftTarget.dwLowDateTime;
                uTarget.HighPart = ftTarget.dwHighDateTime;
                __int64 diff = (__int64)(uTarget.QuadPart - uNow.QuadPart) / 10000000;

                if (diff > 0) {
                    char buf[256];
                    sprintf(buf, _S(STR_COUNTDOWN_FMT),
                            diff / 86400, (diff % 86400) / 3600,
                            (diff % 3600) / 60, diff % 60);
                    TextOutA(hdc, leftMargin, y, buf, (int)strlen(buf));
                } else {
                    TextOutA(hdc, leftMargin, y, _S(STR_COUNTDOWN_ENDED),
                             (int)strlen(_S(STR_COUNTDOWN_ENDED)));
                }
                y += cfg.fontSize + 10;
                if (!cfg.dailyRemark.empty()) {
                    std::string remark = cfg.dailyRemark;
                    TextOutA(hdc, leftMargin, y, remark.c_str(), (int)remark.length());
                    y += cfg.fontSize + 10;
                }
            }
            else if (cfg.countdownMode == COUNTDOWN_ONCE)
            {
                FILETIME ftNow, ftTarget;
                SystemTimeToFileTime(&now, &ftNow);
                SystemTimeToFileTime(&cfg.countdownTarget.onceTime, &ftTarget);
                ULARGE_INTEGER uNow, uTarget;
                uNow.LowPart   = ftNow.dwLowDateTime;
                uNow.HighPart  = ftNow.dwHighDateTime;
                uTarget.LowPart  = ftTarget.dwLowDateTime;
                uTarget.HighPart = ftTarget.dwHighDateTime;
                __int64 diff = (__int64)(uTarget.QuadPart - uNow.QuadPart) / 10000000;

                if (diff > 0) {
                    char buf[256];
                    sprintf(buf, _S(STR_COUNTDOWN_FMT),
                            diff / 86400, (diff % 86400) / 3600,
                            (diff % 3600) / 60, diff % 60);
                    TextOutA(hdc, leftMargin, y, buf, (int)strlen(buf));
                } else {
                    TextOutA(hdc, leftMargin, y, _S(STR_COUNTDOWN_ENDED),
                             (int)strlen(_S(STR_COUNTDOWN_ENDED)));
                }
                y += cfg.fontSize + 10;
            }
        }
        else
        {
            TextOutA(hdc, leftMargin, y, _S(STR_COUNTDOWN_NOT_SET),
                     (int)strlen(_S(STR_COUNTDOWN_NOT_SET)));
            y += cfg.fontSize + 10;
        }
    }

    // 3. 显示消息
    if (cfg.showMessage && !cfg.message.empty())
    {
        RECT msgRect = {leftMargin, y, leftMargin + maxWidth, clientHeight - 10};
        int msgHeight = DrawTextA(hdc, cfg.message.c_str(), -1, &msgRect,
                                  DT_WORDBREAK | DT_CALCRECT);
        msgRect.bottom = msgRect.top + msgHeight;
        DrawTextA(hdc, cfg.message.c_str(), -1, &msgRect, DT_WORDBREAK);
        y += msgHeight + 10;
    }

    // 4. 显示天气
    if (cfg.showWeather)
    {
        if (g_weatherLoading) {
            TextOutA(hdc, leftMargin, y, _S(STR_WEATHER_LOADING),
                     (int)strlen(_S(STR_WEATHER_LOADING)));
            y += cfg.fontSize + 10;
        } else if (g_weather.valid) {
            std::string wx = g_weather.condition + " " + g_weather.temp
                           + " " + _S(STR_HUMIDITY) + g_weather.humidity
                           + " " + _S(STR_WIND) + g_weather.wind;
            TextOutA(hdc, leftMargin, y, wx.c_str(), (int)wx.length());
            y += cfg.fontSize + 10;
        } else if (!cfg.weatherCity.empty()) {
            TextOutA(hdc, leftMargin, y, _S(STR_WEATHER_FAILED),
                     (int)strlen(_S(STR_WEATHER_FAILED)));
            y += cfg.fontSize + 10;
        }
    }

    // 5. work.txt 内容
    std::string workContent = ReadWorkFile();
    if (!workContent.empty())
    {
        RECT workRect = {leftMargin, y, leftMargin + maxWidth, clientHeight - 10};
        int workHeight = DrawTextA(hdc, workContent.c_str(), -1, &workRect,
                                   DT_WORDBREAK | DT_CALCRECT);
        workRect.bottom = workRect.top + workHeight;
        DrawTextA(hdc, workContent.c_str(), -1, &workRect, DT_WORDBREAK);
    }
}
