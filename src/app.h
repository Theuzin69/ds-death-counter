#pragma once
#include <windows.h>
#include <d3d11.h>
#include <shellapi.h>
#include <string>
#include "data_store.h"
#include "death_tracker.h"
#include "game_reader.h"
#include "overlay.h"

struct ImFont;

#define WM_APP_TRAY   (WM_APP + 1)
#define WM_APP_SHOWME (WM_APP + 2)

extern const wchar_t* kMainWindowClass;

class App {
public:
    explicit App(HINSTANCE inst) : inst_(inst), tracker_(store_) {}
    int Run(bool startInTray);

private:
    static LRESULT CALLBACK StaticWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    // window / rendering (app.cpp)
    bool CreateMainWindow();
    bool CreateDeviceD3D();
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void LoadFonts(float dpiScale);
    void ApplyStyle(float dpiScale);
    void RenderFrame();
    void ShowMain();
    void HideMain();
    void AddTray();
    void UpdateTrayTip();
    void ShowTrayMenu();
    void RegisterOverlayHotkey();
    void Tick();
    void UpdateOverlay();

    // UI (app_ui.cpp)
    void DrawUI();
    void UiHeader();
    void UiProfilesTab();
    void UiProfileDetail(Profile& p);
    void UiDeathTable(Profile& p);
    void UiStatsTable(Profile& p, bool byCause);
    void UiSettingsTab();
    void UiDiagnosticsTab();
    void UiAboutTab();
    void UiWelcome();
    void UiPopups();
    void ExportCsv(const Profile& p);

    HINSTANCE inst_;
    HWND hwnd_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
    bool occluded_ = false;
    UINT resizeW_ = 0, resizeH_ = 0;
    float dpiScale_ = 1.0f;
    float pendingDpi_ = 0.0f;

    bool running_ = true;
    bool visible_ = false;
    bool trayHintShown_ = false;
    NOTIFYICONDATAW nid_{};
    UINT taskbarCreatedMsg_ = 0;
    std::wstring trayTip_;

    DataStore store_;
    GameReader reader_;
    DeathTracker tracker_;
    Overlay overlay_;
    HWND gameHwnd_ = nullptr;
    ULONGLONG gameHwndCheck_ = 0;

    // UI state
    ImFont* fontUi_ = nullptr;
    ImFont* fontBold_ = nullptr;
    ImFont* fontTitle_ = nullptr;
    ImFont* fontMid_ = nullptr;
    ImFont* fontBig_ = nullptr;
    std::string selectedKey_;
    std::string lastActiveKey_;
    std::string deathFilter_;
    bool overlayPreview_ = false;
    bool previewAsEdited_ = false;

    // popups
    bool openEditCount_ = false;
    int editCountValue_ = 0;
    bool openDeleteProfile_ = false;
    bool openEditRecord_ = false;
    bool editingArea_ = false;      // false = editing the cause
    int64_t editRecordId_ = 0;
    std::string editBuffer_;
    std::string statusMessage_;
    ULONGLONG statusMessageUntil_ = 0;
};
