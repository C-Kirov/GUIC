#include <windows.h>
#include "main.h"
#include "config/config.h"
#include "utils/logger.h"
#include "utils/network.h"
#include "i18n.h"
#include "resource.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    AppConfig& cfg = AppConfig::GetInstance();
    cfg.InitPaths();
    cfg.LoadConfig();

    //   初始化国际化
    I18nInit(cfg.language == AppLanguage::EN_US ? Language::EN_US : Language::ZH_CN);

    cfg.InitSpecialDays();

    // 进程级 Winsock 初始化（修复：服务器线程 socket() 报 WSANOTINITIALISED）
    WinsockInit();

    const char* CLASS_NAME = "GUIClockWindowClass";

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon         = hIcon;
    wc.hIconSm       = hIcon;

    RegisterClassEx(&wc);

    RECT rect = { 0, 0, cfg.windowWidth, cfg.windowHeight };
    //   无边框模式需要不同的窗口样式
    DWORD style = cfg.borderless
        ? (WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX)
        : WS_OVERLAPPEDWINDOW;
    BOOL hasMenu = cfg.borderless ? FALSE : TRUE;

    AdjustWindowRect(&rect, style, hasMenu);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        _S(STR_WINDOW_TITLE),    //   i18n
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    //   无边框窗口默认居中
    if (cfg.borderless) {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(hwnd, NULL,
            (screenW - (rect.right - rect.left)) / 2,
            (screenH - (rect.bottom - rect.top)) / 2,
            0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 进程退出前清理 Winsock（引用计数归零才真正 WSACleanup）
    WinsockShutdown();

    return 0;
}
