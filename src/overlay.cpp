#include "overlay.h"
#include "util.h"
#include <algorithm>
#include <cmath>
namespace Gdiplus { using std::min; using std::max; }
#include <objidl.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace {
const wchar_t* kOverlayClass = L"DSDeathCounterOverlay";
}

bool Overlay::Create(HINSTANCE inst)
{
    Gdiplus::GdiplusStartupInput in;
    if (Gdiplus::GdiplusStartup(&gdiplusToken_, &in, nullptr) != Gdiplus::Ok) return false;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = inst;
    wc.lpszClassName = kOverlayClass;
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                            kOverlayClass, L"DS Death Counter Overlay", WS_POPUP,
                            0, 0, 1, 1, nullptr, nullptr, inst, nullptr);
    return hwnd_ != nullptr;
}

void Overlay::Destroy()
{
    if (hwnd_) DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    if (gdiplusToken_) Gdiplus::GdiplusShutdown(gdiplusToken_);
    gdiplusToken_ = 0;
}

void Overlay::Update(const OverlayContent& c, const Settings& s)
{
    if (!hwnd_) return;
    if (!c.visible || c.text.empty()) {
        if (shown_) ShowWindow(hwnd_, SW_HIDE);
        shown_ = false;
        return;
    }

    // Sizes are authored for a 1080p game window and scaled to the real one.
    float scale = 1.0f;
    int rectH = c.anchorRect.bottom - c.anchorRect.top;
    if (s.overlayScaleWithGame && rectH > 200) scale = rectH / 1080.0f;
    float fontPx = std::clamp(s.overlayFontSize * scale, 8.0f, 400.0f);

    wchar_t keyExtra[160];
    swprintf(keyExtra, std::size(keyExtra), L"|%.1f|%08X|%08X|%.1f|%d|", fontPx, c.color, s.colorOutline,
             s.overlayOutline, (int)s.overlayBold);
    std::wstring key = c.text + keyExtra + Utf8ToWide(s.overlayFont);
    if (key != drawnKey_) {
        Redraw(c, s, fontPx);
        drawnKey_ = key;
    }

    int offX = (int)std::lround(s.overlayOffsetX * scale);
    int offY = (int)std::lround(s.overlayOffsetY * scale);
    bool right = s.overlayAnchor == 1 || s.overlayAnchor == 3;
    bool bottom = s.overlayAnchor == 2 || s.overlayAnchor == 3;
    int x = right ? c.anchorRect.right - offX - size_.cx : c.anchorRect.left + offX;
    int y = bottom ? c.anchorRect.bottom - offY - size_.cy : c.anchorRect.top + offY;

    RECT cur{};
    GetWindowRect(hwnd_, &cur);
    ULONGLONG now = GetTickCount64();
    if (!shown_ || cur.left != x || cur.top != y || now - lastTopmost_ > 1000) {
        // Re-assert topmost regularly: the game window tends to get pushed above us.
        SetWindowPos(hwnd_, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        lastTopmost_ = now;
    }
    shown_ = true;
}

void Overlay::Redraw(const OverlayContent& c, const Settings& s, float fontPx)
{
    using namespace Gdiplus;

    std::wstring fontName = Utf8ToWide(s.overlayFont);
    FontFamily family(fontName.c_str());
    FontFamily fallback(L"Georgia");
    const FontFamily* fam = &family;
    if (family.GetLastStatus() != Ok) fam = fallback.GetLastStatus() == Ok ? &fallback : FontFamily::GenericSerif();
    INT style = s.overlayBold ? FontStyleBold : FontStyleRegular;
    if (!fam->IsStyleAvailable(style)) style = FontStyleRegular;

    GraphicsPath path;
    StringFormat fmt(StringFormat::GenericTypographic());
    path.AddString(c.text.c_str(), -1, fam, style, fontPx, PointF(0, 0), &fmt);
    RectF bounds;
    path.GetBounds(&bounds);

    float outline = std::max(0.0f, s.overlayOutline * fontPx / 36.0f);
    int pad = (int)std::ceil(outline) + 4;
    int w = std::max(1, (int)std::ceil(bounds.Width) + pad * 2);
    int h = std::max(1, (int)std::ceil(bounds.Height) + pad * 2);
    Matrix m;
    m.Translate(pad - bounds.X, pad - bounds.Y);
    path.Transform(&m);

    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;   // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ old = SelectObject(mem, dib);
    {
        // Draw straight into the DIB with premultiplied alpha, as UpdateLayeredWindow expects.
        Bitmap bmp(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)bits);
        Graphics g(&bmp);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetCompositingQuality(CompositingQualityHighQuality);
        g.Clear(Color(0, 0, 0, 0));
        if (outline > 0.0f) {
            Pen pen(Color((ARGB)s.colorOutline), outline * 2.0f);
            pen.SetLineJoin(LineJoinRound);
            g.DrawPath(&pen, &path);
        }
        SolidBrush brush(Color((ARGB)c.color));
        g.FillPath(&brush, &path);
    }
    POINT src{ 0, 0 };
    SIZE sz{ w, h };
    BLENDFUNCTION bf{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    UpdateLayeredWindow(hwnd_, screen, nullptr, &sz, mem, &src, 0, &bf, ULW_ALPHA);
    size_ = sz;

    SelectObject(mem, old);
    DeleteObject(dib);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);
}
