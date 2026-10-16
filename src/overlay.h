#pragma once
// Click-through, always-on-top layered window drawn over the game window.
// It is a separate window owned by this app: nothing is drawn inside the game.

#include <windows.h>
#include <string>
#include "data_store.h"

struct OverlayContent {
    bool visible = false;
    RECT anchorRect{};          // screen rect to place the text in (game client area)
    std::wstring text;
    uint32_t color = 0;         // ARGB
};

class Overlay {
public:
    bool Create(HINSTANCE inst);
    void Destroy();
    void Update(const OverlayContent& c, const Settings& s);

private:
    void Redraw(const OverlayContent& c, const Settings& s, float fontPx);

    HWND hwnd_ = nullptr;
    ULONG_PTR gdiplusToken_ = 0;
    bool shown_ = false;
    ULONGLONG lastTopmost_ = 0;

    // cache of what is currently drawn
    std::wstring drawnKey_;
    SIZE size_{};
};
