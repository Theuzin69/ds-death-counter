#pragma once
// Read-only access to the memory of the supported games (DarkSoulsRemastered.exe and,
// through the modules in src/games/, the other games).
//
// Only ReadProcessMemory is used: nothing is written to the game, nothing is
// injected and no hooks are installed. The game cannot tell the difference
// between this and any other program that happens to be running.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games/sekiro.h"

struct Vec3 { float x = 0, y = 0, z = 0; };

// Game ids, also stored in Profile::game: "dsr" (Dark Souls Remastered), "sekiro".
// Returns nullptr for an exe that isn't a supported game.
const char* GameIdFromExeName(const wchar_t* exeName);
// "Dark Souls Remastered", "Sekiro" (the id itself if unknown).
const char* GameDisplayName(const std::string& gameId);
// "DSR", "Sekiro": for lists.
const char* GameShortName(const std::string& gameId);

// Byte-pattern helpers ("48 8B 05 ? ? ? ?"), shared with the per-game modules in src/games/.
std::vector<size_t> FindPattern(const std::vector<uint8_t>& img, const char* pattern);
bool PatternAt(const std::vector<uint8_t>& img, size_t at, const char* pattern);
int32_t ImageI32(const std::vector<uint8_t>& img, size_t at);

struct GameSnapshot {
    std::string game = "dsr";       // which game this snapshot came from
    std::string areaName;           // current area, filled by the game modules (empty: use mapArea/mapBlock)
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
    int      lastBonfire  = 0;      // DSR: last bonfire id; Sekiro: respawn point (last idol) entity id
    int      hp           = -1;
    bool     hasPos       = false;
    Vec3     pos;
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
    void SetModuleForAnalysis(uintptr_t base, uint32_t size, const char* game = "dsr")
    {
        base_ = base;
        size_ = size;
        game_ = game;
    }
    uintptr_t GameManAddr() const { return gameMan_; }
    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    const std::string& ScanLog() const { return scanLog_; }
    // Game id of the attached process ("dsr", "sekiro").
    const std::string& GameId() const { return game_; }
    const SekiroReader& Sekiro() const { return sekiro_; }

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

    // Version dependent offsets
    bool newLayout_ = true;     // 1.03+ (current Steam build) vs 1.01.x
    int  offSaveSlot_ = 0xAA0;
    int  offLastBonfire_ = 0xB34;
    int  offStableMap_ = 0xBD0;
    int  offChrMapData_ = 0x68;
    int  offHp_ = 0x3E8;
    std::string scanLog_;

    // Which game is attached, and the modules for games other than DSR
    std::string  game_ = "dsr";
    std::wstring exeName_;
    SekiroReader sekiro_;
};
