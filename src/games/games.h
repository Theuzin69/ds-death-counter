#pragma once
// The games the counter recognizes. Dark Souls Remastered ("dsr") is the original game of
// the app; every other game registers a GameInfo here and implements a GameModule.

#include <memory>
#include <string>
#include <vector>
#include "game_module.h"

struct GameInfo {
    const char*    id;          // stored in Profile::game; prefix of the profile keys ("ds2|slot|name"), except "dsr"
    const wchar_t* exe;         // process / module name
    const char*    name;        // full name for the UI
    const char*    shortName;   // tag shown next to each character
    std::unique_ptr<GameModule> (*create)();   // nullptr = GameReader's own DSR code
    std::string (*bonfireName)(int id);        // "" when unknown
    const std::vector<std::string>& (*allMapNames)();       // suggestions when editing an area
    const std::vector<std::string>& (*causeSuggestions)();  // suggestions when editing a cause
    const char*    checkpointLabel;  // column title for bonfireName ("Última fogueira", "Último ídolo (área)")
    bool           hasLevel;         // the character has a level worth showing
    bool           fallHeuristic;    // big height loss before dying means a fall death
    bool           customReader;     // attached by its own reader (Bloodborne port), not by GameReader's scan
    // Optional: groups detailed area names for the "Por área" statistics ("Limgrave - Church of Elleh"
    // -> "Limgrave"). nullptr = each area on its own.
    std::string (*areaGroup)(const std::string& area) = nullptr;
    bool           countsItself = false;     // the game keeps no death counter (Hollow Knight): GameSnapshot::deaths counts the
                                     // deaths seen since the module attached and the app keeps each profile's total
};

const std::vector<const GameInfo*>& AllGames();
// Unknown or empty ids fall back to Dark Souls Remastered (data saved by 1.0.0 has no game id).
const GameInfo& GameById(const std::string& id);
// "Dark Souls Remastered e Dark Souls II: ..." for texts that list the supported games.
std::string SupportedGamesText();
