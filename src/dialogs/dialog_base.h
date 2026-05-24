#ifndef DIALOG_BASE_H
#define DIALOG_BASE_H

#include <windows.h>

class ModalDialogHelper {
public:
    static void Run(HWND parent,
                    LPCSTR className,
                    LPCSTR title,
                    WNDPROC dlgProc,
                    int width, int height,
                    LPARAM param = 0);

private:
    static LRESULT CALLBACK WrapperProc(HWND, UINT, WPARAM, LPARAM);
    static thread_local WNDPROC m_originalProc;
};

#endif
