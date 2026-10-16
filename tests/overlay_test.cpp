// Shows the overlay over the primary monitor for a few seconds (normal color, then "edited" color).
#include "overlay.h"
#include <objbase.h>

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    Overlay overlay;
    if (!overlay.Create(inst)) return 1;
    Settings s;
    OverlayContent c;
    c.visible = true;
    c.anchorRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    c.text = L"Mortes: 123";
    for (int i = 0; i < 60; ++i) {
        c.color = i < 30 ? s.colorNormal : s.colorEdited;
        overlay.Update(c, s);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);
        Sleep(100);
    }
    overlay.Destroy();
    return 0;
}
