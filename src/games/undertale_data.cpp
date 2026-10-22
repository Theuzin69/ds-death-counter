#include "games/undertale.h"
#include <cstring>
#include <iterator>

// Tables taken from Undertale 1.08's data.win (rooms, scr_roomname, scr_battlegroup and
// scr_monstersetup with the English text).

namespace {

// Room names in the room order of data.win (Undertale 1.08); the room global holds an index into it.
const char* const kRooms[] = {
    "room_start", "room_introstory", "room_introimage", "room_intromenu", "room_area1", "room_area1_2",
    "room_ruins1", "room_ruins2", "room_ruins3", "room_ruins4", "room_ruins5", "room_ruins6", "room_ruins7",
    "room_ruins7A", "room_ruins8", "room_ruins9", "room_ruins10", "room_ruins11", "room_ruins12A",
    "room_ruins12", "room_ruins12B", "room_ruins13", "room_ruins14", "room_ruins15A", "room_ruins15B",
    "room_ruins15C", "room_ruins15D", "room_ruins15E", "room_ruins16", "room_ruins17", "room_ruins18OLD",
    "room_ruins19", "room_torhouse1", "room_torhouse2", "room_torhouse3", "room_torielroom",
    "room_asrielroom", "room_kitchen", "room_basement1", "room_basement2", "room_basement3", "room_basement4",
    "room_basement5", "room_ruinsexit", "room_tundra1", "room_tundra2", "room_tundra3", "room_tundra3A",
    "room_tundra4", "room_tundra5", "room_tundra6", "room_tundra6A", "room_tundra7", "room_tundra8",
    "room_tundra8A", "room_tundra9", "room_tundra_spaghetti", "room_tundra_snowpuzz", "room_tundra_xoxosmall",
    "room_tundra_xoxopuzz", "room_tundra_randoblock", "room_tundra_lesserdog", "room_tundra_icehole",
    "room_tundra_iceentrance", "room_tundra_iceexit_new", "room_tundra_iceexit", "room_tundra_poffzone",
    "room_tundra_dangerbridge", "room_tundra_town", "room_tundra_town2", "room_tundra_dock",
    "room_tundra_inn", "room_tundra_inn_2f", "room_tundra_grillby", "room_tundra_library",
    "room_tundra_garage", "room_tundra_sanshouse", "room_tundra_paproom", "room_tundra_sansroom",
    "room_tundra_sansroom_dark", "room_tundra_sansbasement", "room_fogroom", "room_water1", "room_water2",
    "room_water3", "room_water3A", "room_water4", "room_water_bridgepuzz1", "room_water5", "room_water5A",
    "room_water6", "room_water7", "room_water8", "room_water9", "room_water_savepoint1", "room_water11",
    "room_water_nicecream", "room_water12", "room_water_shoe", "room_water_bird", "room_water_onionsan",
    "room_water14", "room_water_piano", "room_water_dogroom", "room_water_statue", "room_water_prewaterfall",
    "room_water_waterfall", "room_water_waterfall2", "room_water_waterfall3", "room_water_waterfall4",
    "room_water_preundyne", "room_water_undynebridge", "room_water_undynebridgeend", "room_water_trashzone1",
    "room_water_trashsavepoint", "room_water_trashzone2", "room_water_friendlyhub", "room_water_undyneyard",
    "room_water_undynehouse", "room_water_blookyard", "room_water_blookhouse", "room_water_hapstablook",
    "room_water_farm", "room_water_prebird", "room_water_shop", "room_water_dock", "room_water15",
    "room_water16", "room_water_temvillage", "room_water17", "room_water18", "room_water19", "room_water20",
    "room_water21", "room_water_undynefinal", "room_water_undynefinal2", "room_water_undynefinal3",
    "room_fire1", "room_fire2", "room_fire_prelab", "room_fire_dock", "room_fire_lab1", "room_fire_lab2",
    "room_fire3", "room_fire5", "room_fire6", "room_fire6A", "room_fire_lasers1", "room_fire7", "room_fire8",
    "room_fire_shootguy_2", "room_fire9", "room_fire_shootguy_1", "room_fire_turn", "room_fire_cookingshow",
    "room_fire_savepoint1", "room_fire_elevator_r1", "room_fire_elevator_r2", "room_fire_hotdog",
    "room_fire_walkandbranch", "room_fire_sorry", "room_fire_apron", "room_fire10", "room_fire_rpuzzle",
    "room_fire_mewmew2", "room_fire_boysnightout", "room_fire_newsreport", "room_fire_coreview2",
    "room_fire_elevator_l2", "room_fire_elevator_l3", "room_fire_spidershop", "room_fire_walkandbranch2",
    "room_fire_conveyorlaser", "room_fire_shootguy_3", "room_fire_preshootguy4", "room_fire_shootguy_4",
    "room_fire_savepoint2", "room_fire_spider", "room_fire_pacing", "room_fire_operatest",
    "room_fire_multitile", "room_fire_hotelfront_1", "room_fire_hotelfront_2", "room_fire_hotellobby",
    "room_fire_restaurant", "room_fire_hoteldoors", "room_fire_hotelbed", "room_fire_elevator_r3",
    "room_fire_precore", "room_fire_core1", "room_fire_core2", "room_fire_core3", "room_fire_core4",
    "room_fire_core5", "room_fire_core_freebattle", "room_fire_core_laserfun", "room_fire_core_branch",
    "room_fire_core_bottomleft", "room_fire_core_left", "room_fire_core_topleft", "room_fire_core_top",
    "room_fire_core_topright", "room_fire_core_right", "room_fire_core_bottomright", "room_fire_core_center",
    "room_fire_shootguy_5", "room_fire_core_treasureleft", "room_fire_core_treasureright",
    "room_fire_core_warrior", "room_fire_core_bridge", "room_fire_core_premett", "room_fire_core_metttest",
    "room_fire_core_final", "room_fire_elevator", "room_fire_elevator_l1", "room_fire_finalelevator",
    "room_castle_elevatorout", "room_castle_precastle", "room_castle_hook", "room_castle_front",
    "room_asghouse1", "room_asghouse2", "room_asghouse3", "room_asgoreroom", "room_asrielroom_final",
    "room_kitchen_final", "room_basement1_final", "room_basement2_final", "room_basement3_final",
    "room_basement4_final", "room_lastruins_corridor", "room_sanscorridor", "room_castle_finalshoehorn",
    "room_castle_coffins1", "room_castle_coffins2", "room_castle_throneroom", "room_castle_prebarrier",
    "room_castle_barrier", "room_castle_exit", "room_undertale_end", "room_castle_trueexit",
    "room_outsideworld", "room_fire_labelevator", "room_truelab_elevatorinside", "room_truelab_elevator",
    "room_truelab_hall1", "room_truelab_hub", "room_truelab_hall2", "room_truelab_operatingroom",
    "room_truelab_redlever", "room_truelab_prebed", "room_truelab_bedroom", "room_truelab_mirror",
    "room_truelab_bluelever", "room_truelab_hall3", "room_truelab_shower", "room_truelab_determination",
    "room_truelab_tv", "room_truelab_cooler", "room_truelab_greenlever", "room_truelab_fan",
    "room_truelab_castle_elevator", "room_truelab_prepower", "room_truelab_power", "room_gaster",
    "room_icecave1", "room_ice_dog", "room2", "room_water_fakehallway", "room_mysteryman", "room_soundtest",
    "TESTROOM", "room_water_redacted", "room_water13", "room_overworld", "room_overworld3", "bullettest",
    "room_water16A", "room_end_castroll", "room_end_highway", "room_end_beach", "room_end_metta",
    "room_end_school", "room_end_mtebott", "room_creditsdodger", "room_end_myroom", "room_end_theend",
    "room_spritecheck", "room_settings", "room_controltest", "room_f_start", "room_f_intro", "room_f_menu",
    "room_f_room", "room_floweyx", "room_f_phrase", "room_fire4", "room_fire10_old", "room_fire10A_old",
    "room_tundra_placeholder", "room_ruins12B_old", "room_tundra_rollsnow", "room_water7_older",
    "room_meetundyne_old", "room_water_mushroom", "room_monsteralign_test", "room_battle",
    "room_floweybattle", "room_fastbattle", "room_storybattle", "room_gameover", "room_shop1", "room_shop2",
    "room_shop3", "room_shop4", "room_shop5", "room_riverman_transition", "room_papdate", "room_adate",
    "room_flowey_endchoice", "room_flowey_regret", "room_empty", "room_emptywhite", "room_emptyblack",
    "room_nothingness", "room_undertale", "room_of_dog", "room_quizholder", "room_friendtest",
    "room_bringitinguys", "room_asrielappears", "room_goodbyeasriel", "room_asrielmemory", "room_asrieltest",
    "room_afinaltest", "room_dogshrine",
};
// Save point names shown by the game (scr_roomname, English text).
const struct { int room; const char* name; } kSaveRooms[] = {
    { 6, "Ruins - Entrance" },   // room_ruins1
    { 12, "Ruins - Leaf Pile" },   // room_ruins7
    { 18, "Ruins - Mouse Hole" },   // room_ruins12A
    { 31, "Ruins - Home" },   // room_ruins19
    { 44, "Snowdin - Ruins Exit" },   // room_tundra1
    { 46, "Snowdin - Box Road" },   // room_tundra3
    { 56, "Snowdin - Spaghetti" },   // room_tundra_spaghetti
    { 61, "Snowdin - Dog House" },   // room_tundra_lesserdog
    { 68, "Snowdin - Town" },   // room_tundra_town
    { 83, "Waterfall - Checkpoint" },   // room_water2
    { 86, "Waterfall - Hallway" },   // room_water4
    { 94, "Waterfall - Crystal" },   // room_water_savepoint1
    { 110, "Waterfall - Bridge" },   // room_water_preundyne
    { 114, "Waterfall - Trash Zone" },   // room_water_trashsavepoint
    { 116, "Waterfall - Quiet Area" },   // room_water_friendlyhub
    { 128, "Waterfall - Temmie Village" },   // room_water_temvillage
    { 134, "Waterfall - Undyne Arena" },   // room_water_undynefinal
    { 139, "Hotland - Laboratory Entrance" },   // room_fire_prelab
    { 145, "Hotland - Magma Chamber" },   // room_fire6
    { 155, "Hotland - Core View" },   // room_fire_savepoint1
    { 164, "Hotland - Bad Opinion Zone" },   // room_fire_mewmew2
    { 176, "Hotland - Spider Entrance" },   // room_fire_savepoint2
    { 183, "Hotland - Hotel Lobby" },   // room_fire_hotellobby
    { 196, "Hotland - Core Branch" },   // room_fire_core_branch
    { 210, "Hotland - Core End" },   // room_fire_core_premett
    { 216, "Castle Elevator" },   // room_castle_elevatorout
    { 219, "New Home" },   // room_castle_front
    { 231, "Last Corridor" },   // room_sanscorridor
    { 232, "Throne Entrance" },   // room_castle_finalshoehorn
    { 235, "Throne Room" },   // room_castle_throneroom
    { 236, "The End" },   // room_castle_prebarrier
    { 246, "True Laboratory" },   // room_truelab_hub
    { 251, "True Lab - Bedroom" },   // room_truelab_bedroom
};

// Enemies of each battle group (global.battlegroup), "|" between enemies of the same battle.
const struct { int group; const char* names; } kGroups[] = {
    { 1, "Froggit" }, { 2, "Dummy" }, { 3, "Froggit" }, { 4, "Froggit" }, { 5, "Whimsun" },
    { 6, "Froggit|Whimsun" }, { 7, "Moldsmal" }, { 8, "Moldsmal" }, { 9, "Froggit" }, { 10, "Moldsmal" },
    { 11, "Moldsmal|Migosp" }, { 12, "Migosp|Vegetoid" }, { 13, "Loox" }, { 14, "Loox|Vegetoid" },
    { 15, "Loox|Vegetoid|Migosp" }, { 16, "Vegetoid" }, { 17, "Loox" }, { 18, "Vegetoid" },
    { 20, "Napstablook" }, { 21, "Loox" }, { 22, "Toriel" }, { 23, "Doggo" }, { 24, "Lesser Dog" },
    { 25, "Dogamy|Dogaressa" }, { 26, "Greater Dog" }, { 27, "Papyrus" }, { 28, "Gyftrot" },
    { 30, "Snowdrake" }, { 31, "Snowdrake" }, { 32, "Ice Cap" }, { 33, "Ice Cap|Snowdrake" }, { 34, "Jerry" },
    { 35, "Ice Cap|Jerry" }, { 36, "Ice Cap|Jerry|Snowdrake" }, { 40, "Aaron" }, { 41, "Temmie" },
    { 42, "Moldsmal" }, { 43, "Woshua" }, { 44, "Shyren" }, { 45, "Mad Dummy" }, { 46, "Aaron|Woshua" },
    { 47, "Undyne" }, { 48, "Mettaton" }, { 49, "RG 01|RG 02" }, { 50, "Tsunderplane" }, { 51, "Vulkin" },
    { 52, "Pyrope" }, { 53, "Moldsmal" }, { 54, "Woshua|Aaron" }, { 55, "Woshua|Moldsmal" }, { 56, "Muffet" },
    { 57, "Mettaton" }, { 58, "Undyne" }, { 59, "Madjick" }, { 60, "Knight Knight" }, { 61, "Final Froggit" },
    { 62, "Astigmatism" }, { 63, "Whimsalot" }, { 64, "Whimsalot|Final Froggit" },
    { 65, "Whimsalot|Astigmatism" }, { 66, "Final Froggit|Astigmatism" },
    { 67, "Final Froggit|Astigmatism|Whimsalot" }, { 68, "Knight Knight|Madjick" },
    { 69, "Bomba (Mettaton)" }, { 70, "Bomba (Mettaton)" }, { 71, "Bomba (Mettaton)" },
    { 72, "Bomba (Mettaton)" }, { 73, "Bomba (Mettaton)" }, { 74, "Bomba (Mettaton)" },
    { 75, "Bomba (Mettaton)" }, { 76, "RG 04|RG 03" }, { 77, "Tsunderplane|Vulkin" }, { 78, "Pyrope" },
    { 79, "Vulkin" }, { 80, "Mettaton" }, { 81, "Mettaton EX" }, { 82, "Lemon Bread" }, { 83, "Reaper Bird" },
    { 84, "Snowdrake's Mother" }, { 85, "Memoryhead" }, { 86, "Endogeny" }, { 87, "Asriel Dreemurr" },
    { 88, "Asriel Dreemurr" }, { 89, "Asriel Dreemurr" }, { 90, "Asriel Dreemurr" }, { 91, "Monster Kid" },
    { 92, "Undyne the Undying" }, { 93, "Glad Dummy" }, { 94, "Mettaton NEO" }, { 95, "Sans" },
    { 100, "Asgore" }, { 101, "Asgore" }, { 120, "Final Froggit|Astigmatism" },
    { 121, "Final Froggit|Migospel" }, { 122, "Parsnik" }, { 123, "Moldessa" }, { 124, "Moldessa" },
    { 125, "Final Froggit|Whimsalot" }, { 126, "Final Froggit" }, { 127, "Whimsalot|Parsnik" },
    { 128, "Moldessa|Migospel" }, { 129, "Parsnik|Migospel" }, { 130, "Parsnik" },
    { 131, "Parsnik|Astigmatism" }, { 132, "Astigmatism" }, { 133, "Astigmatism|Migospel|Moldessa" },
    { 134, "Astigmatism|Parsnik|Moldessa" }, { 135, "Glyde" }, { 140, "So Sorry" },
    { 255, "Asriel Dreemurr" }, { 256, "Asriel Dreemurr" },
};

const char* const kBosses[] = {
    "Toriel", "Papyrus", "Undyne", "Undyne the Undying", "Mettaton", "Mettaton EX", "Mettaton NEO", "Muffet",
    "Asgore", "Sans", "Napstablook", "Mad Dummy", "Glad Dummy", "Asriel Dreemurr",
};

bool StartsWith(const std::string& s, const char* prefix) { return s.compare(0, strlen(prefix), prefix) == 0; }

bool IsBoss(const std::string& name)
{
    for (const char* b : kBosses)
        if (name == b) return true;
    return false;
}

} // namespace

namespace ut {

RoomKind KindOfRoom(const std::string& r)
{
    if (r == "room_gameover") return RoomKind::GameOver;
    if (r == "room_battle" || r == "room_floweybattle" || r == "room_fastbattle" || r == "room_storybattle")
        return RoomKind::Battle;
    // Title screen, intro, naming, settings, Flowey's file screens after Genocide, credits.
    if (r.empty() || r == "room_start" || r == "room_introstory" || r == "room_introimage" || r == "room_intromenu" ||
        r == "room_settings" || r == "room_controltest" || r == "room_nothingness" || r == "room_undertale_end" ||
        StartsWith(r, "room_f_") || StartsWith(r, "room_end_") || StartsWith(r, "room_empty"))
        return RoomKind::Menu;
    return RoomKind::World;
}

std::string AreaOfRoom(const std::string& r)
{
    if (StartsWith(r, "room_fire_core") || r == "room_fire_finalelevator") return "Core";
    if (r == "room_fire_labelevator" || StartsWith(r, "room_truelab")) return "True Lab";
    if (StartsWith(r, "room_fire") || r == "room_shop4" || r == "room_shop5") return "Hotland";
    if (StartsWith(r, "room_water") || r == "room_shop2" || r == "room_shop3" || r == "room_riverman_transition")
        return "Waterfall";
    if (StartsWith(r, "room_tundra") || r == "room_fogroom" || StartsWith(r, "room_ice") || r == "room_shop1" ||
        r == "room_papdate")
        return "Snowdin";
    // New Home copies the Ruins' house: its rooms end in "_final".
    if (StartsWith(r, "room_castle") || StartsWith(r, "room_asghouse") || r == "room_asgoreroom" ||
        r == "room_lastruins_corridor" || r == "room_sanscorridor" ||
        (r.size() > 6 && r.compare(r.size() - 6, 6, "_final") == 0))
        return "New Home";
    if (StartsWith(r, "room_ruins") || StartsWith(r, "room_area1") || StartsWith(r, "room_torhouse") ||
        r == "room_torielroom" || r == "room_asrielroom" || r == "room_kitchen" || StartsWith(r, "room_basement"))
        return "Ruins";
    if (r == "room_outsideworld") return "Surface";
    return {};
}

std::string BattleGroupEnemies(int group)
{
    for (const auto& g : kGroups) {
        if (g.group != group) continue;
        // "Froggit|Whimsun" -> "Froggit e Whimsun"; a boss alone -> "Chefe: Toriel".
        std::vector<std::string> names;
        for (const char* p = g.names; *p;) {
            const char* bar = strchr(p, '|');
            names.emplace_back(p, bar ? bar - p : strlen(p));
            p = bar ? bar + 1 : p + strlen(p);
        }
        if (names.size() == 1 && IsBoss(names[0])) return "Chefe: " + names[0];
        std::string t;
        for (size_t i = 0; i < names.size(); ++i) {
            if (i > 0) t += (i + 1 == names.size()) ? " e " : ", ";
            t += names[i];
        }
        return t;
    }
    return {};
}

std::string RoomName(int room)
{
    return room >= 0 && room < (int)std::size(kRooms) ? kRooms[room] : std::string();
}

std::string SaveRoomLabel(int room)
{
    for (const auto& s : kSaveRooms)
        if (s.room == room) return s.name;
    return AreaOfRoom(RoomName(room));
}

const std::vector<std::string>& AllAreaNames()
{
    static const std::vector<std::string> list = {
        "Ruins", "Snowdin", "Waterfall", "Hotland", "Core", "New Home", "True Lab",
    };
    return list;
}

const std::vector<std::string>& CauseSuggestions()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (const char* b : kBosses) v.push_back(std::string("Chefe: ") + b);
        for (const char* e : { "Froggit", "Whimsun", "Moldsmal", "Migosp", "Vegetoid", "Loox", "Doggo", "Lesser Dog",
                               "Dogamy e Dogaressa", "Greater Dog", "Snowdrake", "Ice Cap", "Gyftrot", "Jerry",
                               "Aaron", "Woshua", "Shyren", "Temmie", "RG 01 e RG 02", "Vulkin", "Tsunderplane",
                               "Pyrope", "RG 03 e RG 04", "Madjick", "Knight Knight", "Final Froggit", "Whimsalot",
                               "Astigmatism", "Amalgamate", "So Sorry", "Glyde" })
            v.push_back(e);
        return v;
    }();
    return list;
}

} // namespace ut
