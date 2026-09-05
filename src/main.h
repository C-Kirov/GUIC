#ifndef MAIN_H
#define MAIN_H

#include <windows.h>
#include "resource.h"
#include "i18n.h"

// WM_DPICHANGED 兼容定义（MinGW 旧版本可能没有）
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

// 自定义消息
#define WM_WEATHER_READY    (WM_APP + 1)   // 天气获取完成
#define WM_TRAY_ICON        (WM_APP + 2)   // 托盘图标消息（预留）
#define WM_LUNAR_READY      (WM_APP + 3)   // 在线农历获取完成

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // MAIN_H
