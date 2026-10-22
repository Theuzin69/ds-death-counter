#pragma once
// Undertale (UNDERTALE.exe: a 32-bit GameMaker: Studio 1.4 runner plus data.win).
//
// The count is the game's own: on every GAME OVER screen obj_heartdefeated adds 1 to "Gameover" in
// the [General] section of %LOCALAPPDATA%\UNDERTALE\undertale.ini (a few seconds into the screen,
// before the player can continue). The game keeps that number across Resets and new names (Sans
// quotes it); only a True Reset deletes undertale.ini and starts it again from 0. The ini is only
// read, opened for a moment with full sharing so the game can always write or delete it.
//
// Area and cause come from read-only memory reads, and the count works without them:
//  - the current room: the global behind GameMaker's built-in variable "room", found through the
//    getter the runner registers for that name;
//  - global.battlegroup: a variable of the runner's global instance (hash map keyed by the
//    variable's slot, which comes from the VARI chunk of data.win).
// Game over room right after a battle room = died in that battle, in the area of the last room
// walked in before the battle.
//
// Character: Undertale has a single save, so a "character" is the fallen human's name: key
// "ut|0|<name>". The same name after a Reset continues the same character, as the game itself does.

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "games.h"

struct GameSnapshot;

extern const GameInfo kUndertaleInfo;

namespace ut {

// What the counter uses from undertale.ini.
struct IniData {
    int gameOvers = 0;          // [General] Gameover (0 when the key is missing: no game over yet)
    std::string name;           // [General] Name, written when the game is saved (UTF-8)
    int love = 0;               // [General] Love (LV at the last save)
    uint32_t timeFrames = 0;    // [General] Time, play time at the last save in frames (30 per second)
    int room = -1;              // [General] Room, room of the last save
};
// False when the text is not a whole ini file (empty, or cut in the middle of a write).
bool ParseIni(const std::string& text, IniData& out);

enum class RoomKind { Menu, World, Battle, GameOver };
RoomKind KindOfRoom(const std::string& roomName);
// "Ruins", "Snowdin", "Waterfall", "Hotland", "Core", "New Home", "True Lab", "Surface" ("" = unknown).
std::string AreaOfRoom(const std::string& roomName);
// Who the player fights in a battle group ("Chefe: Toriel", "Froggit e Whimsun"); "" when unknown.
std::string BattleGroupEnemies(int group);
// Internal room name in the room order of Undertale 1.08 ("" out of range).
std::string RoomName(int room);
// Label for the room of the last save: the game's save point name, or the area.
std::string SaveRoomLabel(int room);
const std::vector<std::string>& AllAreaNames();
const std::vector<std::string>& CauseSuggestions();

// Follows the current room (and the battle group during battles) to tell where and to whom the
// last game over happened. Fed on every read; the counter asks area()/cause() when the game's
// count goes up, which happens while the game over screen is still up.
class DeathContext {
public:
    // battleGroup: global.battlegroup, -1 when it can't be read.
    void Update(const std::string& roomName, int battleGroup, ULONGLONG now);
    std::string area() const;
    std::string cause() const;
    bool inGameOver() const { return inGameOver_; }

private:
    bool recentGameOver() const;

    std::string lastWorldRoom_, battleRoom_;
    RoomKind prevKind_ = RoomKind::Menu;
    int battleGroup_ = -1;          // group of the current / last battle
    int worldEntryGroup_ = -1;      // global.battlegroup when the current walking room was entered
    bool inGameOver_ = false;
    ULONGLONG now_ = 0, leftGameOver_ = 0;
    std::string goArea_, goCause_;
};

// The few things the counter needs from data.win (read-only, from the game folder).
struct DataWin {
    int bytecode = 0;                   // GEN8 bytecode version (Undertale 1.08: 16)
    std::vector<std::string> rooms;     // room names, by room index
    int slotBattleGroup = -1, slotCharName = -1, slotLv = -1;   // global variable slots
    std::string error;
    bool Load(const std::wstring& path);
};

} // namespace ut

class UndertaleModule : public GameModule {
public:
    UndertaleModule();
    // Tests: read undertale.ini from another folder instead of %LOCALAPPDATA%\UNDERTALE.
    explicit UndertaleModule(const std::wstring& saveFolder) : saveFolder_(saveFolder) {}

    bool Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t size, std::string& log) override;
    bool Read(HANDLE proc, GameSnapshot& s) override;
    std::string VersionLabel(uint32_t moduleSize) const override;
    std::string LiveDiagnostics() const override;
    bool Is32Bit() const override { return true; }
    // The ini alone is enough to count (e.g. a runner this module doesn't recognize).
    bool CanReadWithoutProcess() const override { return true; }
    bool ReadWithoutProcess(GameSnapshot& s) override;
    std::string WithoutProcessStatus() const override { return "pelo undertale.ini (sem área e causa)"; }

    // For tests
    uintptr_t RoomAddr() const { return roomAddr_; }
    uintptr_t GlobalInstanceAddr() const { return globalAddr_; }
    int VarsOffset() const { return offVars_; }
    int MapOffset() const { return offMap_; }
    const ut::DataWin& dataWin() const { return dataWin_; }
    bool LoadDataWin(const std::wstring& path);
    const ut::IniData& ini() const { return ini_; }
    bool PollIni();

private:
    bool ReadGlobal(HANDLE proc, int slot, double* num, std::string* str);
    void FillFromIni(GameSnapshot& s) const;

    std::wstring saveFolder_;
    // Runner globals (absolute addresses in the 32-bit process)
    uintptr_t roomAddr_ = 0, globalAddr_ = 0;
    int offVars_ = 4, offMap_ = 0x44;   // CInstance: RValue array by slot, or the hash map of variables

    // data.win, loaded once the exe path is known
    bool dataWinTried_ = false;
    ut::DataWin dataWin_;

    // undertale.ini
    ut::IniData ini_;
    bool iniOk_ = false;
    FILETIME iniTime_{};
    uint64_t iniSize_ = 0;
    ULONGLONG nextIniCheck_ = 0;
    int pendingLower_ = 0;          // smaller Gameover waiting for confirmation, + 1 (0 = none)
    ULONGLONG lowerSince_ = 0;

    // Live state
    ut::DeathContext ctx_;
    int room_ = -1, battleGroup_ = -1;
    std::string charName_;
    bool globalsOk_ = false;
};
