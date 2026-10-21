#pragma once
// Read-only access to DarkSoulsRemastered.exe memory.
//
// Only ReadProcessMemory is used: nothing is written to the game, nothing is
// injected and no hooks are installed. The game cannot tell the difference
// between this and any other program that happens to be running.
//
// Other games live in src/games/ and are dispatched from here by process name.
// Elden Ring: while Easy Anti-Cheat runs, the game process is never opened at all
// and the count comes from the save file (see games/eldenring.h).

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games/eldenring.h"

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
    std::string game = "dsr";       // game id: "dsr", "er"
    std::string areaName;           // area name filled by game modules that name areas themselves
};

// Game ids used in profiles ("dsr", "er") and their display names.
const char* GameDisplayName(const std::string& gameId);

// Byte pattern search ("48 8B 05 ? ? ? ?"), shared by the game modules. Every match offset.
std::vector<size_t> FindPattern(const std::vector<uint8_t>& buf, const char* pattern);

class GameReader {
public:
    // SaveFile: Elden Ring protected by Easy Anti-Cheat; counting from the save file, no access to the game.
    enum class State { NotRunning, AccessDenied, Scanning, Ready, SaveFile };
    enum class Game { None, DarkSoulsRemastered, EldenRing };

    ~GameReader() { Detach(); }

    // Looks for the game process and resolves pointers. Cheap to call repeatedly.
    void Update();
    bool Read(GameSnapshot& out);
    void Detach();

    State state() const { return state_; }
    DWORD pid() const { return pid_; }
    bool  IsReady() const { return state_ == State::Ready || state_ == State::SaveFile; }
    Game  game() const { return game_; }
    std::string GameName() const;
    const std::string& EacReason() const { return eacReason_; }
    bool  EacActive() const { return eacSeen_; }   // Elden Ring protected by EAC in this run of the game
    // Elden Ring found, the game not opened yet: still making sure Easy Anti-Cheat is not running.
    bool CheckingEac() const { return game_ == Game::EldenRing && state_ == State::Scanning && !proc_; }
    std::wstring ProcessPath() const;
    std::string VersionName() const;
    std::string DiagnosticsText() const;

    // Resolves globals and offsets from a copy of the exe image (base/size must be set).
    // Public so it can be exercised against the exe file on disk.
    bool AnalyzeImage(const std::vector<uint8_t>& img);
    void SetModuleForAnalysis(uintptr_t base, uint32_t size, Game game = Game::DarkSoulsRemastered,
                              uint32_t fileVersionMS = 0, uint32_t fileVersionLS = 0) {
        base_ = base; size_ = size; game_ = game; fileVersionMS_ = fileVersionMS; fileVersionLS_ = fileVersionLS;
    }
    uintptr_t GameManAddr() const { return gameMan_; }
    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    const std::string& ScanLog() const { return game_ == Game::EldenRing ? er_.ScanLog() : scanLog_; }
    const EldenRingGame& eldenRing() const { return er_; }

private:
    bool OpenGame(DWORD pid);
    bool ScanPatterns();
    bool ProcessAlive() const;
    void ReadFileVersion();
    bool UpdateEldenRing(ULONGLONG now);

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

    // Version dependent offsets
    bool newLayout_ = true;     // 1.03+ (current Steam build) vs 1.01.x
    int  offSaveSlot_ = 0xAA0;
    int  offLastBonfire_ = 0xB34;
    int  offStableMap_ = 0xBD0;
    int  offChrMapData_ = 0x68;
    int  offHp_ = 0x3E8;
    std::string scanLog_;

    // Elden Ring (src/games/eldenring.*)
    Game      game_ = Game::None;
    uint32_t  fileVersionMS_ = 0, fileVersionLS_ = 0;
    EldenRingGame er_;
    ULONGLONG nextEacCheck_ = 0;
    int       eacClearChecks_ = 0;    // consecutive checks without EAC before the game may be opened
    bool      eacSeen_ = false;       // EAC protected this run of the game: never open it
    std::string eacReason_;
};
