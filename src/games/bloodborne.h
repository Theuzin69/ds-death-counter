#pragma once
// Bloodborne running in the "Bloodborne PC Windows port" (bbport.exe), read-only.
//
// The port loads the game's own PS4 x86-64 code (eboot.bin) as one flat image inside
// bbport.exe (eboot address 0 = image start, at 0x800000000 in v0.5) and runs it directly.
// So the game's globals sit at image base + their eboot address, and the same byte
// signatures work on the eboot file and on the live process. Like the Dark Souls
// Remastered reader, this only uses OpenProcess (read/query rights) and ReadProcessMemory.

#include "game_reader.h"
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

extern const GameInfo kBloodborneInfo;

namespace bb {

// Where the game keeps the values the counter reads. Found by AnalyzeImage.
struct Layout {
    uint64_t gameDataMan = 0;   // eboot address of the GameDataMan pointer
    int offDeaths = 0;          // GameDataMan: death counter (Lua AddDeathCount)
    int offTrueDeaths = 0;      // GameDataMan: second counter (Lua AddTrueDeathCount), diagnostics only
    int offPlayTime = 0;        // GameDataMan: play time in ms
    int offClearCount = -1;     // GameDataMan: times the game was finished (NG+ cycle), -1 = not found
    int offPlayerData = 0;      // GameDataMan -> PlayerGameData pointer
    int offSoulLevel = 0;       // PlayerGameData: level
    int offName = 0;            // PlayerGameData: character name (UTF-16)
};

// Scans a flat eboot image (eboot address 0 at index 0) and fills `out`.
// Appends one line per item to `log`. True when everything needed for counting was found.
bool AnalyzeImage(const std::vector<uint8_t>& img, Layout& out, std::string& log);

// Builds that flat image from a plaintext PS4 SELF (the eboot.bin file the port loads).
bool FlatImageFromSelf(const std::vector<uint8_t>& self, std::vector<uint8_t>& img, std::string& err);

class Reader {
public:
    ~Reader() { Detach(); }

    // Looks for the port's process and analyzes it. Returns true while it owns the reader
    // state (process attached, or access denied), and writes that state to `state`.
    bool Update(GameReader::State& state);
    bool Attached() const { return proc_ != nullptr; }
    bool Read(GameSnapshot& out);
    void Detach();

    DWORD pid() const { return pid_; }
    std::wstring ProcessPath() const;
    std::string VersionName() const;
    std::string DiagnosticsText(GameReader::State state) const;

private:
    bool FindImageBase();
    bool Scan();
    uintptr_t ReadPtr(uintptr_t addr) const;
    template <class T> bool ReadT(uintptr_t addr, T& v) const {
        SIZE_T n = 0;
        return addr && ReadProcessMemory(proc_, (LPCVOID)addr, &v, sizeof(T), &n) && n == sizeof(T);
    }

    HANDLE    proc_ = nullptr;
    DWORD     pid_ = 0;
    bool      denied_ = false;
    bool      ready_ = false;
    uintptr_t base_ = 0;
    size_t    size_ = 0;
    ULONGLONG nextTry_ = 0;
    int       scanAttempts_ = 0;
    Layout    layout_;
    std::string scanLog_;

    // Play time only runs while a character is in the game (not on the title screen).
    uint32_t  lastPlayTime_ = UINT32_MAX;
    ULONGLONG playTimeMovedAt_ = 0;
    int       lastTrueDeaths_ = -1;
};

} // namespace bb
