#pragma once
#include "data_store.h"
#include "game_reader.h"
#include <deque>

// Turns game snapshots into per-character death counts and death records.
class DeathTracker {
public:
    explicit DeathTracker(DataStore& store) : store_(store) {}

    // Call ~10 times per second.
    void Tick(GameReader& reader);

    GameReader::State gameState() const { return gameState_; }
    const GameSnapshot& snapshot() const { return snap_; }
    bool hasSnapshot() const { return hasSnap_; }

    // Character currently loaded in the game (nullptr on the title screen / game closed).
    Profile* active();
    // True while the overlay should consider the player "in game" (in world, or briefly loading).
    bool inGame() const { return inGame_; }
    // Bumped every time a death is recorded (lets the UI flash / scroll).
    int deathEvents() const { return deathEvents_; }

private:
    struct Sample {
        ULONGLONG t;
        Vec3 pos;
        int mapArea, mapBlock, bonfire;
        uint32_t playTimeMs;
        int boss[GameSnapshot::kBossSlots] = { -1, -1 };   // boss bars on screen (name ids)
    };

    void Activate(const GameSnapshot& s, const std::string& baseKey);
    void RecordDeaths(Profile& p, int count);
    void AddUntracked(Profile& p, int count);

    DataStore& store_;
    GameReader::State gameState_ = GameReader::State::NotRunning;
    GameSnapshot snap_;
    bool hasSnap_ = false;

    std::string activeKey_;
    std::string candidateKey_;
    ULONGLONG candidateSince_ = 0;
    int lastDeaths_ = 0;

    bool inGame_ = false;
    ULONGLONG lastInWorld_ = 0;
    int deathEvents_ = 0;

    std::deque<Sample> history_;   // positions while alive, last ~10 s
};
