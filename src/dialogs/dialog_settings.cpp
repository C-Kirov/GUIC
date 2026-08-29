#include "dialog_settings.h"
#include "../version.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"
#include "../i18n.h"
#include "../utils/ntp_server.h"
#include "../utils/http_server.h"
#include <cstdio>

// 窗口设置
static INT_PTR CALLBACK WindowSettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        AppConfig& cfg = AppConfig::GetInstance();
        SetWindowTextA(hwnd, _S(STR_WINDOW_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_CHECK_RESIZABLE, _S(STR_WINDOW_RESIZABLE));
        SetDlgItemTextA(hwnd, IDC_STATIC_WINDOW_SIZE, _S(STR_WINDOW_SIZE_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_WIDTH, _S(STR_WINDOW_WIDTH_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_HEIGHT, _S(STR_WINDOW_HEIGHT_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_WINDOW_RANGE, _S(STR_WINDOW_RANGE_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        CheckDlgButton(hwnd, IDC_CHECK_RESIZABLE, cfg.resizable ? BST_CHECKED : BST_UNCHECKED);
        char buf[16];
        sprintf(buf, "%d", cfg.windowWidth);
        SetDlgItemTextA(hwnd, IDC_EDIT_WINDOW_WIDTH, buf);
        sprintf(buf, "%d", cfg.windowHeight);
        SetDlgItemTextA(hwnd, IDC_EDIT_WINDOW_HEIGHT, buf);
        return TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            AppConfig& cfg = AppConfig::GetInstance();
            cfg.resizable = (IsDlgButtonChecked(hwnd, IDC_CHECK_RESIZABLE) == BST_CHECKED);
            char widthBuf[16], heightBuf[16];
            GetDlgItemTextA(hwnd, IDC_EDIT_WINDOW_WIDTH, widthBuf, sizeof(widthBuf));
            GetDlgItemTextA(hwnd, IDC_EDIT_WINDOW_HEIGHT, heightBuf, sizeof(heightBuf));
            int w = atoi(widthBuf), h = atoi(heightBuf);
            if (w >= 300 && w <= 2000 && h >= 200 && h <= 1500) {
                cfg.windowWidth  = w;
                cfg.windowHeight = h;
                HWND parent = GetParent(hwnd);
                if (parent) {
                    RECT rect = {0, 0, w, h};
                    DWORD style = GetWindowLong(parent, GWL_STYLE);
                    BOOL hasMenu = (GetMenu(parent) != NULL);
                    AdjustWindowRect(&rect, style, hasMenu);
                    SetWindowPos(parent, NULL, 0, 0,
                                 rect.right - rect.left, rect.bottom - rect.top,
                                 SWP_NOMOVE | SWP_NOZORDER);
                }
                EndDialog(hwnd, IDOK);
            } else {
                MessageBoxA(hwnd, _S(STR_WINDOW_SIZE_INVALID), _S(STR_ERROR), MB_OK | MB_ICONERROR);
            }
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    }
    }
    return FALSE;
}

// 字体设置
static INT_PTR CALLBACK FontSettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        AppConfig& cfg = AppConfig::GetInstance();
        SetWindowTextA(hwnd, _S(STR_FONT_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_STATIC_FONT_SIZE, _S(STR_FONT_SIZE_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_FONT_NAME, _S(STR_FONT_NAME_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        char buf[16];
        sprintf(buf, "%d", cfg.fontSize);
        SetDlgItemTextA(hwnd, IDC_EDIT_FONT_SIZE, buf);
        SetDlgItemTextA(hwnd, IDC_EDIT_FONT_NAME, cfg.fontName.c_str());
        return TRUE;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            char sizeBuf[16], nameBuf[256];
            GetDlgItemTextA(hwnd, IDC_EDIT_FONT_SIZE, sizeBuf, sizeof(sizeBuf));
            GetDlgItemTextA(hwnd, IDC_EDIT_FONT_NAME, nameBuf, sizeof(nameBuf));
            int sz = atoi(sizeBuf);
            if (sz > 0 && sz <= 100) AppConfig::GetInstance().fontSize = sz;
            if (strlen(nameBuf) > 0) AppConfig::GetInstance().fontName = nameBuf;
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    }
    }
    return FALSE;
}

// 颜色设置
static INT_PTR CALLBACK ColorSettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowTextA(hwnd, _S(STR_COLOR_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_STATIC_COLOR_LABEL, _S(STR_COLOR_LABEL));
        SetDlgItemTextA(hwnd, IDC_BUTTON_COLOR, _S(STR_COLOR_BUTTON));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        return TRUE;
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_BUTTON_COLOR) {
            CHOOSECOLORA cc = {0};
            static COLORREF custColors[16] = {0};
            cc.lStructSize  = sizeof(cc);
            cc.hwndOwner    = hwnd;
            cc.lpCustColors = custColors;
            cc.rgbResult    = AppConfig::GetInstance().fontColor;
            cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorA(&cc))
                AppConfig::GetInstance().fontColor = cc.rgbResult;
        } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, LOWORD(wParam));
        }
        break;
    }
    }
    return FALSE;
}

// 天气城市设置
static INT_PTR CALLBACK WeatherCityDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowTextA(hwnd, _S(STR_WEATHER_CITY_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_STATIC_WEATHER_LABEL, _S(STR_WEATHER_CITY_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        SetDlgItemTextA(hwnd, IDC_EDIT_WEATHER_CITY,
                        AppConfig::GetInstance().weatherCity.c_str());
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            char buf[256];
            GetDlgItemTextA(hwnd, IDC_EDIT_WEATHER_CITY, buf, sizeof(buf));
            AppConfig::GetInstance().weatherCity = buf;
            AppConfig::GetInstance().SaveConfig();
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    }
    return FALSE;
}

// NTP 服务器设置
static INT_PTR CALLBACK NtpServerDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowTextA(hwnd, _S(STR_NTP_SERVER_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_STATIC_NTP_LABEL, _S(STR_NTP_SERVER_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        SetDlgItemTextA(hwnd, IDC_EDIT_NTP_SERVER,
                        AppConfig::GetInstance().ntpCustomServer.c_str());
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            char buf[256];
            GetDlgItemTextA(hwnd, IDC_EDIT_NTP_SERVER, buf, sizeof(buf));
            AppConfig::GetInstance().ntpCustomServer = buf;
            AppConfig::GetInstance().SaveConfig();
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    }
    return FALSE;
}

// 服务器端口设置对话框过程（NTP 服务器 / Web 控制面板端口）
static INT_PTR CALLBACK ServerSettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        AppConfig& cfg = AppConfig::GetInstance();
        SetWindowTextA(hwnd, _S(STR_SERVER_SETTINGS));
        SetDlgItemTextA(hwnd, IDC_STATIC_NTP_PORT_LABEL, _S(STR_NTP_PORT_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_HTTP_PORT_LABEL, _S(STR_HTTP_PORT_LABEL));
        SetDlgItemTextA(hwnd, IDC_STATIC_PORT_RANGE, _S(STR_PORT_RANGE_LABEL));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
        char buf[16];
        sprintf(buf, "%d", cfg.ntpServerPort);
        SetDlgItemTextA(hwnd, IDC_EDIT_NTP_PORT, buf);
        sprintf(buf, "%d", cfg.httpServerPort);
        SetDlgItemTextA(hwnd, IDC_EDIT_HTTP_PORT, buf);
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            AppConfig& cfg = AppConfig::GetInstance();
            char ntpBuf[16], httpBuf[16];
            GetDlgItemTextA(hwnd, IDC_EDIT_NTP_PORT, ntpBuf, sizeof(ntpBuf));
            GetDlgItemTextA(hwnd, IDC_EDIT_HTTP_PORT, httpBuf, sizeof(httpBuf));
            int ntpPort  = atoi(ntpBuf);
            int httpPort = atoi(httpBuf);
            if (ntpPort < 1 || ntpPort > 65535 || httpPort < 1 || httpPort > 65535) {
                MessageBoxA(hwnd, _S(STR_PORT_INVALID), _S(STR_ERROR), MB_OK | MB_ICONERROR);
                return TRUE;
            }
            bool ntpRestart  = IsNtpServerRunning();
            bool httpRestart = IsHttpServerRunning();
            cfg.ntpServerPort  = ntpPort;
            cfg.httpServerPort = httpPort;
            cfg.SaveConfig();
            // 运行中的服务器按新端口重启
            if (ntpRestart) {
                StopNtpServer();
                StartNtpServer((unsigned short)ntpPort);
            }
            if (httpRestart) {
                StopHttpServer();
                StartHttpServer((unsigned short)httpPort);
            }
            MessageBoxA(hwnd, _S(STR_SERVER_PORTS_UPDATED), _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
            EndDialog(hwnd, IDOK);
        } else if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwnd, IDCANCEL);
        }
        break;
    }
    return FALSE;
}

// 关于对话框
static INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG) {
        SetWindowTextA(hwnd, _S(STR_ABOUT_TITLE));
        SetDlgItemTextA(hwnd, IDC_STATIC_ABOUT_TITLE, _S(STR_ABOUT_DESC));
        char verBuf[32];
        sprintf(verBuf, _S(STR_ABOUT_VERSION),
                GUIC_VERSION_MAJOR, GUIC_VERSION_MINOR,
                GUIC_VERSION_BUILD, GUIC_VERSION_REV);
        SetDlgItemTextA(hwnd, IDC_STATIC_ABOUT_VER, verBuf);
        SetDlgItemTextA(hwnd, IDC_STATIC_ABOUT_FEATURES, _S(STR_ABOUT_FEATURES));
        SetDlgItemTextA(hwnd, IDC_STATIC_ABOUT_NETWORK, _S(STR_ABOUT_NETWORK));
        SetDlgItemTextA(hwnd, IDC_STATIC_ABOUT_COMPILE, _S(STR_ABOUT_COMPILE));
        SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
        return TRUE;
    }
    if (msg == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
        EndDialog(hwnd, LOWORD(wParam));
    return FALSE;
}

void ShowWindowSettingsDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WINDOW_SETTINGS_DIALOG),
                   parent, WindowSettingsDlgProc, 0);
}
void ShowFontSettingsDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FONT_SETTINGS_DIALOG),
                   parent, FontSettingsDlgProc, 0);
}
void ShowColorSettingsDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLOR_SETTINGS_DIALOG),
                   parent, ColorSettingsDlgProc, 0);
}
void ShowWeatherCityDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WEATHER_CITY_DIALOG),
                   parent, WeatherCityDlgProc, 0);
}
void ShowNtpServerDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_NTP_SERVER_DIALOG),
                   parent, NtpServerDlgProc, 0);
}
void ShowAboutDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT_DIALOG),
                   parent, AboutDlgProc, 0);
}
void ShowServerSettingsDialog(HWND parent) {
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SERVER_SETTINGS_DIALOG),
                   parent, ServerSettingsDlgProc, 0);
}
