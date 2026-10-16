#include "platform.h"
#include "util.h"
#include <commdlg.h>
#include <shellapi.h>
#include <regex>
#include <vector>

namespace {
const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* kRunValue = L"DSDeathCounter";
}

bool IsAutostartEnabled()
{
    wchar_t buf[MAX_PATH * 2];
    DWORD size = sizeof(buf);
    return RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, buf, &size) == ERROR_SUCCESS;
}

void SetAutostart(bool enabled)
{
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) return;
    if (enabled) {
        std::wstring cmd = L"\"" + GetExePath() + L"\" --tray";
        RegSetValueExW(key, kRunValue, 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(key, kRunValue);
    }
    RegCloseKey(key);
}

std::wstring FindSteamGameExe()
{
    wchar_t steam[MAX_PATH];
    DWORD size = sizeof(steam);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, nullptr, steam, &size) != ERROR_SUCCESS)
        return {};
    std::wstring steamPath = steam;
    for (auto& ch : steamPath) if (ch == L'/') ch = L'\\';

    std::vector<std::wstring> libraries{ steamPath };
    std::string vdf;
    if (ReadWholeFile(steamPath + L"\\steamapps\\libraryfolders.vdf", vdf)) {
        std::regex re("\"path\"\\s+\"([^\"]+)\"");
        for (std::sregex_iterator it(vdf.begin(), vdf.end(), re), end; it != end; ++it) {
            std::string p = (*it)[1].str();
            std::string unescaped;
            for (size_t i = 0; i < p.size(); ++i) {
                if (p[i] == '\\' && i + 1 < p.size() && p[i + 1] == '\\') ++i;
                unescaped += p[i];
            }
            libraries.push_back(Utf8ToWide(unescaped));
        }
    }
    for (const auto& lib : libraries) {
        std::wstring exe = lib + L"\\steamapps\\common\\DARK SOULS REMASTERED\\DarkSoulsRemastered.exe";
        if (FileExists(exe)) return exe;
    }
    return {};
}

bool LaunchGame(const std::wstring& exePath)
{
    std::wstring lower = exePath;
    CharLowerBuffW(lower.data(), (DWORD)lower.size());
    bool steamInstall = exePath.empty() || lower.find(L"\\steamapps\\common\\") != std::wstring::npos;
    if (steamInstall || !FileExists(exePath)) {
        return (INT_PTR)ShellExecuteW(nullptr, L"open", L"steam://rungameid/570940", nullptr, nullptr, SW_SHOWNORMAL) > 32;
    }
    std::wstring dir = exePath.substr(0, exePath.find_last_of(L'\\'));
    return (INT_PTR)ShellExecuteW(nullptr, L"open", exePath.c_str(), nullptr, dir.c_str(), SW_SHOWNORMAL) > 32;
}

HWND FindMainWindow(DWORD pid)
{
    struct Ctx { DWORD pid; HWND best; LONG bestArea; } ctx{ pid, nullptr, 0 };
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* c = (Ctx*)lp;
        DWORD wpid = 0;
        GetWindowThreadProcessId(h, &wpid);
        if (wpid != c->pid || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
        RECT r;
        GetClientRect(h, &r);
        LONG area = (r.right - r.left) * (r.bottom - r.top);
        if (area > c->bestArea) { c->best = h; c->bestArea = area; }
        return TRUE;
    }, (LPARAM)&ctx);
    return ctx.best;
}

bool GetClientScreenRect(HWND hwnd, RECT& out)
{
    RECT r;
    if (!hwnd || !GetClientRect(hwnd, &r)) return false;
    POINT tl{ 0, 0 };
    ClientToScreen(hwnd, &tl);
    out = { tl.x, tl.y, tl.x + r.right, tl.y + r.bottom };
    return r.right > 0 && r.bottom > 0;
}

bool MakeWindowBorderless(HWND hwnd)
{
    if (!hwnd) return false;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    style &= ~(LONG_PTR)(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    ex &= ~(LONG_PTR)(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
    const RECT& m = mi.rcMonitor;
    return SetWindowPos(hwnd, HWND_TOP, m.left, m.top, m.right - m.left, m.bottom - m.top,
                        SWP_FRAMECHANGED | SWP_NOOWNERZORDER) != 0;
}

std::wstring BrowseForExe(HWND owner, const std::wstring& initial)
{
    wchar_t file[MAX_PATH * 2] = L"DarkSoulsRemastered.exe";
    std::wstring dir;
    if (!initial.empty()) dir = initial.substr(0, initial.find_last_of(L'\\'));
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Dark Souls Remastered\0DarkSoulsRemastered.exe\0Executáveis (*.exe)\0*.exe\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = (DWORD)std::size(file);
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrTitle = L"Selecione DarkSoulsRemastered.exe";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&ofn) ? std::wstring(file) : std::wstring();
}

std::wstring BrowseSaveCsv(HWND owner, const std::wstring& suggestedName)
{
    wchar_t file[MAX_PATH * 2] = {};
    wcsncpy_s(file, suggestedName.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"CSV (*.csv)\0*.csv\0";
    ofn.lpstrDefExt = L"csv";
    ofn.lpstrFile = file;
    ofn.nMaxFile = (DWORD)std::size(file);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetSaveFileNameW(&ofn) ? std::wstring(file) : std::wstring();
}

void OpenFolder(const std::wstring& path)
{
    ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
