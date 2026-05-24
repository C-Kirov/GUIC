#include "renderer.h"
#include "../config/config.h"       // AppConfig 单例
#include "utils.h"                 // ReadWorkFile
#include "gdi_utils.h"             // RAII GDI 包装
#include <cstdio>

void Renderer::Draw(HWND hwnd, HDC hdc)
{
    AppConfig& cfg = AppConfig::GetInstance();

    // 创建字体（使用 RAII 自动释放）
    ScopedFont hFont(CreateFont(
        cfg.fontSize, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, cfg.fontName.c_str()
    ));

    // 选择字体并保存旧字体（自动恢复）
    GDIObjectSelector fontSel(hdc, hFont);
    SetTextColor(hdc, cfg.fontColor);
    SetBkMode(hdc, TRANSPARENT);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientWidth  = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // 白色背景
    ScopedBrush whiteBrush(CreateSolidBrush(RGB(255, 255, 255)));
    FillRect(hdc, &clientRect, whiteBrush);

    int y = 10;
    const int leftMargin = 10;
    const int rightMargin = 10;
    int maxWidth = clientWidth - leftMargin - rightMargin;

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
        TextOutA(hdc, leftMargin, y, displayStr.c_str(), displayStr.length());
        y += cfg.autoLayout ? cfg.fontSize + 10 : 30;
    }

    // 2. 显示倒计时
    if (cfg.showCountdown)
    {
        SYSTEMTIME empty = {};
        if (memcmp(&cfg.countdownTarget.onceTime, &empty, sizeof(SYSTEMTIME)) != 0)
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
                    sprintf(buf, "倒计时：%lld天%lld小时%lld分钟%lld秒",
                            diff / 86400, (diff % 86400) / 3600, (diff % 3600) / 60, diff % 60);
                    TextOutA(hdc, leftMargin, y, buf, strlen(buf));
                } else {
                    TextOutA(hdc, leftMargin, y, "倒计时已结束!", 13);
                }
                y += cfg.fontSize + 10;
                if (!cfg.dailyRemark.empty()) {
                    std::string remark = "备注: " + cfg.dailyRemark;
                    TextOutA(hdc, leftMargin, y, remark.c_str(), remark.length());
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
                    sprintf(buf, "倒计时：%lld天%lld小时%lld分钟%lld秒",
                            diff / 86400, (diff % 86400) / 3600, (diff % 3600) / 60, diff % 60);
                    TextOutA(hdc, leftMargin, y, buf, strlen(buf));
                } else {
                    TextOutA(hdc, leftMargin, y, "倒计时已结束!", 13);
                }
                y += cfg.fontSize + 10;
            }
        }
        else
        {
            TextOutA(hdc, leftMargin, y, "倒计时功能未启用", 17);
            y += cfg.fontSize + 10;
        }
    }

    // 3. 提示信息（自动换行）
    if (cfg.showMessage && !cfg.message.empty())
    {
        RECT msgRect = {leftMargin, y, leftMargin + maxWidth, clientHeight - 10};
        int msgHeight = DrawTextA(hdc, cfg.message.c_str(), -1, &msgRect, DT_WORDBREAK | DT_CALCRECT);
        msgRect.bottom = msgRect.top + msgHeight;
        DrawTextA(hdc, cfg.message.c_str(), -1, &msgRect, DT_WORDBREAK);
        y += msgHeight + 10;
    }

    // 4. work.txt 内容
    std::string workContent = ReadWorkFile();
    if (!workContent.empty())
    {
        RECT workRect = {leftMargin, y, leftMargin + maxWidth, clientHeight - 10};
        int workHeight = DrawTextA(hdc, workContent.c_str(), -1, &workRect, DT_WORDBREAK | DT_CALCRECT);
        workRect.bottom = workRect.top + workHeight;
        DrawTextA(hdc, workContent.c_str(), -1, &workRect, DT_WORDBREAK);
    }
}
