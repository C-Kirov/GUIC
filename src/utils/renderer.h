#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>

class Renderer {
public:
    // 绘制主窗口内容，hwnd 用于获取客户区大小
    static void Draw(HWND hwnd, HDC hdc);
};

#endif // RENDERER_H
