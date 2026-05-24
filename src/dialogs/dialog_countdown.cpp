#include "dialog_countdown.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"
#include <cstdio>

// 一次性倒计时对话框过程
static INT_PTR CALLBACK CountdownOnceDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK) {
                char buffer[1024];
                GetDlgItemTextA(hwnd, IDC_EDIT_ONCE_TIME, buffer, sizeof(buffer));
                std::string timeStr(buffer);
                if (timeStr.length() >= 19) {
                    int year, month, day, hour, minute, second;
                    if (sscanf(timeStr.c_str(), "%d-%d-%d %d:%d:%d",
                               &year, &month, &day, &hour, &minute, &second) == 6 &&
                        year >= 1900 && year <= 2100 &&
                        month >= 1 && month <= 12 &&
                        day >= 1 && day <= 31 &&
                        hour >= 0 && hour <= 23 &&
                        minute >= 0 && minute <= 59 &&
                        second >= 0 && second <= 59)
                    {
                        AppConfig& cfg = AppConfig::GetInstance();
                        cfg.countdownMode = COUNTDOWN_ONCE;
                        memset(&cfg.countdownTarget.onceTime, 0, sizeof(SYSTEMTIME));
                        cfg.countdownTarget.onceTime.wYear   = (WORD)year;
                        cfg.countdownTarget.onceTime.wMonth  = (WORD)month;
                        cfg.countdownTarget.onceTime.wDay    = (WORD)day;
                        cfg.countdownTarget.onceTime.wHour   = (WORD)hour;
                        cfg.countdownTarget.onceTime.wMinute = (WORD)minute;
                        cfg.countdownTarget.onceTime.wSecond = (WORD)second;
                        WriteLog("设置一次性倒计时: " + timeStr);
                        MessageBox(hwnd, "一次性倒计时设置成功！", "提示", MB_OK | MB_ICONINFORMATION);
                        EndDialog(hwnd, IDOK);
                    } else {
                        MessageBox(hwnd, "请输入有效的时间格式！\n格式：YYYY-MM-DD HH:MM:SS", "错误", MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBox(hwnd, "请输入完整的时间格式！\n格式：YYYY-MM-DD HH:MM:SS", "错误", MB_OK | MB_ICONERROR);
                }
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
            }
            break;
        }
    }
    return FALSE;
}

// 每日倒计时对话框过程
static INT_PTR CALLBACK CountdownDailyDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            SetDlgItemTextA(hwnd, IDC_EDIT_DAILY_REMARK, AppConfig::GetInstance().dailyRemark.c_str());
            return TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK) {
                char timeBuf[256];
                GetDlgItemTextA(hwnd, IDC_EDIT_DAILY_TIME, timeBuf, sizeof(timeBuf));
                std::string timeStr(timeBuf);
                if (timeStr.length() >= 8) {
                    int hour, minute, second;
                    if (sscanf(timeStr.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3 &&
                        hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59)
                    {
                        AppConfig& cfg = AppConfig::GetInstance();
                        cfg.countdownMode = COUNTDOWN_DAILY;
                        memset(&cfg.countdownTarget.dailyTime, 0, sizeof(SYSTEMTIME));
                        cfg.countdownTarget.dailyTime.wHour   = (WORD)hour;
                        cfg.countdownTarget.dailyTime.wMinute = (WORD)minute;
                        cfg.countdownTarget.dailyTime.wSecond = (WORD)second;

                        char remarkBuf[256];
                        GetDlgItemTextA(hwnd, IDC_EDIT_DAILY_REMARK, remarkBuf, sizeof(remarkBuf));
                        cfg.dailyRemark = remarkBuf;

                        WriteLog("设置每日倒计时: " + timeStr + " 备注: " + cfg.dailyRemark);
                        MessageBox(hwnd, "每日倒计时设置成功！", "提示", MB_OK | MB_ICONINFORMATION);
                        EndDialog(hwnd, IDOK);
                    } else {
                        MessageBox(hwnd, "请输入有效的时间格式！\n格式：HH:MM:SS", "错误", MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBox(hwnd, "请输入完整的时间格式！\n格式：HH:MM:SS", "错误", MB_OK | MB_ICONERROR);
                }
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
            }
            break;
        }
    }
    return FALSE;
}

void ShowCountdownOnceDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COUNTDOWN_ONCE_DIALOG), parent, CountdownOnceDlgProc, 0);
}

void ShowCountdownDailyDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COUNTDOWN_DAILY_DIALOG), parent, CountdownDailyDlgProc, 0);
}
