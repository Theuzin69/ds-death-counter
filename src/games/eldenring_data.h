#pragma once
// Elden Ring names: areas by map id, Sites of Grace by their map, cause suggestions.
#include <cstdint>
#include <string>
#include <vector>

// Map ids are packed like the game stores them: 0xAABBCCDD = mAA_BB_CC_DD
// (open world tiles are m60_XX_YY_00, the Realm of Shadow is m61_XX_YY_00).
std::string ErMapIdText(uint32_t mapId);             // "m60_42_36_00"
bool ErMapIdValid(uint32_t mapId);                   // plausible map id (rejects garbage reads)
// Area of a map: the legacy dungeon / dungeon name, or "Region - places" for open world tiles.
// Unknown maps fall back to their kind ("Catacombs (m30_21)") or to "Mapa mAA_BB_CC_DD".
std::string ErMapName(uint32_t mapId);
// Region part of an area name ("Limgrave - Church of Elleh" -> "Limgrave"), used to group statistics.
std::string ErRegionOf(const std::string& areaName);

// Map of an entity id (Sites of Grace are entities: 1042362950 = m60_42_36_00). 0 if not a map entity.
uint32_t ErEntityMap(uint32_t entityId);
// "Area of the last Site of Grace" label. Empty for 0.
std::string ErGraceLabel(uint32_t entityId);

const std::vector<std::string>& ErAllMapNames();
const std::vector<std::string>& ErCauseSuggestions();
