#pragma once
// Sekiro: Shadows Die Twice (sekiro.exe), read-only like the rest of GameReader.
//
// Every address and offset is found at runtime from byte signatures in the game's own
// code (see sekiro.cpp and docs/how-it-was-made.md). Verified statically on 1.06.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games.h"

struct GameSnapshot;

extern const GameInfo kSekiroInfo;

class SekiroReader {
public:
    // Resolves globals and offsets from a copy of the exe image mapped at `base`.
    // Appends one line per structure to `log`. False if something essential is missing.
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log);
    bool Read(HANDLE proc, GameSnapshot& s) const;
    void Reset() { *this = SekiroReader(); }

    static std::string VersionLabel(uint32_t moduleSize);

    uintptr_t GameDataManAddr() const { return gameDataMan_; }
    uintptr_t GameManAddr() const { return gameMan_; }
    uintptr_t WorldChrManAddr() const { return worldChrMan_; }
    uintptr_t FieldAreaAddr() const { return fieldArea_; }

private:
    // Globals (static pointers inside the exe)
    uintptr_t gameDataMan_ = 0, gameMan_ = 0, worldChrMan_ = 0, fieldArea_ = 0;

    // GameDataMan
    int offDeaths_ = -1, offPlayTime_ = -1, offClearCount_ = -1;
    // GameMan
    int offSaveSlot_ = -1, offReturnPoint_ = -1, offReturnOverride_ = -1;
    // WorldChrMan -> PlayerIns -> module bag -> data module (HP) / physics module (position)
    int offPlayer_ = -1, offModules_ = -1, offDataModule_ = -1, offHp_ = -1, offPhysModule_ = -1, offPos_ = -1;
    // FieldArea -> current world block -> map id
    int offBlockIndex_ = -1, offWorldInfo_ = -1, offBlockCount_ = -1, offBlockList_ = -1, blockStride_ = 0,
        offBlockMapId_ = -1;
};

// Plugs SekiroReader into the game registry (games/games.h).
class SekiroModule : public GameModule {
public:
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t, std::string& log) override
    {
        return reader_.Analyze(img, base, log);
    }
    bool Read(HANDLE proc, GameSnapshot& s) override { return reader_.Read(proc, s); }
    std::string VersionLabel(uint32_t moduleSize) const override { return SekiroReader::VersionLabel(moduleSize); }
    const SekiroReader& reader() const { return reader_; }

private:
    SekiroReader reader_;
};
