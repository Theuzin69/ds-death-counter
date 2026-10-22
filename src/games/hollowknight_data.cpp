// Hollow Knight names: map zones, boss arenas and bench areas.
// Scene names checked against the game's build scene list (globalgamemanagers), defeat flags
// against PlayerData's fields (Assembly-CSharp.dll), both from the 2026 Steam build.
#include "games/hollowknight.h"
#include <algorithm>
#include <cstring>
#include <iterator>

namespace hk {

namespace {

// GlobalEnums.MapZone, in enum order.
const char* const kZones[] = {
    "",                         // NONE
    "",                         // TEST_AREA
    "King's Pass",              // KINGS_PASS
    "Howling Cliffs",           // CLIFFS
    "Dirtmouth",                // TOWN
    "Forgotten Crossroads",     // CROSSROADS
    "Greenpath",                // GREEN_PATH
    "Queen's Gardens",          // ROYAL_GARDENS
    "Fog Canyon",               // FOG_CANYON
    "Fungal Wastes",            // WASTES
    "Deepnest",                 // DEEPNEST
    "The Hive",                 // HIVE
    "Bone Forest",              // BONE_FOREST
    "Palace Grounds",           // PALACE_GROUNDS
    "Crystal Peak",             // MINES
    "Resting Grounds",          // RESTING_GROUNDS
    "City of Tears",            // CITY
    "Dream",                    // DREAM_WORLD
    "Colosseum of Fools",       // COLOSSEUM
    "Ancient Basin",            // ABYSS
    "Royal Quarter",            // ROYAL_QUARTER
    "White Palace",             // WHITE_PALACE
    "Ancestral Mound",          // SHAMAN_TEMPLE
    "Royal Waterways",          // WATERWAYS
    "Queen's Station",          // QUEENS_STATION
    "Kingdom's Edge",           // OUTSKIRTS
    "King's Station",           // KINGS_STATION
    "Soul Sanctum",             // MAGE_TOWER
    "Tram",                     // TRAM_UPPER
    "Tram",                     // TRAM_LOWER
    "Black Egg Temple",         // FINAL_BOSS
    "Soul Sanctum",             // SOUL_SOCIETY
    "Lake of Unn",              // ACID_LAKE
    "Stone Sanctuary",          // NOEYES_TEMPLE
    "Teacher's Archives",       // MONOMON_ARCHIVE
    "Mantis Village",           // MANTIS_VILLAGE
    "Ruined Tramway",           // RUINED_TRAMWAY
    "Distant Village",          // DISTANT_VILLAGE
    "The Abyss",                // ABYSS_DEEP
    "Isma's Grove",             // ISMAS_GROVE
    "Cast-Off Shell",           // WYRMSKIN
    "Watcher's Spire",          // LURIENS_TOWER
    "Tower of Love",            // LOVE_TOWER
    "Spirits' Glade",           // GLADE
    "Blue Lake",                // BLUE_LAKE
    "Hallownest's Crown",       // PEAK
    "Joni's Repose",            // JONI_GRAVE
    "Overgrown Mound",          // OVERGROWN_MOUND
    "Crystallised Mound",       // CRYSTAL_MOUND
    "Beast's Den",              // BEASTS_DEN
    "Godhome",                  // GODS_GLORY
    "Junk Pit",                 // GODSEEKER_WASTE
};
constexpr int kDreamWorld = 17, kGodsGlory = 50;

// Bench areas: ids are stored in data.json, so only append to this list.
const char* const kBenchAreas[] = {
    "",                         // 0 = unknown
    "King's Pass",              // 1
    "Dirtmouth",                // 2
    "Forgotten Crossroads",     // 3
    "Greenpath",                // 4
    "Fungal Wastes",            // 5
    "Queen's Gardens",          // 6
    "Teacher's Archives",       // 7
    "City of Tears",            // 8
    "Royal Waterways",          // 9
    "Deepnest",                 // 10
    "Kingdom's Edge",           // 11
    "Ancient Basin",            // 12
    "Crystal Peak",             // 13
    "Resting Grounds",          // 14
    "The Hive",                 // 15
    "Howling Cliffs",           // 16
    "White Palace",             // 17
    "Godhome",                  // 18
    "Colosseum of Fools",       // 19
};

// Scene name prefix -> bench area. First match wins, so longer prefixes come first.
const struct { const char* prefix; int id; } kBenchPrefixes[] = {
    { "Tutorial_", 1 }, { "Town", 2 }, { "Room_Town", 2 }, { "Room_Bretta", 2 }, { "Crossroads_", 3 },
    { "Room_temple", 3 }, { "Room_Final_Boss", 3 }, { "Fungus1_", 4 }, { "Room_Slug_Shrine", 4 },
    { "Fungus2_", 5 }, { "Fungus3_archive", 7 }, { "Fungus3_", 6 }, { "Room_Queen", 6 }, { "Ruins", 8 },
    { "Waterways_", 9 }, { "Deepnest_East_", 11 }, { "Room_Colosseum", 19 }, { "Deepnest_", 10 },
    { "Room_spider", 10 }, { "Room_Tram_RG", 14 }, { "Room_Tram", 10 }, { "Abyss_", 12 }, { "Mines_", 13 },
    { "RestingGrounds_", 14 }, { "Room_Mansion", 14 }, { "Hive_", 15 }, { "Cliffs_", 16 },
    { "White_Palace_", 17 }, { "GG_", 18 },
};

} // namespace

std::string MapZoneName(int zone)
{
    if (zone < 0 || zone >= (int)std::size(kZones)) return {};
    return kZones[zone];
}

bool IsDreamZone(int zone)
{
    return zone == kDreamWorld || zone == kGodsGlory;
}

const std::vector<BossArena>& BossArenas()
{
    // World bosses only count while their defeat flag is off (the arena is a normal room
    // afterwards). Dream, Colosseum and Godhome scenes are always a fight. Godhome's "_V"
    // variants (pantheon versions) use the entry of the base scene.
    static const std::vector<BossArena> list = {
        { "Crossroads_10", "falseKnightDefeated", "False Knight" },
        { "Crossroads_04", "giantFlyDefeated", "Gruz Mother" },
        { "Crossroads_09", "mawlekDefeated", "Brooding Mawlek" },
        { "Fungus1_04", "hornet1Defeated", "Hornet Protector" },
        { "Fungus1_29", "megaMossChargerDefeated", "Massive Moss Charger" },
        { "Fungus1_20_v02", "giantBuzzerDefeated", "Vengefly King" },
        { "Fungus2_15", "defeatedMantisLords", "Mantis Lords" },
        { "Fungus3_archive_02", "defeatedMegaJelly", "Uumuu" },
        { "Fungus3_23", "killedTraitorLord", "Traitor Lord" },
        { "Fungus3_40", "killedGhostMarmu", "Marmu" },
        { "Ruins1_24", "mageLordDefeated", "Soul Master" },
        { "Ruins2_03", "killedBlackKnight", "Watcher Knights" },
        { "Ruins2_11", "collectorDefeated", "The Collector" },
        { "Waterways_05", "defeatedDungDefender", "Dung Defender" },
        { "Waterways_12", "flukeMotherDefeated", "Flukemarm" },
        { "Abyss_19", "killedInfectedKnight", "Broken Vessel" },
        { "Mines_18", "defeatedMegaBeamMiner", "Crystal Guardian" },
        { "Mines_32", "defeatedMegaBeamMiner2", "Enraged Guardian" },
        { "Deepnest_32", "killedMimicSpider", "Nosk" },
        { "Deepnest_East_Hornet", "hornetOutskirtsDefeated", "Hornet Sentinel" },
        { "Hive_05", "killedHiveKnight", "Hive Knight" },
        { "Cliffs_02", "killedGhostAladar", "Gorb" },
        { "RestingGrounds_02", "killedGhostXero", "Xero" },
        { "Grimm_Main_Tent", "killedGrimm", "Troupe Master Grimm" },
        { "GG_Lurker", "killedPaleLurker", "Pale Lurker" },
        { "Room_Final_Boss_Core", "killedHollowKnight", "The Hollow Knight" },
        { "Room_Colosseum_Bronze", nullptr, "Trial of the Warrior" },
        { "Room_Colosseum_Silver", nullptr, "Trial of the Conqueror" },
        { "Room_Colosseum_Gold", nullptr, "Trial of the Fool" },
        { "Dream_01_False_Knight", nullptr, "Failed Champion" },
        { "Dream_02_Mage_Lord", nullptr, "Soul Tyrant" },
        { "Dream_03_Infected_Knight", nullptr, "Lost Kin" },
        { "Dream_04_White_Defender", nullptr, "White Defender" },
        { "Dream_Mighty_Zote", nullptr, "Grey Prince Zote" },
        { "Dream_Final_Boss", nullptr, "The Radiance" },
        { "Grimm_Nightmare", nullptr, "Nightmare King Grimm" },
        { "GG_Broken_Vessel", nullptr, "Broken Vessel" },
        { "GG_Brooding_Mawlek", nullptr, "Brooding Mawlek" },
        { "GG_Collector", nullptr, "The Collector" },
        { "GG_Crystal_Guardian", nullptr, "Crystal Guardian" },
        { "GG_Crystal_Guardian_2", nullptr, "Enraged Guardian" },
        { "GG_Dung_Defender", nullptr, "Dung Defender" },
        { "GG_Failed_Champion", nullptr, "Failed Champion" },
        { "GG_False_Knight", nullptr, "False Knight" },
        { "GG_Flukemarm", nullptr, "Flukemarm" },
        { "GG_Ghost_Galien", nullptr, "Galien" },
        { "GG_Ghost_Gorb", nullptr, "Gorb" },
        { "GG_Ghost_Hu", nullptr, "Elder Hu" },
        { "GG_Ghost_Markoth", nullptr, "Markoth" },
        { "GG_Ghost_Marmu", nullptr, "Marmu" },
        { "GG_Ghost_No_Eyes", nullptr, "No Eyes" },
        { "GG_Ghost_Xero", nullptr, "Xero" },
        { "GG_God_Tamer", nullptr, "God Tamer" },
        { "GG_Grey_Prince_Zote", nullptr, "Grey Prince Zote" },
        { "GG_Grimm", nullptr, "Troupe Master Grimm" },
        { "GG_Grimm_Nightmare", nullptr, "Nightmare King Grimm" },
        { "GG_Gruz_Mother", nullptr, "Gruz Mother" },
        { "GG_Hive_Knight", nullptr, "Hive Knight" },
        { "GG_Hollow_Knight", nullptr, "Pure Vessel" },
        { "GG_Hornet_1", nullptr, "Hornet Protector" },
        { "GG_Hornet_2", nullptr, "Hornet Sentinel" },
        { "GG_Lost_Kin", nullptr, "Lost Kin" },
        { "GG_Mage_Knight", nullptr, "Soul Warrior" },
        { "GG_Mantis_Lords", nullptr, "Mantis Lords" },
        { "GG_Mantis_Lords_V", nullptr, "Sisters of Battle" },
        { "GG_Mega_Moss_Charger", nullptr, "Massive Moss Charger" },
        { "GG_Mighty_Zote", nullptr, "Zote" },
        { "GG_Nailmasters", nullptr, "Oro & Mato" },
        { "GG_Nosk", nullptr, "Nosk" },
        { "GG_Nosk_Hornet", nullptr, "Winged Nosk" },
        { "GG_Oblobbles", nullptr, "Oblobbles" },
        { "GG_Painter", nullptr, "Paintmaster Sheo" },
        { "GG_Radiance", nullptr, "Absolute Radiance" },
        { "GG_Sly", nullptr, "Great Nailsage Sly" },
        { "GG_Soul_Master", nullptr, "Soul Master" },
        { "GG_Soul_Tyrant", nullptr, "Soul Tyrant" },
        { "GG_Traitor_Lord", nullptr, "Traitor Lord" },
        { "GG_Uumuu", nullptr, "Uumuu" },
        { "GG_Vengefly", nullptr, "Vengefly King" },
        { "GG_Watcher_Knights", nullptr, "Watcher Knights" },
        { "GG_White_Defender", nullptr, "White Defender" },
    };
    return list;
}

const BossArena* FindBossArena(const std::string& scene)
{
    for (int pass = 0; pass < 2; ++pass) {
        std::string name = scene;
        if (pass == 1) {   // pantheon variant: GG_Gruz_Mother_V -> GG_Gruz_Mother
            if (name.size() < 5 || name.compare(0, 3, "GG_") != 0 || name.compare(name.size() - 2, 2, "_V") != 0)
                break;
            name.resize(name.size() - 2);
        }
        for (const BossArena& b : BossArenas())
            if (name == b.scene) return &b;
    }
    return nullptr;
}

int BenchAreaId(const std::string& respawnScene)
{
    for (const auto& p : kBenchPrefixes)
        if (respawnScene.compare(0, strlen(p.prefix), p.prefix) == 0) return p.id;
    return 0;
}

std::string BenchAreaName(int id)
{
    if (id <= 0 || id >= (int)std::size(kBenchAreas)) return {};
    return kBenchAreas[id];
}

const std::vector<std::string>& AllAreaNames()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (const char* z : kZones)
            if (*z && std::find(v.begin(), v.end(), z) == v.end()) v.push_back(z);
        return v;
    }();
    return list;
}

const std::vector<std::string>& CauseSuggestions()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v = { "Inimigo comum", "Espinhos / ácido (última máscara)", "Sonho" };
        for (const BossArena& b : BossArenas()) {
            std::string c = std::string("Chefe: ") + b.boss;
            if (std::find(v.begin(), v.end(), c) == v.end()) v.push_back(c);
        }
        return v;
    }();
    return list;
}

} // namespace hk
