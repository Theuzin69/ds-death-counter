#include "games/ds3.h"

std::string Ds3MapName(int area, int block)
{
    switch (area * 10 + block) {
        case 300: return "High Wall of Lothric";
        case 301: return "Lothric Castle / Consumed King's Garden";
        case 310: return "Undead Settlement";
        case 320: return "Archdragon Peak";
        case 330: return "Road of Sacrifices / Farron Keep";
        case 341: return "Grand Archives";
        case 350: return "Cathedral of the Deep";
        case 370: return "Irithyll of the Boreal Valley / Anor Londo";
        case 380: return "Catacombs of Carthus / Smouldering Lake";
        case 390: return "Irithyll Dungeon / Profaned Capital";
        case 400: return "Firelink Shrine / Cemetery of Ash / Untended Graves";
        case 410: return "Kiln of the First Flame";
        case 450: return "Painted World of Ariandel";
        case 460: return "Arena: Grand Roof";
        case 470: return "Arena: Kiln of Flame";
        case 500: return "The Dreg Heap";
        case 510: return "The Ringed City";
        case 511: return "Filianore's Rest";
        case 530: return "Arena: Dragon Ruins";
        case 540: return "Arena: Round Plaza";
    }
    return {};
}

std::string Ds3BonfireName(int id)
{
    // Bonfire and respawn point entity ids are AABxxxx (map mAA_BB, e.g. 4002950 in m40_00).
    if (id <= 0) return {};
    std::string map = Ds3MapName(id / 100000 % 100, id / 10000 % 10);
    std::string num = "#" + std::to_string(id);
    return map.empty() ? num : map + " (" + num + ")";
}

const std::vector<std::string>& Ds3CauseSuggestions()
{
    static const std::vector<std::string> list = {
        "Queda", "Armadilha", "Veneno / Toxina", "Sangramento", "Congelamento", "Maldição", "Invasor (PvP)",
        "Inimigo comum",
        "Chefe: Iudex Gundyr", "Chefe: Vordt of the Boreal Valley", "Chefe: Curse-rotted Greatwood",
        "Chefe: Crystal Sage", "Chefe: Deacons of the Deep", "Chefe: Abyss Watchers", "Chefe: High Lord Wolnir",
        "Chefe: Old Demon King", "Chefe: Pontiff Sulyvahn", "Chefe: Yhorm the Giant",
        "Chefe: Aldrich, Devourer of Gods", "Chefe: Dancer of the Boreal Valley", "Chefe: Dragonslayer Armour",
        "Chefe: Oceiros, the Consumed King", "Chefe: Champion Gundyr", "Chefe: Ancient Wyvern",
        "Chefe: Nameless King", "Chefe: Lorian & Lothric (Twin Princes)", "Chefe: Soul of Cinder",
        "Chefe: Sister Friede", "Chefe: Champion's Gravetender", "Chefe: Demon Prince",
        "Chefe: Halflight, Spear of the Church", "Chefe: Darkeater Midir", "Chefe: Slave Knight Gael",
        "Lothric Knight", "Winged Knight", "Black Knight", "Mimic", "Pus of Man", "Boreal Outrider Knight",
        "Silver Knight", "Jailer", "Darkwraith", "Ringed Knight", "Harald Legion Knight", "Basilisk", "Rat",
    };
    return list;
}

const std::vector<std::string>& Ds3AllMapNames()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (int area = 30; area <= 54; ++area)
            for (int block = 0; block <= 1; ++block) {
                std::string n = Ds3MapName(area, block);
                if (!n.empty()) v.push_back(n);
            }
        return v;
    }();
    return list;
}
