#pragma once
#include <string>
#include <vector>

// Name of the map mAA_BB (e.g. 11,1 -> "Ashina Castle"). Empty if unknown.
std::string SekiroMapName(int area, int block);

// Area of the game's respawn point (the last Sculptor's Idol), from its entity id
// (AA B NNNN: map mAA_0B). Empty if the id doesn't belong to a known map.
std::string SekiroIdolArea(int returnPointId);

// Suggestions shown when editing the cause of a death.
const std::vector<std::string>& SekiroCauseSuggestions();

// Every known map name (for the "edit area" suggestions).
const std::vector<std::string>& SekiroAllMapNames();
