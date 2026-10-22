#pragma once
// Dark Souls II: Scholar of the First Sin (64-bit DarkSoulsII.exe), read-only.
// How every address below was found: docs/how-it-was-made.md ("Dark Souls II").

#include "games.h"

extern const GameInfo kDs2Info;

namespace ds2 {

// The GameManagerImp global and the offsets of the fields the counter reads. Defaults are the
// values of the Steam 1.02 build; Analyze() replaces them with what it reads from the game's code.
struct Layout {
    uintptr_t gameManager = 0;      // GameManagerImp* (static in the exe)
    uintptr_t loadState = 0;        // [ptr]+0x11C == 1 while loading (diagnostics only)
    // GameManagerImp -> PlayerCtrl -> PlayerParam
    int offPlayerCtrl = 0xD0, offPlayerParam = 0x490, offHp = 0x168;
    int offDeaths = 0x1A4;          // PlayerParam: total deaths (saved; clamped to 99,999,999)
    int offCounters = 0x104;        // PlayerParam: 20 {int, flag} counters, the first one also counts deaths
    int offSoulLevel = 0xD0;
    // GameManagerImp -> game data -> character info (name, class, NG cycle)
    int offGameData = 0xA8, offCharInfo = 0xC0, offName = 0x24, offNgCycle = 0x68;
    // game data -> save slot summaries (the load menu's data)
    int offSlotSummaries = 0xD8, offSlotIndex = 0x1368, summarySize = 0x1F0, offTimeStamp = 0x1360, offPlayTime = 0x1CC;
    // GameManagerImp -> map manager: current map index -> map id table; place name ids
    int offMapMan = 0x38, offMapIndex = 0x170, offPlaceBase = 0x178, offPlaceDlc = 0x17C;
    int offMapTable = 0x10, offMapCount = 0x18, offMapIds = 0x20;
    // GameManagerImp -> event manager: last bonfire rested at
    int offEventMan = 0x70, offBonfireArea = 0x164, offBonfireId = 0x16C;
    // PlayerCtrl -> position (x, height, z)
    int offPosData = 0x100, offPos = 0x80;
};

class Module : public GameModule {
public:
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t size, std::string& log) override;
    bool Read(HANDLE proc, GameSnapshot& s) override;
    std::string VersionLabel(uint32_t moduleSize) const override;
    std::string LiveDiagnostics() const override;
    const Layout& layout() const { return L_; }

private:
    Layout L_;
    // Last raw values, for the Diagnostics tab.
    int counter0_ = -1, placeId_ = -1, mapIndex_ = -1, ngRaw_ = -1, bonfireArea_ = -1, loading_ = -1;
    uint32_t savedSeconds_ = 0;
};

// Name of the map mAA_BB (e.g. 10,4 -> "Majula"). Empty if unknown.
std::string MapName(int area, int block);
// Name of a bonfire from its id (e.g. 4650 -> "The Far Fire"). Empty if unknown.
std::string BonfireName(int id);
const std::vector<std::string>& AllMapNames();
const std::vector<std::string>& CauseSuggestions();

} // namespace ds2
