#pragma once
// Read-only access to DarkSoulsRemastered.exe memory.
//
// Only ReadProcessMemory is used: nothing is written to the game, nothing is
// injected and no hooks are installed. The game cannot tell the difference
// between this and any other program that happens to be running.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

struct Vec3 { float x = 0, y = 0, z = 0; };

struct GameSnapshot {
    bool     gameDataOk   = false;  // GameDataMan resolved (game finished booting)
    bool     charLoaded   = false;  // a character is loaded (name valid, play time > 0)
    bool     inWorld      = false;  // player entity exists (not main menu / loading screen)
    int      deaths       = 0;      // the game's own per-character death counter
    uint32_t playTimeMs   = 0;
    int      ngPlus       = 0;
    int      saveSlot     = -1;
    int      soulLevel    = 0;
    std::wstring name;
    uint32_t mapRaw       = 0;
    int      mapArea      = -1;     // mAA_BB_00_00
    int      mapBlock     = -1;
    int      lastBonfire  = 0;
    int      hp           = -1;
    bool     hasPos       = false;
    Vec3     pos;
    // Boss health bars on screen: the NPC name id each bar shows, -1 when the slot is empty.
    // The game fills these when a boss fight starts and clears them when the boss dies or
    // the area reloads, so "a bar was up when the player died" means "died in that fight".
    static constexpr int kBossSlots = 2;
    int      bossNameId[kBossSlots] = { -1, -1 };
};

class GameReader {
public:
    enum class State { NotRunning, AccessDenied, Scanning, Ready };

    ~GameReader() { Detach(); }

    // Looks for the game process and resolves pointers. Cheap to call repeatedly.
    void Update();
    bool Read(GameSnapshot& out);
    void Detach();

    State state() const { return state_; }
    DWORD pid() const { return pid_; }
    bool  IsReady() const { return state_ == State::Ready; }
    std::wstring ProcessPath() const;
    std::string VersionName() const;
    std::string DiagnosticsText() const;

    // Resolves globals and offsets from a copy of the exe image (base/size must be set).
    // Public so it can be exercised against the exe file on disk.
    bool AnalyzeImage(const std::vector<uint8_t>& img);
    void SetModuleForAnalysis(uintptr_t base, uint32_t size) { base_ = base; size_ = size; }
    uintptr_t GameManAddr() const { return gameMan_; }
    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    uintptr_t MenuManAddr() const { return menuMan_; }
    int BossGaugeOffset() const { return offBossGauge_; }
    int BossGaugeStride() const { return bossGaugeStride_; }
    const std::string& ScanLog() const { return scanLog_; }

private:
    bool OpenGame(DWORD pid);
    bool ScanPatterns();
    bool ProcessAlive() const;

    uintptr_t ReadPtr(uintptr_t addr) const;
    template <class T> bool ReadT(uintptr_t addr, T& v) const {
        SIZE_T n = 0;
        return addr && ReadProcessMemory(proc_, (LPCVOID)addr, &v, sizeof(T), &n) && n == sizeof(T);
    }

    State     state_ = State::NotRunning;
    HANDLE    proc_ = nullptr;
    DWORD     pid_ = 0;
    uintptr_t base_ = 0;
    uint32_t  size_ = 0;
    ULONGLONG nextTry_ = 0;
    int       scanAttempts_ = 0;

    // Addresses of the game's global pointers (static variables in the exe)
    uintptr_t gameMan_ = 0, gameDataMan_ = 0, worldChrMan_ = 0;
    uintptr_t menuMan_ = 0;     // optional: only used for the boss health bars

    // Version dependent offsets
    bool newLayout_ = true;     // 1.03+ (current Steam build) vs 1.01.x
    int  offSaveSlot_ = 0xAA0;
    int  offLastBonfire_ = 0xB34;
    int  offStableMap_ = 0xBD0;
    int  offChrMapData_ = 0x68;
    int  offHp_ = 0x3E8;
    int  offBossGauge_ = 0;     // MenuMan offset of boss bar slot 0 (0 = not found)
    int  bossGaugeStride_ = 0x14;
    std::string scanLog_;
};
