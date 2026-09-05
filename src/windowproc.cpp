#include "main.h"
#include "config/config.h"
#include "utils/logger.h"
#include "utils/renderer.h"
#include "utils/gdi_utils.h"
#include "utils/lunar.h"
#include "utils/lunar_online.h"
#include "utils/weather.h"
#include "utils/utils.h"
#include "utils/ntp_server.h"
#include "utils/http_server.h"
#include "dialogs/dialog_message.h"
#include "dialogs/dialog_countdown.h"
#include "dialogs/dialog_settings.h"
#include "dialogs/dialog_special.h"
#include "i18n.h"
#include "resource.h"

WeatherInfo g_weather;
bool        g_weatherLoading = false;
LunarOnlineData g_lunarOnline = {false, -1, {0}, 0, 0, 0, false};
bool        g_lunarOnlineLoading = false;
static FILETIME g_lastLunarAttempt = {0, 0};

// 子菜单句柄（用于刷新复选状态）
static HMENU g_hSubSettings = NULL;
static HMENU g_hSubDisplay  = NULL;
static HMENU g_hSubServers  = NULL;
static HMENU g_hSubClock    = NULL;
static HMENU g_hSubLang     = NULL;

// 刷新各子菜单的复选标记（显示开关 / 服务器运行状态 / 当前语言）
static void UpdateMenuChecks(HWND hwnd)
{
    AppConfig& cfg = AppConfig::GetInstance();

    if (g_hSubDisplay) {
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_CLOCK,
                      cfg.showClock ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_COUNTDOWN,
                      cfg.showCountdown ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_MESSAGE,
                      cfg.showMessage ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_LAYOUT,
                      cfg.autoLayout ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_LUNAR,
                      cfg.showLunar ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_WEATHER,
                      cfg.showWeather ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubDisplay, ID_MENU_TOGGLE_BORDERLESS,
                      cfg.borderless ? MF_CHECKED : MF_UNCHECKED);
    }

    if (g_hSubServers) {
        CheckMenuItem(g_hSubServers, ID_MENU_TOGGLE_NTP_SERVER,
                      IsNtpServerRunning() ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubServers, ID_MENU_TOGGLE_HTTP_SERVER,
                      IsHttpServerRunning() ? MF_CHECKED : MF_UNCHECKED);
    }

    if (g_hSubLang) {
        CheckMenuItem(g_hSubLang, ID_MENU_LANGUAGE_ZH,
                      cfg.language == AppLanguage::ZH_CN ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(g_hSubLang, ID_MENU_LANGUAGE_EN,
                      cfg.language == AppLanguage::EN_US ? MF_CHECKED : MF_UNCHECKED);
    }
}

static void CreateMainMenu(HWND hwnd)
{
    AppConfig& cfg = AppConfig::GetInstance();

    HMENU hMenu = CreateMenu();
    HMENU hSub  = CreatePopupMenu();

    // ---- 设置 ----
    g_hSubSettings = CreatePopupMenu();
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SET_MESSAGE,          _S(STR_SET_MESSAGE));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SET_COUNTDOWN_ONCE,   _S(STR_SET_COUNTDOWN_ONCE));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SET_COUNTDOWN_DAILY,  _S(STR_SET_COUNTDOWN_DAILY));
    AppendMenuA(g_hSubSettings, MF_SEPARATOR, 0, NULL);
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_WINDOW,      _S(STR_WINDOW_SETTINGS));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_FONT,        _S(STR_FONT_SETTINGS));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_COLOR,       _S(STR_COLOR_SETTINGS));
    AppendMenuA(g_hSubSettings, MF_SEPARATOR, 0, NULL);
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_ADD_SPECIAL_DAY,      _S(STR_ADD_SPECIAL_DAY));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_WEATHER_CITY, _S(STR_WEATHER_CITY_SETTINGS));
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_NTP,         _S(STR_NTP_SERVER_SETTINGS));
    AppendMenuA(g_hSubSettings, MF_SEPARATOR, 0, NULL);
    AppendMenuA(g_hSubSettings, MF_STRING, ID_MENU_SETTINGS_SERVER,      _S(STR_SERVER_SETTINGS));
    AppendMenuA(hSub, MF_POPUP, (UINT_PTR)g_hSubSettings, _S(STR_SUBMENU_SETTINGS));

    // ---- 显示（复选框切换）----
    g_hSubDisplay = CreatePopupMenu();
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_CLOCK,        _S(STR_SHOW_CLOCK));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_COUNTDOWN,    _S(STR_SHOW_COUNTDOWN));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_MESSAGE,      _S(STR_SHOW_MESSAGE));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_LAYOUT,       _S(STR_SHOW_LAYOUT));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_LUNAR,        _S(STR_SHOW_LUNAR));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_WEATHER,      _S(STR_SHOW_WEATHER));
    AppendMenuA(g_hSubDisplay, MF_STRING, ID_MENU_TOGGLE_BORDERLESS,   _S(STR_SHOW_BORDERLESS));
    AppendMenuA(hSub, MF_POPUP, (UINT_PTR)g_hSubDisplay, _S(STR_SUBMENU_VIEW));

    // ---- 服务器（运行状态复选）----
    g_hSubServers = CreatePopupMenu();
    AppendMenuA(g_hSubServers, MF_STRING, ID_MENU_TOGGLE_NTP_SERVER,  _S(STR_SERVER_NTP));
    AppendMenuA(g_hSubServers, MF_STRING, ID_MENU_TOGGLE_HTTP_SERVER, _S(STR_SERVER_HTTP));
    AppendMenuA(hSub, MF_POPUP, (UINT_PTR)g_hSubServers, _S(STR_SUBMENU_SERVERS));

    // ---- 时钟工具 ----
    g_hSubClock = CreatePopupMenu();
    AppendMenuA(g_hSubClock, MF_STRING, ID_MENU_WORLD_CLOCK,  _S(STR_WORLD_CLOCK));
    AppendMenuA(g_hSubClock, MF_STRING, ID_MENU_ANALOG_CLOCK, _S(STR_ANALOG_CLOCK));
    AppendMenuA(hSub, MF_POPUP, (UINT_PTR)g_hSubClock, _S(STR_SUBMENU_CLOCK));

    // ---- 语言 ----
    g_hSubLang = CreatePopupMenu();
    AppendMenuA(g_hSubLang, MF_STRING, ID_MENU_LANGUAGE_ZH, _S(STR_LANGUAGE_ZH));
    AppendMenuA(g_hSubLang, MF_STRING, ID_MENU_LANGUAGE_EN, _S(STR_LANGUAGE_EN));
    AppendMenuA(hSub, MF_POPUP, (UINT_PTR)g_hSubLang, _S(STR_LANGUAGE));

    AppendMenuA(hSub, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hSub, MF_STRING, ID_MENU_ABOUT, _S(STR_ABOUT));

    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hSub, _S(STR_MENU));
    SetMenu(hwnd, hMenu);

    UpdateMenuChecks(hwnd);
}

// 语言切换后重建主菜单（菜单项文本随语言更新）
static void RebuildMainMenu(HWND hwnd)
{
    HMENU hMenu = GetMenu(hwnd);
    if (hMenu) {
        SetMenu(hwnd, NULL);
        DestroyMenu(hMenu);
    }
    CreateMainMenu(hwnd);
    DrawMenuBar(hwnd);
}

static DWORD WINAPI WeatherThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;
    AppConfig& cfg = AppConfig::GetInstance();
    WeatherInfo info = FetchWeather(cfg.weatherCity);
    WeatherInfo* pInfo = new WeatherInfo(info);
    PostMessage(hwnd, WM_WEATHER_READY, (WPARAM)pInfo, 0);
    return 0;
}

static DWORD WINAPI LunarOnlineThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;
    SYSTEMTIME st;
    GetLocalTime(&st);
    LunarOnlineData* pData = new LunarOnlineData();
    FetchOnlineLunar(st, *pData);
    PostMessage(hwnd, WM_LUNAR_READY, (WPARAM)pData, 0);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    AppConfig& cfg = AppConfig::GetInstance();

    switch (uMsg)
    {
        case WM_CREATE:
        {
            WriteLog("窗口创建");
            WriteSystemInfoIfNeeded();
            CreateMainMenu(hwnd);

            SetTimer(hwnd, 1, 100, NULL);

            HANDLE hFirstSync = CreateThread(NULL, 0, [](LPVOID)->DWORD {
                AppConfig::GetInstance().SyncNetworkTime();
                return 0;
            }, NULL, 0, NULL);
            if (hFirstSync) CloseHandle(hFirstSync);

            SetTimer(hwnd, 2, 300000, NULL);
            SetTimer(hwnd, 3, 2000, NULL);

            if (cfg.showWeather && !cfg.weatherCity.empty()) {
                g_weatherLoading = true;
                HANDLE hWeather = CreateThread(NULL, 0, WeatherThreadProc, hwnd, 0, NULL);
                if (hWeather) CloseHandle(hWeather);
            }

            // 自动恢复上次运行的服务器
            if (cfg.ntpServerEnabled)
                StartNtpServer((unsigned short)cfg.ntpServerPort);
            if (cfg.httpServerEnabled)
                StartHttpServer((unsigned short)cfg.httpServerPort);

            HINSTANCE hInst = GetModuleHandle(NULL);
            HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
            if (hIcon) {
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
            break;
        }

        case WM_DPICHANGED:
        {
            RECT* pRect = (RECT*)lParam;
            SetWindowPos(hwnd, NULL,
                         pRect->left, pRect->top,
                         pRect->right - pRect->left,
                         pRect->bottom - pRect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_INITMENUPOPUP:
            // 打开菜单时刷新复选标记
            UpdateMenuChecks(hwnd);
            break;

        case WM_SIZE:
        {
            if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                cfg.windowWidth  = rect.right - rect.left;
                cfg.windowHeight = rect.bottom - rect.top;
            }
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case ID_MENU_SET_MESSAGE:
                    ShowMessageDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SET_COUNTDOWN_ONCE:
                    ShowCountdownOnceDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SET_COUNTDOWN_DAILY:
                    ShowCountdownDailyDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_CLOCK:
                    cfg.showClock = !cfg.showClock; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_COUNTDOWN:
                    cfg.showCountdown = !cfg.showCountdown; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_LAYOUT:
                    cfg.autoLayout = !cfg.autoLayout; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_MESSAGE:
                    cfg.showMessage = !cfg.showMessage; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_LUNAR:
                    cfg.showLunar = !cfg.showLunar; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_WEATHER:
                    cfg.showWeather = !cfg.showWeather; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    if (cfg.showWeather && !g_weatherLoading) {
                        g_weatherLoading = true;
                        HANDLE h = CreateThread(NULL, 0, WeatherThreadProc, hwnd, 0, NULL);
                        if (h) CloseHandle(h);
                    }
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_TOGGLE_BORDERLESS:
                    cfg.borderless = !cfg.borderless; cfg.SaveConfig();
                    UpdateMenuChecks(hwnd);
                    MessageBoxA(hwnd, _S(STR_BORDERLESS_NOTE),
                                _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
                    break;

                case ID_MENU_SETTINGS_WINDOW:
                    ShowWindowSettingsDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SETTINGS_FONT:
                    ShowFontSettingsDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SETTINGS_COLOR:
                    ShowColorSettingsDialog(hwnd); cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SETTINGS_WEATHER_CITY:
                    ShowWeatherCityDialog(hwnd); cfg.SaveConfig();
                    if (cfg.showWeather && !g_weatherLoading) {
                        g_weatherLoading = true;
                        HANDLE h = CreateThread(NULL, 0, WeatherThreadProc, hwnd, 0, NULL);
                        if (h) CloseHandle(h);
                    }
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_SETTINGS_NTP:
                    ShowNtpServerDialog(hwnd); cfg.SaveConfig(); break;
                case ID_MENU_SETTINGS_SERVER:
                    ShowServerSettingsDialog(hwnd); cfg.SaveConfig();
                    UpdateMenuChecks(hwnd); break;

                case ID_MENU_LANGUAGE_ZH:
                    cfg.language = AppLanguage::ZH_CN; cfg.SaveConfig();
                    I18nInit(Language::ZH_CN);
                    RebuildMainMenu(hwnd);
                    SetWindowTextA(hwnd, _S(STR_WINDOW_TITLE));
                    MessageBoxA(hwnd, _S(STR_LANGUAGE_SWITCHED),
                                _S(STR_TIP), MB_OK | MB_ICONINFORMATION); break;
                case ID_MENU_LANGUAGE_EN:
                    cfg.language = AppLanguage::EN_US; cfg.SaveConfig();
                    I18nInit(Language::EN_US);
                    RebuildMainMenu(hwnd);
                    SetWindowTextA(hwnd, _S(STR_WINDOW_TITLE));
                    MessageBoxA(hwnd, _S(STR_LANGUAGE_SWITCHED),
                                _S(STR_TIP), MB_OK | MB_ICONINFORMATION); break;

                case ID_MENU_ADD_SPECIAL_DAY:
                    ShowAddSpecialDayDialog(hwnd); cfg.SaveSpecialDaysToFile();
                    InvalidateRect(hwnd, NULL, TRUE); break;
                case ID_MENU_WORLD_CLOCK:
                    ShowWorldClockDialog(hwnd); break;
                case ID_MENU_ANALOG_CLOCK:
                    ShowAnalogClockDialog(hwnd); break;

                case ID_MENU_TOGGLE_NTP_SERVER:
                    if (IsNtpServerRunning()) {
                        StopNtpServer();
                        cfg.ntpServerEnabled = false;
                        cfg.SaveConfig();
                        MessageBoxA(hwnd, _S(STR_NTP_SERVER_STOPPED), _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
                    } else {
                        StartNtpServer((unsigned short)cfg.ntpServerPort);
                        cfg.ntpServerEnabled = true;
                        cfg.SaveConfig();
                        char buf[64];
                        sprintf(buf, _S(STR_NTP_SERVER_STARTED_FMT), cfg.ntpServerPort);
                        MessageBoxA(hwnd, buf, _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
                    }
                    UpdateMenuChecks(hwnd); break;
                case ID_MENU_TOGGLE_HTTP_SERVER:
                    if (IsHttpServerRunning()) {
                        StopHttpServer();
                        cfg.httpServerEnabled = false;
                        cfg.SaveConfig();
                        MessageBoxA(hwnd, _S(STR_HTTP_SERVER_STOPPED), _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
                    } else {
                        StartHttpServer((unsigned short)cfg.httpServerPort);
                        cfg.httpServerEnabled = true;
                        cfg.SaveConfig();
                        char buf[128];
                        sprintf(buf, _S(STR_HTTP_SERVER_STARTED_FMT), cfg.httpServerPort);
                        MessageBoxA(hwnd, buf, _S(STR_TIP), MB_OK | MB_ICONINFORMATION);
                    }
                    UpdateMenuChecks(hwnd); break;

                case ID_MENU_ABOUT:
                    ShowAboutDialog(hwnd); break;
            }
            break;
        }

        case WM_WEATHER_READY:
        {
            WeatherInfo* pInfo = (WeatherInfo*)wParam;
            if (pInfo) {
                g_weather = *pInfo;
                delete pInfo;
                g_weatherLoading = false;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_LUNAR_READY:
        {
            LunarOnlineData* pData = (LunarOnlineData*)wParam;
            if (pData) {
                g_lunarOnline = *pData;
                delete pData;
                g_lunarOnlineLoading = false;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            int w = rect.right  - rect.left;
            int h = rect.bottom - rect.top;

            ScopedDC    memDC(CreateCompatibleDC(hdc));
            ScopedBitmap hBitmap(CreateCompatibleBitmap(hdc, w, h));
            GDIObjectSelector sel(memDC, hBitmap);

            Renderer::Draw(hwnd, memDC);

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_TIMER:
            if (wParam == 1) {
                InvalidateRect(hwnd, NULL, FALSE);

                // 在线农历：跨天刷新；获取失败时每 30 分钟重试
                SYSTEMTIME now;
                GetLocalTime(&now);
                bool needLunar;
                if (g_lunarOnline.valid) {
                    needLunar = (g_lunarOnline.fetchTime.wYear != now.wYear ||
                                g_lunarOnline.fetchTime.wMonth != now.wMonth ||
                                g_lunarOnline.fetchTime.wDay != now.wDay);
                } else {
                    // 失败重试：距上次尝试 >= 30 分钟（1.8e10 * 100ns）
                    FILETIME nowFt;
                    GetSystemTimeAsFileTime(&nowFt);
                    ULARGE_INTEGER uNow, uLast;
                    uNow.LowPart = nowFt.dwLowDateTime; uNow.HighPart = nowFt.dwHighDateTime;
                    uLast.LowPart = g_lastLunarAttempt.dwLowDateTime; uLast.HighPart = g_lastLunarAttempt.dwHighDateTime;
                    if (uLast.QuadPart == 0) {
                        needLunar = true;   // 从未尝试过，立即获取
                    } else {
                        needLunar = (uNow.QuadPart - uLast.QuadPart) >= 18000000000ULL;
                    }
                }
                if (needLunar && !g_lunarOnlineLoading) {
                    g_lunarOnlineLoading = true;
                    GetSystemTimeAsFileTime(&g_lastLunarAttempt);
                    HANDLE hLunar = CreateThread(NULL, 0, LunarOnlineThreadProc, hwnd, 0, NULL);
                    if (hLunar) CloseHandle(hLunar);
                    else g_lunarOnlineLoading = false;
                }
            } else if (wParam == 2) {
                HANDLE hThread = CreateThread(NULL, 0, [](LPVOID) -> DWORD {
                    AppConfig::GetInstance().SyncNetworkTime();
                    return 0;
                }, NULL, 0, NULL);
                if (hThread) CloseHandle(hThread);
            } else if (wParam == 3) {
                static FILETIME lastWorkTime = {0,0};
                char workPath[MAX_PATH];
                GetModuleFileNameA(NULL, workPath, MAX_PATH);
                RemoveFileNameFromPath(workPath);
                strcat(workPath, "work.txt");
                HANDLE hFile = CreateFileA(workPath, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    FILETIME ft;
                    if (GetFileTime(hFile, NULL, NULL, &ft)) {
                        if (CompareFileTime(&ft, &lastWorkTime) != 0) {
                            lastWorkTime = ft;
                            InvalidateRect(hwnd, NULL, TRUE);
                        }
                    }
                    CloseHandle(hFile);
                }
            }
            break;

        case WM_ENDSESSION:
            if (wParam) {
                WriteLog("系统会话结束，保存配置");
                cfg.SaveConfig();
            }
            break;

        case WM_NCHITTEST:
        {
            LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);
            if (hit == HTCLIENT && cfg.borderless) {
                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                ScreenToClient(hwnd, &pt);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int border = 8;
                if (pt.y < border) {
                    if (pt.x < border) return HTTOPLEFT;
                    if (pt.x > rc.right - border) return HTTOPRIGHT;
                    return HTTOP;
                }
                if (pt.y > rc.bottom - border) {
                    if (pt.x < border) return HTBOTTOMLEFT;
                    if (pt.x > rc.right - border) return HTBOTTOMRIGHT;
                    return HTBOTTOM;
                }
                if (pt.x < border) return HTLEFT;
                if (pt.x > rc.right - border) return HTRIGHT;
                return HTCAPTION;
            }
            return hit;
        }

        case WM_DESTROY:
        {
            WriteLog("窗口关闭");
            StopNtpServer();
            StopHttpServer();
            cfg.SaveConfig();

            {
                HMENU hMenu = GetMenu(hwnd);
                if (hMenu) {
                    SetMenu(hwnd, NULL);
                    DestroyMenu(hMenu);
                }
            }

            KillTimer(hwnd, 1);
            KillTimer(hwnd, 2);
            KillTimer(hwnd, 3);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
