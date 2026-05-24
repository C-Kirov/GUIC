#include "dialog_message.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"

static INT_PTR CALLBACK MessageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            SetDlgItemTextA(hwnd, IDC_EDIT_MESSAGE, AppConfig::GetInstance().message.c_str());
            return TRUE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                char buffer[1024];
                GetDlgItemTextA(hwnd, IDC_EDIT_MESSAGE, buffer, sizeof(buffer));
                AppConfig::GetInstance().message = buffer;
                WriteLog("设置提示信息: " + AppConfig::GetInstance().message);
                EndDialog(hwnd, IDOK);
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
            }
            break;
    }
    return FALSE;
}

void ShowMessageDialog(HWND parent)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MESSAGE_DIALOG), parent, MessageDlgProc, 0);
}
