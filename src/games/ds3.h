#pragma once
// Dark Souls III (DarkSoulsIII.exe): signatures, offsets and memory reads.
//
// Same rules as the rest of GameReader: the process handle only has read access and
// only ReadProcessMemory is used. Research notes: docs/how-it-was-made.md.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

struct GameSnapshot;

class Ds3Reader {
public:
    // Finds the game's globals and offsets in a copy of the exe image (base = module base).
    // Appends one line per item to `log`. False if something required is missing.
    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log);
    // Fills `s` (already reset by the caller).
    void Read(HANDLE proc, GameSnapshot& s) const;

    uintptr_t gameDataMan = 0, worldChrMan = 0, gameMan = 0;
    int signaturesFound = 0, signaturesTotal = 0;   // every code signature Analyze looked for

    // 1.15 values. Most are read back from the game's own code by Analyze.
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
    } off;
};

// Name of the map mAA_BB (e.g. 30, 0 -> "High Wall of Lothric"). Empty if unknown.
std::string Ds3MapName(int area, int block);

// Label for a respawn point / bonfire id (AABxxxx): the map it belongs to plus the id.
std::string Ds3BonfireName(int id);

// Suggestions shown when editing the cause of a death.
const std::vector<std::string>& Ds3CauseSuggestions();

// Every known map name (for the "edit area" suggestions).
const std::vector<std::string>& Ds3AllMapNames();
