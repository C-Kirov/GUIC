#include "dialog_base.h"
#include <cstring>

thread_local WNDPROC ModalDialogHelper::m_originalProc = nullptr;

void ModalDialogHelper::Run(HWND parent,
                            LPCSTR className,
                            LPCSTR title,
                            WNDPROC dlgProc,
                            int width, int height,
                            LPARAM param)
{
    // 注册窗口类（仅第一次）
    static bool s_registered[256] = {false};
    int idx = (int)((size_t)className & 0xFF);
    if (!s_registered[idx]) {
        WNDCLASSEXA wc = {0};
        wc.cbSize        = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc   = WrapperProc;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.lpszClassName = className;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        if (!RegisterClassExA(&wc))
            return;
        s_registered[idx] = true;
    }

    m_originalProc = dlgProc;

    HWND hDlg = CreateWindowExA(0, className, title,
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                                CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                parent, NULL, GetModuleHandle(NULL), (LPVOID)param);
    if (!hDlg) {
        m_originalProc = nullptr;
        return;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(hDlg, SW_SHOW);

    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    m_originalProc = nullptr;
}

LRESULT CALLBACK ModalDialogHelper::WrapperProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (m_originalProc) {
        return m_originalProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}
