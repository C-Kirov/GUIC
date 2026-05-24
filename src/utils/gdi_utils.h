#ifndef GDI_UTILS_H
#define GDI_UTILS_H

#include <windows.h>

// RAII 包装：自动恢复原 GDI 对象并删除创建的 GDI 对象
class GDIObjectSelector {
public:
    GDIObjectSelector(HDC hdc, HGDIOBJ obj) : m_hdc(hdc), m_oldObj(SelectObject(hdc, obj)) {}
    ~GDIObjectSelector() { SelectObject(m_hdc, m_oldObj); }
private:
    HDC     m_hdc;
    HGDIOBJ m_oldObj;
};

// RAII 包装：自动删除 GDI 对象（画笔、画刷、字体等）
template<typename T>
class GDIResource {
public:
    GDIResource(T obj) : m_obj(obj) {}
    ~GDIResource() { if (m_obj) DeleteObject(m_obj); }
    operator T() const { return m_obj; }
private:
    T m_obj;
};

typedef GDIResource<HPEN>   ScopedPen;
typedef GDIResource<HBRUSH> ScopedBrush;
typedef GDIResource<HFONT>  ScopedFont;

#endif // GDI_UTILS_H
