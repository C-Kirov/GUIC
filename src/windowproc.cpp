#include "main.h"
#include "config/config.h"
#include "utils/logger.h"
#include "utils/renderer.h"
#include "dialogs/dialog_message.h"
#include "dialogs/dialog_countdown.h"
#include "dialogs/dialog_settings.h"
#include "dialogs/dialog_special.h"
#include "resource.h"

static void CreateMainMenu(HWND hwnd)
{
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SET_MESSAGE, "设置提示信息");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SET_COUNTDOWN_ONCE, "设置一次性倒计时");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SET_COUNTDOWN_DAILY, "设置每日倒计时");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_TOGGLE_MESSAGE, "切换提示显示");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_TOGGLE_CLOCK, "切换时钟显示");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_TOGGLE_COUNTDOWN, "切换倒计时显示");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_TOGGLE_LAYOUT, "切换自动布局");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SETTINGS_WINDOW, "窗口设置");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SETTINGS_FONT, "字体设置");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_SETTINGS_COLOR, "颜色设置");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_ADD_SPECIAL_DAY, "添加特殊日子");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_PREVIEW_MESSAGE, "预览提示信息");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_WORLD_CLOCK, "世界时钟");
    AppendMenu(hSubMenu, MF_STRING, ID_MENU_ANALOG_CLOCK, "模拟时钟");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "设置");
    SetMenu(hwnd, hMenu);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    AppConfig& cfg = AppConfig::GetInstance();

    switch (uMsg)
    {
        case WM_CREATE:
        {
            WriteLog("程序启动");
            // 配置已在 main.cpp 中加载，此处不再重复
            WriteSystemInfoIfNeeded();
            CreateMainMenu(hwnd);

            SetTimer(hwnd, 1, 100, NULL);

            // 首次网络同步放在后台线程，避免阻塞启动
            HANDLE hFirstSync = CreateThread(NULL, 0, [](LPVOID)->DWORD {
                AppConfig::GetInstance().SyncNetworkTime();
                return 0;
            }, NULL, 0, NULL);
            if (hFirstSync) CloseHandle(hFirstSync);

            // 每5分钟触发一次后台同步
            SetTimer(hwnd, 2, 300000, NULL);

            HINSTANCE hInst = GetModuleHandle(NULL);
            HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
            if (hIcon) {
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
            break;
        }

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
                    ShowMessageDialog(hwnd);
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_SET_COUNTDOWN_ONCE:
                    ShowCountdownOnceDialog(hwnd);
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_SET_COUNTDOWN_DAILY:
                    ShowCountdownDailyDialog(hwnd);
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_TOGGLE_CLOCK:
                    cfg.showClock = !cfg.showClock;
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_TOGGLE_COUNTDOWN:
                    cfg.showCountdown = !cfg.showCountdown;
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_TOGGLE_LAYOUT:
                    cfg.autoLayout = !cfg.autoLayout;
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_TOGGLE_MESSAGE:
                    cfg.showMessage = !cfg.showMessage;
                    WriteLog(std::string("切换提示显示: ") + (cfg.showMessage ? "开启" : "关闭"));
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_SETTINGS_WINDOW:
                    ShowWindowSettingsDialog(hwnd);
                    cfg.SaveConfig();
                    SetWindowPos(hwnd, NULL, 0, 0, cfg.windowWidth, cfg.windowHeight, SWP_NOMOVE | SWP_NOZORDER);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_SETTINGS_FONT:
                    ShowFontSettingsDialog(hwnd);
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_SETTINGS_COLOR:
                    ShowColorSettingsDialog(hwnd);
                    cfg.SaveConfig();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_ADD_SPECIAL_DAY:
                    ShowAddSpecialDayDialog(hwnd);
                    cfg.SaveSpecialDaysToFile();
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_PREVIEW_MESSAGE:
                    ShowPreviewMessageDialog(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_WORLD_CLOCK:
                    ShowWorldClockDialog(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                case ID_MENU_ANALOG_CLOCK:
                    ShowAnalogClockDialog(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
            }
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HGDIOBJ oldObj = SelectObject(memDC, hBitmap);

            Renderer::Draw(hwnd, memDC);

            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldObj);
            DeleteObject(hBitmap);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_TIMER:
            if (wParam == 1) {
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (wParam == 2) {
                // 异步网络同步，界面不冻结
                HANDLE hThread = CreateThread(NULL, 0, [](LPVOID) -> DWORD {
                    AppConfig::GetInstance().SyncNetworkTime();
                    return 0;
                }, NULL, 0, NULL);
                if (hThread) CloseHandle(hThread);
            }
            break;

        case WM_DESTROY:
            WriteLog("程序关闭");
            cfg.SaveConfig();
            KillTimer(hwnd, 1);
            KillTimer(hwnd, 2);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
