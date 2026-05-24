#include <windows.h>
#include "main.h"
#include "config/config.h"
#include "utils/logger.h"
#include "resource.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 提前加载配置，以便用正确的窗口大小创建窗口
    AppConfig& cfg = AppConfig::GetInstance();
    cfg.InitPaths();           // 设置配置文件路径
    cfg.LoadConfig();          // 读取config.ini
    cfg.InitSpecialDays();     // 加载特殊日子

    const char* CLASS_NAME = "GUIClockWindowClass";

    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon = hIcon;
    wc.hIconSm = hIcon;

    RegisterClassEx(&wc);

    // 使用保存的窗口尺寸
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "图形化时钟",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, cfg.windowWidth, cfg.windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
