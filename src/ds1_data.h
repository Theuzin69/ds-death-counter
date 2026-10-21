#pragma once
#include <string>
#include <vector>

// Name of the map mAA_BB (e.g. 10,1 -> "Undead Burg / Undead Parish"). Empty if unknown.
std::string Ds1MapName(int area, int block);

// Name of a bonfire from the "last bonfire" id. Empty if unknown.
std::string Ds1BonfireName(int bonfireId);

// Name shown on a boss health bar, from its NPC name id (e.g. 5270 -> "Dragon Slayer Ornstein").
// Empty if unknown.
std::string Ds1BossBarName(int nameId);

// Cause of death for a fight given the name ids of the boss bars on screen (-1 = empty slot),
// worded like the matching suggestion (e.g. 5270 + 2360 -> "Chefe: Ornstein & Smough").
// Empty if no bar is up.
std::string Ds1BossCause(const int* nameIds, int count);

// Suggestions shown when editing the cause of a death.
const std::vector<std::string>& Ds1CauseSuggestions();

// Every known map name (for the "edit area" suggestions).
const std::vector<std::string>& Ds1AllMapNames();
