#include "dialog_message.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../main.h"
#include "../i18n.h"

static INT_PTR CALLBACK MessageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            SetWindowTextA(hwnd, _S(STR_SET_MESSAGE));
            SetDlgItemTextA(hwnd, IDC_STATIC_MSG_INPUT, _S(STR_MSG_INPUT_LABEL));
            SetDlgItemTextA(hwnd, IDOK, _S(STR_OK));
            SetDlgItemTextA(hwnd, IDCANCEL, _S(STR_CANCEL));
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
