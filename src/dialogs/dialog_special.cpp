#include "dialog_special.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// ========== 世界时钟（注册表TZI读取，带后备偏移） ==========
struct TimeZoneEntry {
    const char* displayName;
    const char* winKey;
    int   utcOffsetMinutes;   // 后备：无DST时的UTC偏移（分钟）
};

static const std::vector<TimeZoneEntry> g_timeZones = {
    { "北京 (UTC+8)",   "China Standard Time",     480 },
    { "东京 (UTC+9)",   "Tokyo Standard Time",      540 },
    { "纽约 (UTC-5)",   "Eastern Standard Time",   -300 },
    { "伦敦 (UTC+0)",   "GMT Standard Time",          0 },
    { "悉尼 (UTC+11)",  "AUS Eastern Standard Time", 660 },
    { "莫斯科 (UTC+3)", "Russian Standard Time",     180 },
    { "巴黎 (UTC+1)",   "Romance Standard Time",      60 },
    { "洛杉矶 (UTC-8)", "Pacific Standard Time",    -480 }
};

// 注册表中存储的 TZI 结构（与 TIME_ZONE_INFORMATION 的前部兼容，但不含名称）
struct REG_TZI_FORMAT {
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
};

// 从注册表 TZI 值读取时区偏置和夏令时规则，填充到 TIME_ZONE_INFORMATION
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

    // 填充 TIME_ZONE_INFORMATION（名称留空，不影响时间转换）
    ZeroMemory(&tzi, sizeof(tzi));
    tzi.Bias = regTzi.Bias;
    tzi.StandardBias = regTzi.StandardBias;
    tzi.DaylightBias = regTzi.DaylightBias;
    tzi.StandardDate = regTzi.StandardDate;
    tzi.DaylightDate = regTzi.DaylightDate;

    // 名称可选，留空即可
    return true;
}

// 使用夏令时规则转换
static bool ConvertUtcToTimeZone(const SYSTEMTIME& utc, const TIME_ZONE_INFORMATION& tzi, SYSTEMTIME& local)
{
    return SystemTimeToTzSpecificLocalTime(&tzi, &utc, &local) != 0;
}

// 纯偏移计算（忽略夏令时）
static void ApplySimpleOffset(const SYSTEMTIME& utc, int offsetMinutes, SYSTEMTIME& local)
{
    FILETIME ft;
    SystemTimeToFileTime(&utc, &ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;

    LONGLONG offset100ns = (LONGLONG)offsetMinutes * 60LL * 10000000LL;
    ul.QuadPart += offset100ns;

    ft.dwLowDateTime = ul.LowPart;
    ft.dwHighDateTime = ul.HighPart;
    FileTimeToSystemTime(&ft, &local);
}

struct WorldClockState { HWND list; HWND display; int sel; };
static WorldClockState g_wc;

static INT_PTR CALLBACK WorldClockDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        g_wc.list = GetDlgItem(hwnd, IDC_LIST_WORLD_CLOCKS);
        g_wc.display = GetDlgItem(hwnd, IDC_STATIC_WORLD_CLOCK_DISPLAY);
        for (size_t i=0;i<g_timeZones.size();++i)
            SendMessage(g_wc.list, LB_ADDSTRING, 0, (LPARAM)g_timeZones[i].displayName);
        SendMessage(g_wc.list, LB_SETCURSEL, 0, 0);
        g_wc.sel = 0;
        SetTimer(hwnd, 1, 1000, NULL);
        return TRUE;
    case WM_TIMER:
        if (g_wc.list && g_wc.display && g_wc.sel>=0 && g_wc.sel<(int)g_timeZones.size()) {
            SYSTEMTIME utc, local;
            GetSystemTime(&utc);
            bool converted = false;
            const TimeZoneEntry& entry = g_timeZones[g_wc.sel];

            // 方法1：从注册表读取夏令时规则
            TIME_ZONE_INFORMATION tzi;
            if (GetTimeZoneInfoByName(entry.winKey, tzi)) {
                if (ConvertUtcToTimeZone(utc, tzi, local))
                    converted = true;
            }

            // 方法2：后备纯偏移
            if (!converted) {
                ApplySimpleOffset(utc, entry.utcOffsetMinutes, local);
                converted = true;
            }

            char buf[256];
            sprintf(buf,"%s\n%04d-%02d-%02d %02d:%02d:%02d", entry.displayName,
                    local.wYear, local.wMonth, local.wDay,
                    local.wHour, local.wMinute, local.wSecond);
            SetWindowText(g_wc.display, buf);
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            KillTimer(hwnd, 1);
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDC_LIST_WORLD_CLOCKS && HIWORD(wParam) == LBN_SELCHANGE) {
            int s = SendMessage(g_wc.list, LB_GETCURSEL, 0, 0);
            if (s != LB_ERR) { g_wc.sel = s; SendMessage(hwnd, WM_TIMER, 1, 0); }
        }
        break;
    case WM_CLOSE:
        KillTimer(hwnd, 1);
        EndDialog(hwnd, IDCANCEL);
        break;
    }
    return FALSE;
}

// ========== 模拟时钟 ==========
static int g_ah, g_am, g_as; static HWND g_clockWnd;
static INT_PTR CALLBACK AnalogClockDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        g_clockWnd = GetDlgItem(hwnd, IDC_STATIC_ANALOG_CLOCK);
        SetTimer(hwnd, 1, 1000, NULL);
        return TRUE;
    case WM_TIMER: {
        SYSTEMTIME st; AppConfig::GetInstance().GetCurrentDateTime(st);
        g_ah=st.wHour%12; g_am=st.wMinute; g_as=st.wSecond;
        if (g_clockWnd) InvalidateRect(g_clockWnd, NULL, TRUE);
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->hwndItem == g_clockWnd && dis->itemAction == ODA_DRAWENTIRE) {
            HDC hdc=dis->hDC; RECT r=dis->rcItem; int w=r.right-r.left, h=r.bottom-r.top;
            int cx=w/2, cy=h/2, rad=std::min(w,h)/2-10;
            HBRUSH wbr=CreateSolidBrush(RGB(255,255,255)); FillRect(hdc,&r,wbr); DeleteObject(wbr);
            HPEN p=CreatePen(PS_SOLID,2,RGB(0,0,0)); HGDIOBJ old=SelectObject(hdc,p);
            SelectObject(hdc,GetStockObject(NULL_BRUSH));
            Ellipse(hdc,cx-rad,cy-rad,cx+rad,cy+rad);
            for (int i=1;i<=12;i++) {
                double a=(i*30-90)*3.14159/180.0;
                int x1=cx+(int)((rad-15)*cos(a)), y1=cy+(int)((rad-15)*sin(a));
                int x2=cx+(int)((rad-5)*cos(a)), y2=cy+(int)((rad-5)*sin(a));
                MoveToEx(hdc,x1,y1,NULL); LineTo(hdc,x2,y2);
                char nm[4]; sprintf(nm,"%d",i);
                int tx=cx+(int)((rad-25)*cos(a))-5, ty=cy+(int)((rad-25)*sin(a))-5;
                SetBkMode(hdc,TRANSPARENT); TextOut(hdc,tx,ty,nm,strlen(nm));
            }
            double sa=(g_as*6-90)*3.14159/180.0; int sx=cx+(int)((rad-10)*cos(sa)), sy=cy+(int)((rad-10)*sin(sa));
            HPEN sp=CreatePen(PS_SOLID,1,RGB(255,0,0)); SelectObject(hdc,sp);
            MoveToEx(hdc,cx,cy,NULL); LineTo(hdc,sx,sy); DeleteObject(sp);
            double ma=(g_am*6-90)*3.14159/180.0; int mx=cx+(int)((rad-20)*cos(ma)), my=cy+(int)((rad-20)*sin(ma));
            HPEN mp=CreatePen(PS_SOLID,2,RGB(0,0,0)); SelectObject(hdc,mp);
            MoveToEx(hdc,cx,cy,NULL); LineTo(hdc,mx,my); DeleteObject(mp);
            double ha=((g_ah*30+g_am*0.5)-90)*3.14159/180.0; int hx=cx+(int)((rad-30)*cos(ha)), hy=cy+(int)((rad-30)*sin(ha));
            HPEN hp=CreatePen(PS_SOLID,3,RGB(0,0,0)); SelectObject(hdc,hp);
            MoveToEx(hdc,cx,cy,NULL); LineTo(hdc,hx,hy); DeleteObject(hp);
            HBRUSH cb=CreateSolidBrush(RGB(0,0,0)); SelectObject(hdc,cb);
            Ellipse(hdc,cx-3,cy-3,cx+3,cy+3); DeleteObject(cb);
            SelectObject(hdc,old); DeleteObject(p);
            return TRUE;
        } break;
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
    }
    return FALSE;
}

// ========== 添加特殊日子 ==========
static INT_PTR CALLBACK AddSpecialDayDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        CheckDlgButton(hwnd, IDC_CHECK_ANNUAL, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_YEAR), FALSE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CHECK_ANNUAL) {
            BOOL chk = (IsDlgButtonChecked(hwnd, IDC_CHECK_ANNUAL) == BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_YEAR), !chk);
            if (chk) SetDlgItemText(hwnd, IDC_EDIT_YEAR, "");
        } else if (LOWORD(wParam) == IDOK) {
            char name[256], mon[16], day[16], yr[16], hr[16], mn[16];
            GetDlgItemTextA(hwnd, IDC_EDIT_NAME, name, 256);
            GetDlgItemTextA(hwnd, IDC_EDIT_MONTH, mon, 16);
            GetDlgItemTextA(hwnd, IDC_EDIT_DAY, day, 16);
            GetDlgItemTextA(hwnd, IDC_EDIT_YEAR, yr, 16);
            GetDlgItemTextA(hwnd, IDC_EDIT_HOUR, hr, 16);
            GetDlgItemTextA(hwnd, IDC_EDIT_MINUTE, mn, 16);
            if (!strlen(name)) { MessageBox(hwnd, "请输入名称！", "错误", MB_ICONERROR); return TRUE; }
            int m=atoi(mon), d=atoi(day);
            if (m<1||m>12||d<1||d>31) { MessageBox(hwnd, "月份/日期无效", "错误", MB_ICONERROR); return TRUE; }
            bool annual = IsDlgButtonChecked(hwnd, IDC_CHECK_ANNUAL) == BST_CHECKED;
            int y=0;
            if (!annual) { y=atoi(yr); if (y<1900||y>2100) { MessageBox(hwnd, "年份无效", "错误", MB_ICONERROR); return TRUE; } }
            SpecialDay sd; sd.name = name; sd.month = m; sd.day = d;
            sd.hour = atoi(hr); sd.minute = atoi(mn);
            sd.isAnnual = annual; sd.year = y;
            sd.isSolarTerm = IsDlgButtonChecked(hwnd, IDC_CHECK_SOLAR_TERM) == BST_CHECKED;
            AppConfig::GetInstance().specialDays.push_back(sd);
            AppConfig::GetInstance().SaveSpecialDaysToFile();
            WriteLog("添加特殊日子: " + std::string(name));
            MessageBox(hwnd, "添加成功", "提示", MB_OK | MB_ICONINFORMATION);
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
    return FALSE;
}

// ========== 预览提示信息 ==========
static INT_PTR CALLBACK PreviewMessageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetDlgItemTextA(hwnd, IDC_EDIT_PREVIEW_MESSAGE, AppConfig::GetInstance().message.c_str());
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            EndDialog(hwnd, LOWORD(wParam));
        break;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
    return FALSE;
}

void ShowWorldClockDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WORLD_CLOCK_DIALOG), parent, WorldClockDlgProc, 0);
}
void ShowAnalogClockDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ANALOG_CLOCK_DIALOG), parent, AnalogClockDlgProc, 0);
}
void ShowAddSpecialDayDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SPECIAL_DAY_DIALOG), parent, AddSpecialDayDlgProc, 0);
}
void ShowPreviewMessageDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PREVIEW_MESSAGE_DIALOG), parent, PreviewMessageDlgProc, 0);
}
