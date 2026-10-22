#include "games/sekiro_data.h"

// Map ids match the Sculptor's Idol event flags (1AABNNNN -> mAA_0B), e.g. the idols of
// Ashina Castle use 1111000x and those of Ashina Reservoir 1112000x.
std::string SekiroMapName(int area, int block)
{
    switch (area * 10 + block) {
        case 100: return "Hirata Estate";
        case 110: return "Ashina Outskirts";
        case 111: return "Ashina Castle";
        case 112: return "Ashina Reservoir";
        case 130: return "Abandoned Dungeon";
        case 150: return "Ashina Depths (Hidden Forest / Mibu Village)";
        case 170: return "Sunken Valley / Ashina Depths";
        case 200: return "Senpou Temple, Mt. Kongo";
        case 250: return "Fountainhead Palace";
    }
    return {};
}

std::string SekiroIdolArea(int id)
{
    if (id < 1000000 || id > 9999999) return {};
    return SekiroMapName(id / 100000, id / 10000 % 10);
}

const std::vector<std::string>& SekiroCauseSuggestions()
{
    static const std::vector<std::string> list = {
        "Queda", "Veneno", "Queimadura", "Terror", "Raio", "Inimigo comum", "Emboscada / grupo",
        "Chefe: Gyoubu Oniwa", "Chefe: Lady Butterfly", "Chefe: Genichiro Ashina", "Chefe: Folding Screen Monkeys",
        "Chefe: Guardian Ape", "Chefe: Headless Ape", "Chefe: Corrupted Monk", "Chefe: Great Shinobi Owl",
        "Chefe: True Corrupted Monk", "Chefe: Divine Dragon", "Chefe: Owl (Father)", "Chefe: Demon of Hatred",
        "Chefe: Isshin, the Sword Saint", "Chefe: Emma / Isshin Ashina", "Chefe: Genichiro, Way of Tomoe",
        "General Naomori Kawarada", "Chained Ogre", "General Tenzen Yamauchi", "Blazing Bull",
        "Ashina Elite - Jinsuke Saze", "Juzou the Drunkard", "Shinobi Hunter Enshin of Misen",
        "Lone Shadow Longswordsman", "Lone Shadow Masanaga the Spear-bearer", "Lone Shadow Vilehand",
        "Seven Ashina Spears - Shikibu Toshikatsu Yamauchi", "Seven Ashina Spears - Shume Masaji Oniwa",
        "Armored Warrior", "Long-arm Centipede Sen'un", "Long-arm Centipede Giraffe", "Snake Eyes Shirafuji",
        "Snake Eyes Shirahagi", "Sakura Bull of the Palace", "Headless", "Shichimen Warrior", "O'Rin of the Water",
        "Mist Noble", "Ashina Elite - Ujinari Mizou", "Tokujiro the Glutton", "Okami Leader Shizu",
        "Great Colored Carp", "Great Serpent",
    };
    return list;
}

const std::vector<std::string>& SekiroAllMapNames()
{
    static const std::vector<std::string> list = [] {
        std::vector<std::string> v;
        for (int area = 10; area <= 25; ++area)
            for (int block = 0; block <= 2; ++block) {
                std::string n = SekiroMapName(area, block);
                if (!n.empty()) v.push_back(n);
            }
        return v;
    }();
    return list;
}
