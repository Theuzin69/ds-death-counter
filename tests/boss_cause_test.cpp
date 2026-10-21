// Checks the boss health bar -> cause of death table:
// every boss gives a cause that is also one of the suggestions (so the stats group
// automatic and hand-typed causes together), and multi-bar fights give one cause.
//   build\tests\boss_cause_test.exe
#include "ds1_data.h"
#include <algorithm>
#include <cstdio>

int main()
{
    // Name ids of every boss bar in the game's event scripts (event/*.emevd, instruction 2003:11).
    const int ids[] = { 2232, 2231, 2250, 5350, 2240, 5260, 3230, 5210, 5280, 2320, 5270, 2360, 5320, 2730,
                        3320, 5220, 5290, 5390, 5250, 2230, 5200, 5400, 5370, 3471, 4100, 4510, 4500 };
    const auto& suggestions = Ds1CauseSuggestions();
    int failures = 0;
    auto check = [&](bool ok, const char* what, const std::string& got) {
        if (!ok) { printf("FALHOU: %s (%s)\n", what, got.c_str()); ++failures; }
    };

    for (int id : ids) {
        int bars[2] = { id, -1 };
        std::string cause = Ds1BossCause(bars, 2);
        check(!Ds1BossBarName(id).empty(), "nome da barra", std::to_string(id));
        check(std::find(suggestions.begin(), suggestions.end(), cause) != suggestions.end(), "causa fora das sugestoes", cause);
    }

    int ons[2] = { 2360, 5270 };       // Smough (slot 0) + Ornstein (slot 1)
    check(Ds1BossCause(ons, 2) == "Chefe: Ornstein & Smough", "Ornstein & Smough", Ds1BossCause(ons, 2));
    int gargoyles[2] = { 5350, 5350 };
    check(Ds1BossCause(gargoyles, 2) == "Chefe: Bell Gargoyles", "Bell Gargoyles", Ds1BossCause(gargoyles, 2));
    int slot1Only[2] = { -1, 5270 };
    check(Ds1BossCause(slot1Only, 2) == "Chefe: Ornstein & Smough", "so o slot 1", Ds1BossCause(slot1Only, 2));
    int none[2] = { -1, -1 };
    check(Ds1BossCause(none, 2).empty(), "sem barra", Ds1BossCause(none, 2));
    int unknown[2] = { 2390, -1 };
    check(Ds1BossCause(unknown, 2) == "Chefe: id 2390", "id desconhecido", Ds1BossCause(unknown, 2));

    printf("Resultado: %s\n", failures ? "FALHOU" : "OK");
    return failures ? 1 : 0;
}
