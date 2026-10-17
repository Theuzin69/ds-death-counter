// Dark Souls II names. Map ids come from the area ids of the bonfire list (an area id is the
// map id mAA_BB packed as 0xAABB0000); bonfire ids and names from DS2S-META's bonfire table
// (MIT license, see THIRD_PARTY_NOTICES.md). Names stay in English, as in the game.
#include "ds2.h"
#include <unordered_map>

namespace ds2 {

std::string MapName(int area, int block)
{
    switch (area * 100 + block) {
        case 1002: return "Things Betwixt";
        case 1004: return "Majula";
        case 1010: return "Forest of Fallen Giants";
        case 1014: return "Brightstone Cove Tseldora";
        case 1015: return "Aldia's Keep";
        case 1016: return "The Lost Bastille / Sinners' Rise / Belfry Luna";
        case 1017: return "Harvest Valley / Earthen Peak";
        case 1018: return "No-Man's Wharf";
        case 1019: return "Iron Keep / Belfry Sol";
        case 1023: return "Huntsman's Copse / Undead Purgatory";
        case 1025: return "The Gutter / Black Gulch";
        case 1027: return "Dragon Aerie / Dragon Shrine";
        case 1029: return "Shaded Woods (Old Akelarre)";
        case 1031: return "Heide's Tower of Flame / Cathedral of Blue";
        case 1032: return "Shaded Woods";
        case 1033: return "Doors of Pharros";
        case 1034: return "Grave of Saints";
        case 2011: return "Shrine of Amana";
        case 2021: return "Drangleic Castle / King's Passage";
        case 2024: return "Undead Crypt";
        case 5035: return "Shulva, Sanctum City / Dragon's Sanctum";
        case 5036: return "Brume Tower / Iron Passage";
        case 5037: return "Frozen Eleum Loyce";
    }
    return {};
}

std::string BonfireName(int id)
{
    static const std::unordered_map<int, const char*> names = {
        { 2650, "Fire Keepers' Dwelling" },
        { 4650, "The Far Fire" },
        { 10670, "The Crestfallen's Retreat" },
        { 10655, "Cardinal Tower" },
        { 10660, "Soldiers' Rest" },
        { 10675, "The Place Unbeknownst" },
        { 31655, "Heide's Ruin" },
        { 31650, "Tower of Flame" },
        { 31660, "The Blue Cathedral" },
        { 18650, "Unseen Path to Heide" },
        { 16655, "Exile Holding Cells" },
        { 16670, "McDuff's Workshop" },
        { 16675, "Servants' Quarters" },
        { 16650, "Straid's Cell" },
        { 16660, "The Tower Apart" },
        { 16685, "The Saltfort" },
        { 16665, "Upper Ramparts" },
        { 23650, "Undead Refuge" },
        { 23655, "Bridge Approach" },
        { 23660, "Undead Lockaway" },
        { 23665, "Undead Purgatory" },
        { 17665, "Poison Pool" },
        { 17650, "The Mines" },
        { 17655, "Lower Earthen Peak" },
        { 17670, "Central Earthen Peak" },
        { 17675, "Upper Earthen Peak" },
        { 19655, "Threshold Bridge" },
        { 19650, "Ironhearth Hall" },
        { 19660, "Eygil's Idol" },
        { 19665, "Belfry Sol Approach" },
        { 29650, "Old Akelarre" },
        { 32655, "Ruined Fork Road" },
        { 32660, "Shaded Ruins" },
        { 33655, "Gyrm's Respite" },
        { 33660, "Ordeal's End" },
        { 14655, "Royal Army Campsite" },
        { 14660, "Chapel Threshold" },
        { 14650, "Lower Brightstone Cove" },
        { 34655, "Harval's Resting Place" },
        { 34650, "Grave Entrance" },
        { 25665, "Upper Gutter" },
        { 25655, "Central Gutter" },
        { 25650, "Black Gulch Mouth" },
        { 25660, "Hidden Chamber" },
        { 21650, "King's Gate" },
        { 21665, "Under Castle Drangleic" },
        { 21655, "Central Castle Drangleic" },
        { 21660, "Forgotten Chamber" },
        { 11650, "Tower of Prayer (Amana)" },
        { 11655, "Crumbled Ruins" },
        { 11660, "Rhoy's Resting Place" },
        { 11670, "Rise of the Dead" },
        { 24655, "Undead Crypt Entrance" },
        { 24650, "Undead Ditch" },
        { 15650, "Foregarden" },
        { 15655, "Ritual Site" },
        { 27650, "Dragon Aerie" },
        { 27655, "Shrine Entrance" },
        { 35650, "Sanctum Walk" },
        { 35685, "Tower of Prayer (Shulva)" },
        { 35655, "Priestess' Chamber" },
        { 35670, "Hidden Sanctum Chamber" },
        { 35675, "Lair of the Imperfect" },
        { 35680, "Sanctum Interior" },
        { 35665, "Sanctum Nadir" },
        { 36650, "Throne Floor" },
        { 36660, "Upper Floor" },
        { 36655, "Foyer" },
        { 36670, "Lowermost Floor" },
        { 36675, "The Smelter Throne" },
        { 36665, "Iron Hallway Entrance" },
        { 37650, "Outer Wall" },
        { 37660, "Abandoned Dwelling" },
        { 37675, "Expulsion Chamber" },
        { 37685, "Inner Wall" },
        { 37665, "Lower Garrison" },
        { 37670, "Grand Cathedral" },
    };
    auto it = names.find(id);
    return it == names.end() ? std::string() : std::string(it->second);
}

const std::vector<std::string>& AllMapNames()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (int area : { 10, 20, 50 })
            for (int block = 0; block < 100; ++block) {
                std::string n = MapName(area, block);
                if (!n.empty()) v.push_back(n);
            }
        return v;
    }();
    return list;
}

const std::vector<std::string>& CauseSuggestions()
{
    static const std::vector<std::string> list = {
        "Queda", "Armadilha", "Veneno / Toxina", "Petrificação / Maldição", "Invasor (PvP)", "Inimigo comum",
        "Chefe: The Last Giant", "Chefe: The Pursuer", "Chefe: Dragonrider", "Chefe: Old Dragonslayer",
        "Chefe: Flexile Sentry", "Chefe: Ruin Sentinels", "Chefe: Belfry Gargoyles", "Chefe: The Lost Sinner",
        "Chefe: Executioner's Chariot", "Chefe: Skeleton Lords", "Chefe: Covetous Demon",
        "Chefe: Mytha, the Baneful Queen", "Chefe: Smelter Demon", "Chefe: Old Iron King",
        "Chefe: Scorpioness Najka", "Chefe: Royal Rat Authority", "Chefe: Prowling Magus and Congregation",
        "Chefe: The Duke's Dear Freja", "Chefe: Royal Rat Vanguard", "Chefe: The Rotten", "Chefe: Twin Dragonriders",
        "Chefe: Looking Glass Knight", "Chefe: Demon of Song", "Chefe: Velstadt, the Royal Aegis", "Chefe: Vendrick",
        "Chefe: Guardian Dragon", "Chefe: Ancient Dragon", "Chefe: Giant Lord",
        "Chefe: Throne Watcher and Throne Defender", "Chefe: Nashandra", "Chefe: Aldia, Scholar of the First Sin",
        "Chefe: Darklurker", "Chefe: Elana, the Squalid Queen", "Chefe: Sinh, the Slumbering Dragon",
        "Chefe: Afflicted Graverobber, Ancient Soldier Varg and Cerah", "Chefe: Fume Knight",
        "Chefe: Sir Alonne", "Chefe: Blue Smelter Demon", "Chefe: Aava, the King's Pet",
        "Chefe: Burnt Ivory King", "Chefe: Lud and Zallen, the King's Pets",
        "Heide Knight", "Ogre", "Mimic", "Alonne Knight", "Royal Swordsman", "Ironclad Soldier", "Stone Soldier",
        "Hollow Infantry", "Parasitized Undead", "Desert Sorceress", "Gyrm Warrior", "Rat", "Giant Basilisk",
    };
    return list;
}

} // namespace ds2
