#include "dialog_special.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../utils/gdi_utils.h"
#include "../main.h"
#include "../i18n.h"
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// ========== 世界时钟：注册表 TZI 读取时区偏移 ==========
struct TimeZoneEntry {
    StringID nameId;                // 国际化显示名
    const char* winKey;
    int   utcOffsetMinutes;   // 备用：无 DST 时的 UTC 偏移（分钟）
};

static const std::vector<TimeZoneEntry> g_timeZones = {
    { STR_TZ_BEIJING,      "China Standard Time",       480 },
    { STR_TZ_TOKYO,        "Tokyo Standard Time",        540 },
    { STR_TZ_NEW_YORK,     "Eastern Standard Time",     -300 },
    { STR_TZ_LONDON,       "GMT Standard Time",            0 },
    { STR_TZ_SYDNEY,       "AUS Eastern Standard Time",  660 },
    { STR_TZ_MOSCOW,       "Russian Standard Time",       180 },
    { STR_TZ_PARIS,        "Romance Standard Time",        60 },
    { STR_TZ_LOS_ANGELES,  "Pacific Standard Time",      -480 }
};

// 注册表中的 TZI 二进制格式
struct REG_TZI_FORMAT {
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
};

// 通过注册表获取时区信息
static bool GetTimeZoneInfoByName(const char* winKey, TIME_ZONE_INFORMATION& tzi)
{
    char subKey[256];
    sprintf(subKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\%s", winKey);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        WriteLog(std::string("无法打开注册表键: ") + subKey);
        return false;
    }

    REG_TZI_FORMAT regTzi;
    DWORD size = sizeof(regTzi);
    DWORD type = 0;
    LONG ret = RegQueryValueExA(hKey, "TZI", NULL, &type, (LPBYTE)&regTzi, &size);
    RegCloseKey(hKey);

    if (ret != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(regTzi)) {
        WriteLog(std::string("读取 TZI 失败，键: ") + subKey + ", 错误码: " + std::to_string(ret));
        return false;
    }

    ZeroMemory(&tzi, sizeof(tzi));
    tzi.Bias           = regTzi.Bias;
    tzi.StandardBias   = regTzi.StandardBias;
    tzi.DaylightBias   = regTzi.DaylightBias;
    tzi.StandardDate   = regTzi.StandardDate;
    tzi.DaylightDate   = regTzi.DaylightDate;
    return true;
}

// 使用时区信息转换 UTC → 本地
static bool ConvertUtcToTimeZone(const SYSTEMTIME& utc,
                                 const TIME_ZONE_INFORMATION& tzi,
                                 SYSTEMTIME& local)
{
    return SystemTimeToTzSpecificLocalTime(&tzi, &utc, &local) != 0;
}

// 简单偏移计算（备用方案）
static void ApplySimpleOffset(const SYSTEMTIME& utc, int offsetMinutes, SYSTEMTIME& local)
{
    FILETIME ft;
    SystemTimeToFileTime(&utc, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart  = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;

    LONGLONG offset100ns = (LONGLONG)offsetMinutes * 60LL * 10000000LL;
    ul.QuadPart += offset100ns;

    ft.dwLowDateTime  = ul.LowPart;
    ft.dwHighDateTime = ul.HighPart;
    FileTimeToSystemTime(&ft, &local);
}

// ========== 世界时钟对话框 —— 每实例数据 ==========
struct WorldClockData {
    HWND list;
    HWND display;
    int  sel;
};

static INT_PTR CALLBACK WorldClockDlgProc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{
    auto* data = (WorldClockData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_INITDIALOG: {
        data = new WorldClockData();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

        SetWindowTextA(hwnd, _S(STR_WORLD_CLOCK));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_CLOSE));

        data->list    = GetDlgItem(hwnd, IDC_LIST_WORLD_CLOCKS);
        data->display = GetDlgItem(hwnd, IDC_STATIC_WORLD_CLOCK_DISPLAY);

        for (size_t i = 0; i < g_timeZones.size(); ++i)
            SendMessageA(data->list, LB_ADDSTRING, 0,
                         (LPARAM)_S(g_timeZones[i].nameId));
        SendMessage(data->list, LB_SETCURSEL, 0, 0);
        data->sel = 0;

        SetTimer(hwnd, 1, 1000, NULL);
        return TRUE;
    }

    case WM_TIMER:
        if (data && data->list && data->display &&
            data->sel >= 0 && data->sel < (int)g_timeZones.size())
        {
            SYSTEMTIME utc, local;
            GetSystemTime(&utc);
            bool converted = false;
            const TimeZoneEntry& entry = g_timeZones[data->sel];

            TIME_ZONE_INFORMATION tzi;
            if (GetTimeZoneInfoByName(entry.winKey, tzi)) {
                if (ConvertUtcToTimeZone(utc, tzi, local))
                    converted = true;
            }

            if (!converted) {
                ApplySimpleOffset(utc, entry.utcOffsetMinutes, local);
                converted = true;
            }

            char buf[256];
            sprintf(buf, "%s\n%04d-%02d-%02d %02d:%02d:%02d",
                    _S(entry.nameId),
                    local.wYear, local.wMonth, local.wDay,
                    local.wHour, local.wMinute, local.wSecond);
            SetWindowTextA(data->display, buf);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            KillTimer(hwnd, 1);
            EndDialog(hwnd, IDOK);
        } else if (data && LOWORD(wParam) == IDC_LIST_WORLD_CLOCKS &&
                   HIWORD(wParam) == LBN_SELCHANGE) {
            int s = (int)SendMessage(data->list, LB_GETCURSEL, 0, 0);
            if (s != LB_ERR) {
                data->sel = s;
                SendMessage(hwnd, WM_TIMER, 1, 0);
            }
        }
        break;

    case WM_CLOSE:
        KillTimer(hwnd, 1);
        EndDialog(hwnd, IDCANCEL);
        break;

    case WM_DESTROY:
        delete data;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
        break;
    }
    return FALSE;
}

// ========== 模拟时钟对话框 —— 每实例数据 ==========
struct AnalogClockData {
    int  hour, minute, second;
    HWND clockWnd;
};

static INT_PTR CALLBACK AnalogClockDlgProc(HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam)
{
    auto* data = (AnalogClockData*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_INITDIALOG: {
        data = new AnalogClockData();
        data->clockWnd = GetDlgItem(hwnd, IDC_STATIC_ANALOG_CLOCK);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);

        SetWindowTextA(hwnd, _S(STR_ANALOG_CLOCK));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_CLOSE));

        SetTimer(hwnd, 1, 1000, NULL);
        return TRUE;
    }

    case WM_TIMER: {
        if (!data) break;

        SYSTEMTIME st;
        AppConfig::GetInstance().GetCurrentDateTime(st);
        data->hour   = st.wHour % 12;
        data->minute = st.wMinute;
        data->second = st.wSecond;

        if (data->clockWnd)
            InvalidateRect(data->clockWnd, NULL, TRUE);
        break;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (!data) break;

        if (dis->hwndItem == data->clockWnd &&
            dis->itemAction == ODA_DRAWENTIRE)
        {
            HDC   hdc = dis->hDC;
            RECT  r   = dis->rcItem;
            int   w   = r.right  - r.left;
            int   h   = r.bottom - r.top;
            int   cx  = w / 2;
            int   cy  = h / 2;
            int   rad = std::min(w, h) / 2 - 10;

            // 背景填充（RAII）
            {
                ScopedBrush wbr(CreateSolidBrush(RGB(255, 255, 255)));
                FillRect(hdc, &r, wbr);
            }

            // 外圆边框（RAII）
            {
                ScopedPen p(CreatePen(PS_SOLID, 2, RGB(0, 0, 0)));
                GDIObjectSelector sel(hdc, p);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Ellipse(hdc, cx - rad, cy - rad, cx + rad, cy + rad);
            }

            // 12 个刻度数字
            for (int i = 1; i <= 12; i++) {
                double a  = (i * 30 - 90) * 3.14159 / 180.0;
                int    x1 = cx + (int)((rad - 15) * cos(a));
                int    y1 = cy + (int)((rad - 15) * sin(a));
                int    x2 = cx + (int)((rad - 5)  * cos(a));
                int    y2 = cy + (int)((rad - 5)  * sin(a));

                MoveToEx(hdc, x1, y1, NULL);
                LineTo(hdc, x2, y2);

                char nm[4];
                sprintf(nm, "%d", i);
                int tx = cx + (int)((rad - 25) * cos(a)) - 5;
                int ty = cy + (int)((rad - 25) * sin(a)) - 5;
                SetBkMode(hdc, TRANSPARENT);
                TextOutA(hdc, tx, ty, nm, (int)strlen(nm));
            }

            // 秒针（红色，RAII）
            {
                double sa = (data->second * 6 - 90) * 3.14159 / 180.0;
                int    sx = cx + (int)((rad - 10) * cos(sa));
                int    sy = cy + (int)((rad - 10) * sin(sa));
                ScopedPen sp(CreatePen(PS_SOLID, 1, RGB(255, 0, 0)));
                GDIObjectSelector sel(hdc, sp);
                MoveToEx(hdc, cx, cy, NULL);
                LineTo(hdc, sx, sy);
            }

            // 分针（黑色，RAII）
            {
                double ma = (data->minute * 6 - 90) * 3.14159 / 180.0;
                int    mx = cx + (int)((rad - 20) * cos(ma));
                int    my = cy + (int)((rad - 20) * sin(ma));
                ScopedPen mp(CreatePen(PS_SOLID, 2, RGB(0, 0, 0)));
                GDIObjectSelector sel(hdc, mp);
                MoveToEx(hdc, cx, cy, NULL);
                LineTo(hdc, mx, my);
            }

            // 时针（黑色，RAII）
            {
                double ha = ((data->hour * 30 + data->minute * 0.5) - 90) * 3.14159 / 180.0;
                int    hx = cx + (int)((rad - 30) * cos(ha));
                int    hy = cy + (int)((rad - 30) * sin(ha));
                ScopedPen hp(CreatePen(PS_SOLID, 3, RGB(0, 0, 0)));
                GDIObjectSelector sel(hdc, hp);
                MoveToEx(hdc, cx, cy, NULL);
                LineTo(hdc, hx, hy);
            }

            // 中心点（RAII）
            {
                ScopedBrush cb(CreateSolidBrush(RGB(0, 0, 0)));
                GDIObjectSelector sel(hdc, cb);
                Ellipse(hdc, cx - 3, cy - 3, cx + 3, cy + 3);
            }

            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            KillTimer(hwnd, 1);
            EndDialog(hwnd, IDOK);
        }
        break;

    case WM_CLOSE:
        KillTimer(hwnd, 1);
        EndDialog(hwnd, IDCANCEL);
        break;

    case WM_DESTROY:
        delete data;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
        break;
    }
    return FALSE;
}

// ========== 添加特殊日期 ==========
static INT_PTR CALLBACK AddSpecialDayDlgProc(HWND hwnd, UINT msg,
                                             WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowTextA(hwnd, _S(STR_ADD_SPECIAL_DAY));
        SetDlgItemTextA(hwnd, IDC_STATIC_SPECIAL_NAME, _S(STR_SPECIAL_NAME_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_SPECIAL_MONTH, _S(STR_SPECIAL_MONTH_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_SPECIAL_DAY, _S(STR_SPECIAL_DAY_LABEL));
        SetDlgItemTextA(hwnd, IDC_CHECK_ANNUAL, _S(STR_SPECIAL_ANNUAL_CHECK));
        SetDlgItemTextA(hwnd, IDC_STATIC_SPECIAL_YEAR, _S(STR_SPECIAL_YEAR_LABEL));
        SetDlgItemTextA(hwnd, IDC_CHECK_SOLAR_TERM, _S(STR_SPECIAL_SOLAR_TERM_CHECK));
        SetDlgItemTextA(hwnd, IDC_STATIC_SPECIAL_TIME, _S(STR_SPECIAL_TIME_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        CheckDlgButton(hwnd, IDC_CHECK_ANNUAL, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_YEAR), FALSE);
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CHECK_ANNUAL) {
            BOOL chk = (IsDlgButtonChecked(hwnd, IDC_CHECK_ANNUAL) == BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_YEAR), !chk);
            if (chk) SetDlgItemTextA(hwnd, IDC_EDIT_YEAR, "");
        }
        else if (LOWORD(wParam) == IDOK) {
            char name[256], mon[16], day[16], yr[16], hr[16], mn[16];
            GetDlgItemTextA(hwnd, IDC_EDIT_NAME,   name, 256);
            GetDlgItemTextA(hwnd, IDC_EDIT_MONTH,  mon,  16);
            GetDlgItemTextA(hwnd, IDC_EDIT_DAY,    day,  16);
            GetDlgItemTextA(hwnd, IDC_EDIT_YEAR,   yr,   16);
            GetDlgItemTextA(hwnd, IDC_EDIT_HOUR,   hr,   16);
            GetDlgItemTextA(hwnd, IDC_EDIT_MINUTE, mn,   16);

            if (!strlen(name)) {
                MessageBoxA(hwnd, _S(STR_SPECIAL_NAME_REQUIRED), _S(STR_ERROR), MB_ICONERROR);
                return TRUE;
            }
            int m = atoi(mon), d = atoi(day);
            if (m < 1 || m > 12 || d < 1 || d > 31) {
                MessageBoxA(hwnd, _S(STR_SPECIAL_MONTH_DAY_INVALID), _S(STR_ERROR), MB_ICONERROR);
                return TRUE;
            }
            bool annual = IsDlgButtonChecked(hwnd, IDC_CHECK_ANNUAL) == BST_CHECKED;
            int y = 0;
            if (!annual) {
                y = atoi(yr);
                if (y < 1900 || y > 2100) {
                    MessageBoxA(hwnd, _S(STR_SPECIAL_YEAR_INVALID), _S(STR_ERROR), MB_ICONERROR);
                    return TRUE;
                }
            }

            SpecialDay sd;
            sd.name       = name;
            sd.month      = m;
            sd.day        = d;
            sd.hour       = atoi(hr);
            sd.minute     = atoi(mn);
            sd.isAnnual   = annual;
            sd.year       = y;
            sd.isSolarTerm = IsDlgButtonChecked(hwnd, IDC_CHECK_SOLAR_TERM) == BST_CHECKED;

            AppConfig::GetInstance().specialDays.push_back(sd);
            AppConfig::GetInstance().SaveSpecialDaysToFile();

            WriteLog("添加特殊日期: " + std::string(name));
            MessageBoxA(hwnd, _S(STR_SPECIAL_ADD_SUCCESS), _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
            EndDialog(hwnd, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
    return FALSE;
}

// ========== 公开接口 ==========
void ShowWorldClockDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL),
                   MAKEINTRESOURCE(IDD_WORLD_CLOCK_DIALOG),
                   parent, WorldClockDlgProc, 0);
}
void ShowAnalogClockDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL),
                   MAKEINTRESOURCE(IDD_ANALOG_CLOCK_DIALOG),
                   parent, AnalogClockDlgProc, 0);
}
void ShowAddSpecialDayDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL),
                   MAKEINTRESOURCE(IDD_SPECIAL_DAY_DIALOG),
                   parent, AddSpecialDayDlgProc, 0);
}
