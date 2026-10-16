#pragma once
#include <windows.h>
#include <string>

// Start with Windows (HKCU Run key, no admin rights needed).
bool IsAutostartEnabled();
void SetAutostart(bool enabled);

// Looks for DarkSoulsRemastered.exe in every Steam library. Empty if not found.
std::wstring FindSteamGameExe();
// Starts the game (through Steam when it is a Steam install).
bool LaunchGame(const std::wstring& exePath);

// Largest visible top-level window of the process, or nullptr.
HWND FindMainWindow(DWORD pid);
// Client area of a window in screen coordinates.
bool GetClientScreenRect(HWND hwnd, RECT& out);
// Removes the title bar/borders of the game window and makes it cover its monitor.
bool MakeWindowBorderless(HWND hwnd);

std::wstring BrowseForExe(HWND owner, const std::wstring& initial);
std::wstring BrowseSaveCsv(HWND owner, const std::wstring& suggestedName);
void OpenFolder(const std::wstring& path);
