#pragma once
// Dark Souls III (DarkSoulsIII.exe), read-only like the rest of GameReader.
//
// Every global and most offsets are found at runtime from byte signatures in the game's
// own code (see ds3.cpp and docs/how-it-was-made.md). Verified statically on 1.15.0.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games.h"

struct GameSnapshot;

extern const GameInfo kDs3Info;

class Ds3Reader {
public:
    // Resolves globals and offsets from a copy of the exe image mapped at `base`.
    // Appends one line per structure to `log`. False if something essential is missing.
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log);
    bool Read(HANDLE proc, GameSnapshot& s);
    std::string LiveDiagnostics() const;

    static std::string VersionLabel(uint32_t moduleSize);

    uintptr_t gameDataMan = 0, worldChrMan = 0, gameMan = 0, fieldArea = 0;
    int signaturesFound = 0, signaturesTotal = 0;   // every code signature Analyze looked for

    // 1.15.0 values. Most are read back from the game's own code by Analyze.
    struct Offsets {
        int deaths = 0x98;          // GameDataMan
        int playTime = 0xA4;        // GameDataMan
        int playerIns = 0x80;       // WorldChrMan
        int saveSlot = 0xA60;       // GameMan
        int respawn = 0xACC;        // GameMan: last bonfire / respawn point
        int modules = 0x1F90;       // PlayerIns -> character modules
        int dataModule = 0x18;      // modules -> SprjChrDataModule
        int hp = 0xD8;              // SprjChrDataModule
        int playRegion = 0x1ABC;    // PlayerIns: play region of the floor the player stands on
        // FieldArea -> world info + loaded block index -> block list -> map id (cross-check / fallback)
        int worldInfo = 0x10, blockIndex = 0x20, blockCount = 0x18, blockList = 0x20, blockStride = 0x70, blockMapId = 8;
    } off;

private:
    bool hasBlockMap_ = false;      // the FieldArea block lookup was found
    uint32_t lastBlockMap_ = 0;     // last map id read from FieldArea (for the Diagnostics tab)
    int lastRegion_ = 0;            // last play region read from the player
};

// Plugs Ds3Reader into the game registry (games/games.h).
class Ds3Module : public GameModule {
public:
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t, std::string& log) override
    {
        return reader_.Analyze(img, base, log);
    }
    bool Read(HANDLE proc, GameSnapshot& s) override { return reader_.Read(proc, s); }
    std::string VersionLabel(uint32_t moduleSize) const override { return Ds3Reader::VersionLabel(moduleSize); }
    std::string LiveDiagnostics() const override { return reader_.LiveDiagnostics(); }
    const Ds3Reader& reader() const { return reader_; }

private:
    Ds3Reader reader_;
};

// Name of the map mAA_BB (e.g. 30, 0 -> "High Wall of Lothric"). Empty if unknown.
std::string Ds3MapName(int area, int block);

// Label for a respawn point / bonfire id (AABxxxx): the map it belongs to plus the id.
std::string Ds3BonfireName(int id);

// Suggestions shown when editing the cause of a death.
const std::vector<std::string>& Ds3CauseSuggestions();

// Every known map name (for the "edit area" suggestions).
const std::vector<std::string>& Ds3AllMapNames();
