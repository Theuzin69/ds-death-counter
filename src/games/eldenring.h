#pragma once
// Elden Ring support.
//
// Two ways to count, chosen by GameReader:
//  - Easy Anti-Cheat running (normal Steam launch): the game process is never opened. The count
//    comes from the save file, which the game writes after every death (see eldenring_save.h).
//  - EAC not running (offline launch with EAC disabled, Seamless Co-op...): read-only memory
//    access like Dark Souls Remastered, which counts instantly and knows the area of each death.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games/eldenring_save.h"

struct GameSnapshot;

// Easy Anti-Cheat detection, done without touching the game process: the process list, the EAC
// services' state and the game's parent process (start_protected_game.exe is EAC's launcher).
struct ErEacCheck {
    bool active = false;
    std::string reason;
};
ErEacCheck ErDetectEac(DWORD gamePid);

// "1.02.0" from the exe's product version (1.2.0.0); 1.10 and later ship as 2.x (2.6.0.0 = 1.16.0).
std::string ErVersionName(uint32_t versionMS, uint32_t versionLS);

class EldenRingGame {
public:
    // Memory route. Resolves the game's globals in a copy of the exe image (also used by scan_test).
    // fileVersionMS/LS: the exe's product version (0 if unknown: newest layout).
    bool AnalyzeImage(const std::vector<uint8_t>& img, uintptr_t base, uint32_t fileVersionMS, uint32_t fileVersionLS);
    bool ReadMemory(HANDLE proc, GameSnapshot& s);
    // Save route: never touches the game.
    bool ReadSave(GameSnapshot& s);

    void Poll() { save_.Poll(); }
    void Reset();
    void SetMemoryMode(bool memory) { save_.SetAllowCoop(memory); }
    const ErSaveWatcher& save() const { return save_; }

    // Diagnostics
    const std::string& ScanLog() const { return scanLog_; }
    bool allSignaturesUnique() const { return allUnique_; }
    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    int playerInsOffset() const { return offPlayerIns_; }

private:
    ErSaveWatcher save_;
    std::string scanLog_;
    bool allUnique_ = false;
    uintptr_t gameDataMan_ = 0, worldChrMan_ = 0;
    int offDeaths_ = 0x94, offPlayTime_ = 0xA0;
    int offPlayerIns_ = 0x1E508, offPos_ = 0x6D4, offMap_ = 0x6D0;

    // Matching the character in memory with its save slot.
    std::wstring matchName_;
    ULONGLONG matchSince_ = 0;
};
