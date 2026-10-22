#pragma once
// Hollow Knight (hollow_knight.exe, Unity + Mono), read-only.
//
// The game logic is C# running on Unity's Mono runtime, so there are no stable globals in the
// exe. The reader walks the Mono runtime's own structures instead (loaded assemblies ->
// Assembly-CSharp -> classes -> fields), the same way speedrun autosplitters do, and reads the
// GameManager / PlayerData / HeroController objects by field name. Only ReadProcessMemory.
//
// The game has no death counter of its own, so the module counts deaths itself (DeathDetector)
// and the app keeps the total per save slot (GameInfo::countsItself).
// How it was found and checked: docs/how-it-was-made.md ("Hollow Knight").

#include "games.h"
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

extern const GameInfo kHollowKnightInfo;

namespace hk {

// Offsets inside the Mono runtime (mono-2.0-bdwgc.dll) the reader needs. AnalyzeMono reads them
// from the runtime's own exported functions; -1 = not found.
struct MonoLayout {
    uint32_t loadedAssemblies = 0;  // RVA of the global GList* of loaded assemblies
    int assemblyImage = -1;         // MonoAssembly -> MonoImage*
    int imageName = -1;             // MonoImage -> assembly name (char*)
    int imageClassCache = -1;       // MonoImage -> class cache (MonoInternalHashTable)
    int classParent = -1;           // MonoClass -> parent MonoClass*
    int className = -1;             // MonoClass -> name (char*)
    int classNamespace = -1;        // MonoClass -> namespace (char*)
    int classVtableSize = -1;       // MonoClass -> vtable_size (int)
    int classFields = -1;           // MonoClass -> MonoClassField[]
    int classRuntimeInfo = -1;      // MonoClass -> MonoClassRuntimeInfo* (one vtable per domain)
    int classDefFieldCount = -1;    // MonoClassDef -> field_count
    int classDefNextCache = -1;     // MonoClassDef -> next class in the same class cache bucket
    int fieldName = -1;             // MonoClassField -> name (char*)
    int fieldOffset = -1;           // MonoClassField -> offset (int)
    int fieldSize = 0;              // sizeof(MonoClassField)
    int vtableData = -1;            // MonoVTable -> vtable[]; static fields sit at vtable[vtable_size]
};

// Fills `out` from the runtime image `img` (mapped layout: index = RVA). Appends one line per
// item to `log`. True when everything the reader needs was found.
bool AnalyzeMono(const std::vector<uint8_t>& img, MonoLayout& out, std::string& log);
// Builds that mapped layout from the DLL file (tests).
bool MappedImageFromFile(const std::vector<uint8_t>& file, std::vector<uint8_t>& img);

// What the death logic needs from one reading.
struct Sample {
    bool inGame = false;        // a save is loaded and the Knight exists (not the menu)
    bool playing = false;       // GameManager.gameState == PLAYING
    uintptr_t save = 0;         // the loaded PlayerData object: changes when a save is (re)loaded
    int health = -1;            // PlayerData.health (masks)
    bool dead = false;          // HeroControllerStates.dead: set by real deaths, not dream ones
};

// Counts each death once. A death is health reaching 0 (every death goes through
// HeroController.Die with health 0, dream and Godhome deaths included) or the hero's
// "dead" state turning on. Both stay set until the respawn, which re-arms the detector.
// Spike/acid hits that only cost a mask, the menu, loading a save and quitting while
// dying don't count. The first reading after the menu or a load only sets the baseline.
class DeathDetector {
public:
    // True when `s` is a new death (count() was incremented).
    bool Update(const Sample& s);
    int count() const { return count_; }
    bool dying() const { return dying_; }

private:
    int count_ = 0;
    bool haveBase_ = false;     // previous reading was in game with the same save
    uintptr_t save_ = 0;
    int lastHealth_ = -1;
    bool lastDead_ = false;
    bool dying_ = false;        // counted, waiting for the respawn
};

// Map zone (GlobalEnums.MapZone) -> area name ("Forgotten Crossroads"); "" if unknown.
std::string MapZoneName(int zone);
// Dream and Godhome zones: dying there wakes the Knight up instead of leaving a shade.
bool IsDreamZone(int zone);

// A boss fought in `scene`. `flag` is the PlayerData bool set when it's beaten (nullptr = always
// a boss fight there, e.g. dreams and Godhome).
struct BossArena {
    const char* scene;
    const char* flag;
    const char* boss;
};
const std::vector<BossArena>& BossArenas();
const BossArena* FindBossArena(const std::string& scene);

// Bench (respawn scene) -> area id for DeathRecord::bonfire (stable: the table only grows); 0 = unknown.
int BenchAreaId(const std::string& respawnScene);
std::string BenchAreaName(int id);

const std::vector<std::string>& AllAreaNames();
const std::vector<std::string>& CauseSuggestions();

} // namespace hk
