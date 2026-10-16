#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

std::string  WideToUtf8(const std::wstring& w);
std::wstring Utf8ToWide(const std::string& s);

// %APPDATA%\DSDeathCounter (created if missing)
std::wstring GetAppDataDir();
std::wstring GetExePath();

// Local time "2026-10-16 14:03:22"
std::string NowLocalString();
// "12:34:56" from milliseconds (hours may exceed 24)
std::string FormatPlayTime(uint32_t ms);

bool WriteFileAtomic(const std::wstring& path, const std::string& content);
bool ReadWholeFile(const std::wstring& path, std::string& out);

bool FileExists(const std::wstring& path);
