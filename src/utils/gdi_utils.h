#ifndef GDI_UTILS_H
#define GDI_UTILS_H

#include <windows.h>

// ================================================================
// RAII 封装：自动恢复原 GDI 对象并删除临时 GDI 对象
// ================================================================

// --- 选择器：构造时 SelectObject，析构时恢复 ---
class GDIObjectSelector {
public:
    GDIObjectSelector(HDC hdc, HGDIOBJ obj)
        : m_hdc(hdc), m_oldObj(SelectObject(hdc, obj)) {}
    ~GDIObjectSelector() { SelectObject(m_hdc, m_oldObj); }
private:
    HDC     m_hdc;
    HGDIOBJ m_oldObj;
};

// --- 资源持有者：构造时持有，析构时 DeleteObject ---
template<typename T>
class GDIResource {
public:
    explicit GDIResource(T obj) : m_obj(obj) {}
    ~GDIResource() { if (m_obj) DeleteObject(m_obj); }

    GDIResource(const GDIResource&) = delete;
    GDIResource& operator=(const GDIResource&) = delete;

    operator T() const { return m_obj; }
    T get() const { return m_obj; }

private:
    T m_obj;
};

// 常用 RAII 类型别名
typedef GDIResource<HPEN>    ScopedPen;
typedef GDIResource<HBRUSH>  ScopedBrush;
typedef GDIResource<HFONT>   ScopedFont;
typedef GDIResource<HBITMAP> ScopedBitmap;

// ================================================================
// DC 需要特殊处理：用 DeleteDC 而非 DeleteObject
// ================================================================
class ScopedDC {
public:
    explicit ScopedDC(HDC hdc) : m_hdc(hdc) {}
    ~ScopedDC() { if (m_hdc) DeleteDC(m_hdc); }

    ScopedDC(const ScopedDC&) = delete;
    ScopedDC& operator=(const ScopedDC&) = delete;

    operator HDC() const { return m_hdc; }
    HDC get() const { return m_hdc; }

private:
    HDC m_hdc;
};

#endif // GDI_UTILS_H
