#pragma once
// Elden Ring (eldenring.exe).
//
// Two ways to count, chosen through GameModule::CheckAccess before the game is ever opened:
//  - Easy Anti-Cheat running (normal Steam launch): the game process is never opened. The count
//    comes from the save file, which the game writes after every death (see eldenring_save.h).
//  - EAC not running (offline launch with EAC disabled, Seamless Co-op...): read-only memory
//    access like the other games, which counts instantly and knows the area of each death.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games.h"
#include "games/eldenring_save.h"

struct GameSnapshot;

extern const GameInfo kEldenRingInfo;

// Easy Anti-Cheat detection, done without touching the game process: the process list, the EAC
// services' state and the game's parent process (start_protected_game.exe is EAC's launcher).
struct ErEacCheck {
    bool active = false;
    std::string reason;
};
ErEacCheck ErDetectEac(DWORD gamePid);

// "1.02.0" from the exe's product version (1.2.0.0); 1.10 and later ship as 2.x (2.6.0.0 = 1.16.0).
std::string ErVersionName(uint32_t versionMS, uint32_t versionLS);

class EldenRingModule : public GameModule {
public:
    // Memory route. Resolves the game's globals in a copy of the exe image. The game version
    // (which picks a few offsets) comes from the version resource inside the image.
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t size, std::string& log) override;
    bool Read(HANDLE proc, GameSnapshot& s) override;
    std::string VersionLabel(uint32_t moduleSize) const override;
    std::string LiveDiagnostics() const override;

    // Easy Anti-Cheat and the save file route.
    Access CheckAccess(DWORD pid, std::string& reason) override;
    bool CanReadWithoutProcess() const override { return true; }
    bool ReadWithoutProcess(GameSnapshot& s) override;
    std::string WithoutProcessStatus() const override;
    void Poll() override { save_.Poll(); }

    const ErSaveWatcher& save() const { return save_; }
    // For scan_test
    bool allSignaturesUnique() const { return allUnique_; }
    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    int playerInsOffset() const { return offPlayerIns_; }
    uint32_t versionMS() const { return versionMS_; }
    uint32_t versionLS() const { return versionLS_; }

private:
    ErSaveWatcher save_;
    bool allUnique_ = false;
    uintptr_t gameDataMan_ = 0, worldChrMan_ = 0;
    uint32_t versionMS_ = 0, versionLS_ = 0;
    int offDeaths_ = 0x94, offPlayTime_ = 0xA0;
    int offPlayerIns_ = 0x1E508, offPos_ = 0x6D4, offMap_ = 0x6D0;

    // Easy Anti-Cheat checks for this run of the game
    int eacClearChecks_ = 0;    // consecutive checks without any sign of EAC
    bool eacSeen_ = false;      // EAC was running at some point: never open the game
    std::string eacReason_;

    // Matching the character in memory with its save slot.
    std::wstring matchName_;
    ULONGLONG matchSince_ = 0;
};
