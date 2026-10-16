#include "ds1_data.h"
#include <unordered_map>

std::string Ds1MapName(int area, int block)
{
    switch (area * 10 + block) {
        case 100: return "The Depths";
        case 101: return "Undead Burg / Undead Parish";
        case 102: return "Firelink Shrine";
        case 110: return "Painted World of Ariamis";
        case 120: return "Darkroot Garden / Darkroot Basin";
        case 121: return "Oolacile (Royal Wood / Township / Chasm of the Abyss)";
        case 130: return "The Catacombs";
        case 131: return "Tomb of the Giants";
        case 132: return "The Great Hollow / Ash Lake";
        case 140: return "Blighttown / Quelaag's Domain";
        case 141: return "Demon Ruins / Lost Izalith";
        case 150: return "Sen's Fortress";
        case 151: return "Anor Londo";
        case 160: return "New Londo Ruins / Valley of Drakes / The Abyss";
        case 170: return "The Duke's Archives / Crystal Cave";
        case 180: return "Kiln of the First Flame";
        case 181: return "Northern Undead Asylum";
    }
    return {};
}

std::string Ds1BonfireName(int id)
{
    static const std::unordered_map<int, const char*> names = {
        { 1811960, "Undead Asylum - Courtyard" },
        { 1811961, "Undead Asylum - Interior" },
        { 1021960, "Firelink Shrine" },
        { 1801960, "Firelink Altar" },
        { 1011962, "Undead Burg" },
        { 1011961, "Undead Burg - Sunlight Altar" },
        { 1011964, "Undead Parish" },
        { 1201961, "Darkroot Garden" },
        { 1601961, "Darkroot Basin" },
        { 1001960, "The Depths" },
        { 1401962, "Blighttown - Catwalk" },
        { 1401961, "Blighttown - Swamp" },
        { 1401960, "Quelaag's Domain" },
        { 1321962, "The Great Hollow" },
        { 1321961, "Ash Lake" },
        { 1321960, "Ash Lake - Stone Dragon" },
        { 1411961, "Demon Ruins - Entrance" },
        { 1411962, "Demon Ruins - Staircase" },
        { 1411963, "Demon Ruins - Catacombs" },
        { 1411964, "Lost Izalith - Lava Pits" },
        { 1411960, "Lost Izalith - Illusory Wall" },
        { 1411950, "Lost Izalith - Bed of Chaos" },
        { 1501961, "Sen's Fortress" },
        { 1511960, "Anor Londo" },
        { 1511962, "Anor Londo - Darkmoon Tomb" },
        { 1511961, "Anor Londo - Residence" },
        { 1511950, "Anor Londo - Chamber of the Princess" },
        { 1101960, "Painted World of Ariamis" },
        { 1701962, "Duke's Archives - Entrance" },
        { 1701961, "Duke's Archives - Prison Cell" },
        { 1701960, "Duke's Archives - Balcony" },
        { 1701950, "Crystal Cave" },
        { 1301960, "Catacombs - Necromancer" },
        { 1301961, "Catacombs - Illusory Wall" },
        { 1311961, "Tomb of the Giants - Patches" },
        { 1311960, "Tomb of the Giants" },
        { 1311950, "Altar of the Gravelord" },
        { 1601950, "The Abyss" },
        { 1211963, "Oolacile - Sanctuary Garden" },
        { 1211961, "Oolacile - Sanctuary" },
        { 1211962, "Oolacile - Township" },
        { 1211964, "Oolacile - Township Dungeon" },
        { 1211950, "Chasm of the Abyss" },
    };
    auto it = names.find(id);
    return it == names.end() ? std::string() : std::string(it->second);
}

const std::vector<std::string>& Ds1CauseSuggestions()
{
    static const std::vector<std::string> list = {
        "Queda", "Armadilha", "Lava", "Veneno / Toxina", "Maldição", "Invasor (PvP)", "Inimigo comum",
        "Chefe: Asylum Demon", "Chefe: Stray Demon", "Chefe: Taurus Demon", "Chefe: Bell Gargoyles",
        "Chefe: Capra Demon", "Chefe: Gaping Dragon", "Chefe: Moonlight Butterfly",
        "Chefe: Great Grey Wolf Sif", "Chefe: Chaos Witch Quelaag", "Chefe: Iron Golem",
        "Chefe: Ornstein & Smough", "Chefe: Dark Sun Gwyndolin", "Chefe: Crossbreed Priscilla",
        "Chefe: Pinwheel", "Chefe: Gravelord Nito", "Chefe: Seath the Scaleless", "Chefe: Four Kings",
        "Chefe: Ceaseless Discharge", "Chefe: Demon Firesage", "Chefe: Centipede Demon",
        "Chefe: Bed of Chaos", "Chefe: Gwyn, Lord of Cinder", "Chefe: Sanctuary Guardian",
        "Chefe: Knight Artorias", "Chefe: Black Dragon Kalameet", "Chefe: Manus, Father of the Abyss",
        "Hellkite Dragon (ponte)", "Hydra", "Havel the Rock", "Black Knight", "Titanite Demon",
        "Mimic", "Channeler", "Silver Knight", "Anor Londo Archers", "Undead Dragon",
        "Ghosts (New Londo)", "Basilisk", "Skeleton / Bonewheel", "Capra Demon Dogs",
        "Giant Crab", "Darkwraith", "Stone Guardian", "Man-Eater Shell",
    };
    return list;
}

const std::vector<std::string>& Ds1AllMapNames()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (int area = 10; area <= 18; ++area)
            for (int block = 0; block <= 2; ++block) {
                std::string n = Ds1MapName(area, block);
                if (!n.empty()) v.push_back(n);
            }
        return v;
    }();
    return list;
}
