#pragma once
#include <string>
#include <vector>

// Name of the map mAA_BB (e.g. 10,1 -> "Undead Burg / Undead Parish"). Empty if unknown.
std::string Ds1MapName(int area, int block);

// Name of a bonfire from the "last bonfire" id. Empty if unknown.
std::string Ds1BonfireName(int bonfireId);

// Suggestions shown when editing the cause of a death.
const std::vector<std::string>& Ds1CauseSuggestions();

// Every known map name (for the "edit area" suggestions).
const std::vector<std::string>& Ds1AllMapNames();
