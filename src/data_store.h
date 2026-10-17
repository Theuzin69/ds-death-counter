#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Settings {
    bool firstRunDone = false;
    std::string gameExePath;            // UTF-8, used by the "Iniciar Dark Souls" button
    bool startWithWindows = true;
    bool closeToTray = true;
    bool hotkeyEnabled = true;          // Ctrl+Shift+D toggles the overlay

    bool overlayEnabled = true;
    bool overlayOnlyWhenFocused = true; // hide while alt-tabbed
    bool overlayHideInMenu = true;      // hide on the title screen
    bool overlayShowName = false;
    bool overlayScaleWithGame = true;   // font size relative to a 1080p game window
    int  overlayAnchor = 0;             // 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right
    int  overlayOffsetX = 32;
    int  overlayOffsetY = 32;
    float overlayFontSize = 36.0f;
    std::string overlayFont = "Georgia";
    bool overlayBold = true;
    std::string overlayLabel = "Mortes: ";
    float overlayOutline = 3.0f;
    uint32_t colorNormal = 0xFFE8DFC8;  // ARGB
    uint32_t colorEdited = 0xFFFF8A2A;
    uint32_t colorOutline = 0xFF000000;
};

struct DeathRecord {
    int64_t id = 0;
    int gameCount = 0;          // value of the in-game counter right after this death (0 = unknown)
    std::string time;           // local time when detected
    uint32_t playTimeMs = 0;
    int mapArea = -1, mapBlock = -1;
    std::string area;           // display name (auto or edited)
    int bonfire = 0;
    bool hasPos = false;
    float x = 0, y = 0, z = 0;
    std::string cause;          // auto (heuristic) or typed by the user
    bool causeEdited = false;
    bool untracked = false;     // happened while the counter was closed
};

struct Profile {
    std::string game = "dsr";   // game id (games/games.h); data saved by 1.0.0 is Dark Souls Remastered
    std::string key;            // "slot|name" (+ "#N" when a slot was reused by a new character); other games: "ds2|slot|name"
    std::string baseKey;        // "slot|name" (other games: "ds2|slot|name")
    std::string name;
    int slot = -1;
    int gameDeaths = 0;         // last value read from the game
    int baseDeaths = 0;         // in-game deaths when the counter first saw this character
    int offset = 0;             // manual adjustment: shown = gameDeaths + offset
    bool edited = false;
    uint32_t playTimeMs = 0;
    int soulLevel = 0;
    int ngPlus = 0;
    std::string firstSeen, lastSeen;
    std::vector<DeathRecord> deaths;

    int Shown() const { return gameDeaths + offset; }
};

class DataStore {
public:
    Settings settings;
    std::vector<Profile> profiles;

    bool Load();
    bool Save();
    void MarkDirty() { dirty_ = true; }
    void SaveIfDirty() { if (dirty_) Save(); }

    Profile* Find(const std::string& key);
    int64_t NextRecordId();
    std::wstring FilePath() const;

private:
    bool dirty_ = false;
    int64_t lastId_ = 0;
};
