#include "dialog_settings.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"
#include <cstdio>

// 窗口设置
static INT_PTR CALLBACK WindowSettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            AppConfig& cfg = AppConfig::GetInstance();
            CheckDlgButton(hwnd, IDC_CHECK_RESIZABLE, cfg.resizable ? BST_CHECKED : BST_UNCHECKED);
            char buf[16];
            sprintf(buf, "%d", cfg.windowWidth);
            SetDlgItemText(hwnd, IDC_EDIT_WINDOW_WIDTH, buf);
            sprintf(buf, "%d", cfg.windowHeight);
            SetDlgItemText(hwnd, IDC_EDIT_WINDOW_HEIGHT, buf);
            return TRUE;
        }
        case WM_COMMAND:
        {
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
                    WriteLog("窗口设置更新: " + std::to_string(w) + "x" + std::to_string(h));

                    HWND parent = GetParent(hwnd);  // 注意：DialogBox 的父窗口是 owner
                    if (parent) {
                        RECT rect = {0, 0, w, h};
                        AdjustWindowRect(&rect, GetWindowLong(parent, GWL_STYLE), TRUE);
                        SetWindowPos(parent, NULL, 0, 0,
                                     rect.right - rect.left, rect.bottom - rect.top,
                                     SWP_NOMOVE | SWP_NOZORDER);
                    }
                    EndDialog(hwnd, IDOK);
                } else {
                    MessageBox(hwnd, "请输入有效的窗口大小！\n宽度: 300-2000, 高度: 200-1500", "错误", MB_OK | MB_ICONERROR);
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
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            AppConfig& cfg = AppConfig::GetInstance();
            char buf[16];
            sprintf(buf, "%d", cfg.fontSize);
            SetDlgItemText(hwnd, IDC_EDIT_FONT_SIZE, buf);
            SetDlgItemText(hwnd, IDC_EDIT_FONT_NAME, cfg.fontName.c_str());
            return TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK) {
                char sizeBuf[16], nameBuf[256];
                GetDlgItemTextA(hwnd, IDC_EDIT_FONT_SIZE, sizeBuf, sizeof(sizeBuf));
                GetDlgItemTextA(hwnd, IDC_EDIT_FONT_NAME, nameBuf, sizeof(nameBuf));
                int sz = atoi(sizeBuf);
                if (sz > 0 && sz <= 100) {
                    AppConfig::GetInstance().fontSize = sz;
                }
                if (strlen(nameBuf) > 0) {
                    AppConfig::GetInstance().fontName = nameBuf;
                }
                WriteLog("字体设置更新: 大小=" + std::to_string(AppConfig::GetInstance().fontSize) +
                         ", 字体=" + AppConfig::GetInstance().fontName);
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
    switch (msg)
    {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_BUTTON_COLOR) {
                CHOOSECOLOR cc = {0};
                static COLORREF custColors[16] = {0};
                cc.lStructSize  = sizeof(CHOOSECOLOR);
                cc.hwndOwner    = hwnd;
                cc.lpCustColors = custColors;
                cc.rgbResult    = AppConfig::GetInstance().fontColor;
                cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColor(&cc)) {
                    AppConfig::GetInstance().fontColor = cc.rgbResult;
                    WriteLog("设置字体颜色: RGB(" +
                             std::to_string(GetRValue(cc.rgbResult)) + "," +
                             std::to_string(GetGValue(cc.rgbResult)) + "," +
                             std::to_string(GetBValue(cc.rgbResult)) + ")");
                }
            } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, LOWORD(wParam));
            }
            break;
        }
    }
    return FALSE;
}

void ShowWindowSettingsDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WINDOW_SETTINGS_DIALOG), parent, WindowSettingsDlgProc, 0);
}

void ShowFontSettingsDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FONT_SETTINGS_DIALOG), parent, FontSettingsDlgProc, 0);
}

void ShowColorSettingsDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLOR_SETTINGS_DIALOG), parent, ColorSettingsDlgProc, 0);
}
