#include "app.h"
#include "platform.h"
#include "util.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <dwmapi.h>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const wchar_t* kMainWindowClass = L"DSDeathCounterMain";

namespace {
constexpr int kHotkeyId = 1;
constexpr UINT kTrayMenuOpen = 100, kTrayMenuOverlay = 101, kTrayMenuExit = 102;
}

// ---------------------------------------------------------------------------------------------
// Main loop

int App::Run(bool startInTray)
{
    store_.Load();
    Settings& s = store_.settings;
    // Keep the "start with Windows" entry pointing at this exe, even if it was moved.
    if (s.firstRunDone && s.startWithWindows) SetAutostart(true);

    if (!CreateMainWindow()) return 1;
    if (!CreateDeviceD3D()) {
        MessageBoxW(nullptr, L"Não foi possível iniciar o Direct3D 11.", L"DS Death Counter", MB_ICONERROR);
        return 1;
    }
    overlay_.Create(inst_);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(device_, context_);
    dpiScale_ = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd_);
    LoadFonts(dpiScale_);

    AddTray();
    RegisterOverlayHotkey();
    if (!startInTray || !s.firstRunDone) ShowMain();

    ULONGLONG lastTick = 0, lastSave = GetTickCount64();
    while (running_) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running_ = false;
        }
        if (!running_) break;

        ULONGLONG now = GetTickCount64();
        if (now - lastTick >= 100) {
            lastTick = now;
            Tick();
        }
        if (now - lastSave >= 30000) {
            lastSave = now;
            store_.SaveIfDirty();
        }

        if (!visible_ || IsIconic(hwnd_)) {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
            continue;
        }
        if (occluded_ && swapChain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLINPUT);
            continue;
        }
        occluded_ = false;
        RenderFrame();
        // Keep the UI cheap while the user is playing.
        if (GetForegroundWindow() != hwnd_) MsgWaitForMultipleObjects(0, nullptr, FALSE, 66, QS_ALLINPUT);
    }

    store_.Save();
    UnregisterHotKey(hwnd_, kHotkeyId);
    Shell_NotifyIconW(NIM_DELETE, &nid_);
    overlay_.Destroy();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd_);
    return 0;
}

void App::Tick()
{
    tracker_.Tick(reader_);
    Settings& s = store_.settings;
    if (reader_.IsReady() && s.gameExePath.empty() && reader_.GameId() == "dsr") {   // "Iniciar Dark Souls" button
        std::wstring path = reader_.ProcessPath();
        if (!path.empty()) {
            s.gameExePath = WideToUtf8(path);
            store_.MarkDirty();
        }
    }
    UpdateOverlay();
    UpdateTrayTip();
}

void App::UpdateOverlay()
{
    const Settings& s = store_.settings;
    ULONGLONG now = GetTickCount64();
    if (!reader_.IsReady()) {
        gameHwnd_ = nullptr;
    } else if (!gameHwnd_ || !IsWindow(gameHwnd_) || now - gameHwndCheck_ > 2000) {
        gameHwnd_ = FindMainWindow(reader_.pid());
        gameHwndCheck_ = now;
    }

    OverlayContent c;
    Profile* p = tracker_.active();
    bool haveRect = gameHwnd_ && !IsIconic(gameHwnd_) && GetClientScreenRect(gameHwnd_, c.anchorRect);
    int count = 0;
    bool edited = false;
    std::wstring name;

    if (overlayPreview_) {
        if (!haveRect) {
            MONITORINFO mi{ sizeof(mi) };
            GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi);
            c.anchorRect = mi.rcMonitor;
        }
        c.visible = true;
        count = p ? p->Shown() : 42;
        edited = previewAsEdited_ || (p && p->edited);
        name = p ? Utf8ToWide(p->name) : L"Chosen Undead";
    } else if (p && haveRect && s.overlayEnabled) {
        bool focused = !s.overlayOnlyWhenFocused || GetForegroundWindow() == gameHwnd_;
        c.visible = focused && (!s.overlayHideInMenu || tracker_.inGame());
        count = p->Shown();
        edited = p->edited;
        name = Utf8ToWide(p->name);
    }

    if (c.visible) {
        c.text = (s.overlayShowName ? name + L"\n" : L"") + Utf8ToWide(s.overlayLabel) + std::to_wstring(count);
        c.color = edited ? s.colorEdited : s.colorNormal;
    }
    overlay_.Update(c, s);
}

// ---------------------------------------------------------------------------------------------
// Window

bool App::CreateMainWindow()
{
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = inst_;
    wc.hIcon = LoadIconW(inst_, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(inst_, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), 0);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kMainWindowClass;
    RegisterClassExW(&wc);

    // Size for the monitor DPI the window is going to appear on.
    POINT pt{ 0, 0 };
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    float scale = ImGui_ImplWin32_GetDpiScaleForMonitor(mon);
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfoW(mon, &mi);
    int w = std::min((int)(1100 * scale), (int)(mi.rcWork.right - mi.rcWork.left));
    int h = std::min((int)(720 * scale), (int)(mi.rcWork.bottom - mi.rcWork.top));
    int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - w) / 2;
    int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;

    hwnd_ = CreateWindowExW(0, kMainWindowClass, L"DS Death Counter", WS_OVERLAPPEDWINDOW, x, y, w, h,
                            nullptr, nullptr, inst_, this);
    if (hwnd_) {
        BOOL dark = TRUE;   // dark title bar to match the UI (Windows 10 2004+ / 11)
        DwmSetWindowAttribute(hwnd_, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    }
    taskbarCreatedMsg_ = RegisterWindowMessageW(L"TaskbarCreated");
    return hwnd_ != nullptr;
}

void App::ShowMain()
{
    ShowWindow(hwnd_, IsIconic(hwnd_) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(hwnd_);
    visible_ = true;
}

void App::HideMain()
{
    ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
    overlayPreview_ = false;
    if (!trayHintShown_) {
        trayHintShown_ = true;
        NOTIFYICONDATAW n = nid_;
        n.uFlags = NIF_INFO;
        wcscpy_s(n.szInfoTitle, L"DS Death Counter");
        wcscpy_s(n.szInfo, L"Continuo contando suas mortes aqui na bandeja.");
        n.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconW(NIM_MODIFY, &n);
    }
}

void App::AddTray()
{
    nid_ = NOTIFYICONDATAW{};
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hwnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_APP_TRAY;
    nid_.hIcon = (HICON)LoadImageW(inst_, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), 0);
    wcscpy_s(nid_.szTip, L"DS Death Counter");
    Shell_NotifyIconW(NIM_ADD, &nid_);
    trayTip_.clear();
}

void App::UpdateTrayTip()
{
    std::wstring tip = L"DS Death Counter";
    if (Profile* p = tracker_.active())
        tip += L"\n" + Utf8ToWide(p->name) + L": " + std::to_wstring(p->Shown()) + L" mortes";
    else if (reader_.state() == GameReader::State::NotRunning)
        tip += L"\nAguardando o Dark Souls";
    if (tip == trayTip_) return;
    trayTip_ = tip;
    wcsncpy_s(nid_.szTip, tip.c_str(), _TRUNCATE);
    nid_.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &nid_);
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}

void App::ShowTrayMenu()
{
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kTrayMenuOpen, L"Abrir DS Death Counter");
    AppendMenuW(menu, MF_STRING | (store_.settings.overlayEnabled ? MF_CHECKED : 0), kTrayMenuOverlay,
                L"Mostrar overlay\tCtrl+Shift+D");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayMenuExit, L"Sair");
    SetMenuDefaultItem(menu, kTrayMenuOpen, FALSE);
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd_);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
    switch (cmd) {
        case kTrayMenuOpen: ShowMain(); break;
        case kTrayMenuOverlay:
            store_.settings.overlayEnabled = !store_.settings.overlayEnabled;
            store_.MarkDirty();
            break;
        case kTrayMenuExit: running_ = false; break;
    }
}

void App::RegisterOverlayHotkey()
{
    UnregisterHotKey(hwnd_, kHotkeyId);
    if (store_.settings.hotkeyEnabled)
        RegisterHotKey(hwnd_, kHotkeyId, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'D');
}

LRESULT CALLBACK App::StaticWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)lp;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
    }
    auto* app = (App*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    return app ? app->WndProc(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT App::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return 1;

    if (msg == taskbarCreatedMsg_ && taskbarCreatedMsg_) {   // Explorer restarted
        AddTray();
        return 0;
    }
    switch (msg) {
        case WM_SIZE:
            if (wp == SIZE_MINIMIZED) return 0;
            resizeW_ = LOWORD(lp);
            resizeH_ = HIWORD(lp);
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mmi = (MINMAXINFO*)lp;
            mmi->ptMinTrackSize = { (LONG)(760 * dpiScale_), (LONG)(480 * dpiScale_) };
            return 0;
        }
        case WM_DPICHANGED: {
            pendingDpi_ = HIWORD(wp) / 96.0f;
            const RECT* r = (RECT*)lp;
            SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_SYSCOMMAND:
            if ((wp & 0xfff0) == SC_KEYMENU) return 0;   // no ALT menu
            break;
        case WM_CLOSE:
            if (store_.settings.closeToTray) HideMain();
            else running_ = false;
            return 0;
        case WM_HOTKEY:
            if (wp == kHotkeyId) {
                store_.settings.overlayEnabled = !store_.settings.overlayEnabled;
                store_.MarkDirty();
            }
            return 0;
        case WM_APP_SHOWME:
            ShowMain();
            return 0;
        case WM_APP_TRAY:
            switch (LOWORD(lp)) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDBLCLK: ShowMain(); break;
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU: ShowTrayMenu(); break;
            }
            return 0;
        case WM_ENDSESSION:
            if (wp) store_.Save();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------------------------
// Direct3D 11

bool App::CreateDeviceD3D()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 60, 1 };
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd_;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                               D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &got, &context_);
    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
                                           D3D11_SDK_VERSION, &sd, &swapChain_, &device_, &got, &context_);
    if (FAILED(hr)) return false;
    CreateRenderTarget();
    return true;
}

void App::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (swapChain_) { swapChain_->Release(); swapChain_ = nullptr; }
    if (context_) { context_->Release(); context_ = nullptr; }
    if (device_) { device_->Release(); device_ = nullptr; }
}

void App::CreateRenderTarget()
{
    ID3D11Texture2D* back = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        device_->CreateRenderTargetView(back, nullptr, &rtv_);
        back->Release();
    }
}

void App::CleanupRenderTarget()
{
    if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
}

void App::RenderFrame()
{
    if (resizeW_ && resizeH_) {
        CleanupRenderTarget();
        swapChain_->ResizeBuffers(0, resizeW_, resizeH_, DXGI_FORMAT_UNKNOWN, 0);
        resizeW_ = resizeH_ = 0;
        CreateRenderTarget();
    }
    if (pendingDpi_ > 0.0f) {
        dpiScale_ = pendingDpi_;
        pendingDpi_ = 0.0f;
        ImGui_ImplDX11_InvalidateDeviceObjects();   // font texture is rebuilt on the next NewFrame
        LoadFonts(dpiScale_);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawUI();
    ImGui::Render();

    const float clear[4] = { 0.07f, 0.06f, 0.055f, 1.0f };
    context_->OMSetRenderTargets(1, &rtv_, nullptr);
    context_->ClearRenderTargetView(rtv_, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    HRESULT hr = swapChain_->Present(1, 0);
    occluded_ = (hr == DXGI_STATUS_OCCLUDED);
}

void App::LoadFonts(float scale)
{
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x2010, 0x2027, 0x2030, 0x205E, 0x2190, 0x2193, 0x25CF, 0x25CF, 0 };
    wchar_t winDir[MAX_PATH];
    GetWindowsDirectoryW(winDir, MAX_PATH);
    std::string fontsDir = WideToUtf8(winDir) + "\\Fonts\\";
    auto load = [&](const char* file, float px) -> ImFont* {
        std::string path = fontsDir + file;
        if (!FileExists(Utf8ToWide(path))) return nullptr;
        return io.Fonts->AddFontFromFileTTF(path.c_str(), std::round(px * scale), nullptr, ranges);
    };
    fontUi_ = load("segoeui.ttf", 17.0f);
    if (!fontUi_) fontUi_ = io.Fonts->AddFontDefault();
    fontBold_ = load("segoeuib.ttf", 17.0f);
    if (!fontBold_) fontBold_ = fontUi_;
    fontTitle_ = load("segoeuib.ttf", 24.0f);
    if (!fontTitle_) fontTitle_ = fontBold_;
    fontMid_ = load("georgiab.ttf", 30.0f);
    if (!fontMid_) fontMid_ = fontTitle_;
    fontBig_ = load("georgiab.ttf", 76.0f);
    if (!fontBig_) fontBig_ = load("segoeuib.ttf", 76.0f);
    if (!fontBig_) fontBig_ = fontTitle_;
    io.FontDefault = fontUi_;
    ApplyStyle(scale);
}

void App::ApplyStyle(float scale)
{
    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();
    ImGui::StyleColorsDark(&st);
    st.WindowRounding = 0.0f;
    st.ChildRounding = 6.0f;
    st.FrameRounding = 4.0f;
    st.PopupRounding = 6.0f;
    st.GrabRounding = 4.0f;
    st.TabRounding = 4.0f;
    st.ScrollbarRounding = 6.0f;
    st.WindowPadding = ImVec2(16, 14);
    st.FramePadding = ImVec2(9, 5);
    st.ItemSpacing = ImVec2(9, 8);
    st.CellPadding = ImVec2(8, 5);
    st.WindowBorderSize = 0.0f;
    st.ChildBorderSize = 1.0f;
    st.ScrollbarSize = 13.0f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_Text]                 = ImVec4(0.92f, 0.89f, 0.82f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.56f, 0.52f, 0.46f, 1.00f);
    c[ImGuiCol_WindowBg]             = ImVec4(0.07f, 0.06f, 0.055f, 1.00f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.09f, 0.08f, 1.00f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.11f, 0.10f, 0.09f, 0.98f);
    c[ImGuiCol_Border]               = ImVec4(0.27f, 0.22f, 0.17f, 0.65f);
    c[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.14f, 0.12f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.22f, 0.18f, 0.15f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.21f, 0.16f, 1.00f);
    c[ImGuiCol_TitleBg]              = ImVec4(0.10f, 0.09f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.12f, 0.09f, 1.00f);
    c[ImGuiCol_CheckMark]            = ImVec4(1.00f, 0.62f, 0.25f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.85f, 0.50f, 0.20f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(1.00f, 0.62f, 0.25f, 1.00f);
    c[ImGuiCol_Button]               = ImVec4(0.25f, 0.18f, 0.12f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.45f, 0.28f, 0.13f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.62f, 0.36f, 0.14f, 1.00f);
    c[ImGuiCol_Header]               = ImVec4(0.38f, 0.23f, 0.11f, 0.60f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.52f, 0.31f, 0.13f, 0.70f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.66f, 0.39f, 0.15f, 0.80f);
    c[ImGuiCol_Separator]            = ImVec4(0.30f, 0.24f, 0.18f, 1.00f);
    c[ImGuiCol_Tab]                  = ImVec4(0.15f, 0.12f, 0.10f, 1.00f);
    c[ImGuiCol_TabHovered]           = ImVec4(0.52f, 0.31f, 0.13f, 1.00f);
    c[ImGuiCol_TabSelected]          = ImVec4(0.38f, 0.23f, 0.11f, 1.00f);
    c[ImGuiCol_TabSelectedOverline]  = ImVec4(1.00f, 0.62f, 0.25f, 1.00f);
    c[ImGuiCol_TableHeaderBg]        = ImVec4(0.16f, 0.13f, 0.11f, 1.00f);
    c[ImGuiCol_TableBorderStrong]    = ImVec4(0.27f, 0.22f, 0.17f, 1.00f);
    c[ImGuiCol_TableBorderLight]     = ImVec4(0.20f, 0.17f, 0.14f, 1.00f);
    c[ImGuiCol_TableRowBgAlt]        = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    c[ImGuiCol_PlotHistogram]        = ImVec4(0.85f, 0.45f, 0.15f, 1.00f);
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.55f);
    c[ImGuiCol_NavCursor]            = ImVec4(1.00f, 0.62f, 0.25f, 1.00f);
    st.ScaleAllSizes(scale);
}
