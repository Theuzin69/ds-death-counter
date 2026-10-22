// Checks Hollow Knight's death detection on recorded-style sequences of readings (what the
// reader sees ~10 times per second), plus the area / boss / bench tables.
//   build\tests\hk_death_test.exe
#include "games/hollowknight.h"
#include <algorithm>
#include <cstdio>
#include <string>

namespace {

int failures = 0;

void Check(bool ok, const char* what, long long got = 0)
{
    if (!ok) {
        printf("FALHOU: %s (%lld)\n", what, got);
        ++failures;
    }
}

struct Run {
    hk::DeathDetector d;
    uintptr_t save = 0x1000;
    // n readings in game, playing, with this health and dead flag
    void Play(int health, bool dead = false, int n = 3, bool playing = true)
    {
        for (int i = 0; i < n; ++i) d.Update({ true, playing, save, health, dead });
    }
    void Menu(int n = 3)
    {
        for (int i = 0; i < n; ++i) d.Update({ false, false, 0, -1, false });
    }
};

} // namespace

int main()
{
    {   // Normal death: masks run out, the Knight stays dead through the death animation and the
        // load back to the bench, then respawns with full health.
        Run r;
        r.Play(5); r.Play(3); r.Play(1); r.Play(0); r.Play(0, true, 20);
        r.Play(0, true, 10, false);   // loading screen
        r.Play(5);
        Check(r.d.count() == 1, "morte normal conta uma vez", r.d.count());
        r.Play(2); r.Play(0, true, 5);
        Check(r.d.count() == 2, "segunda morte", r.d.count());
    }
    {   // Dream / Godhome death: health hits 0 but the hero's dead state is never set.
        Run r;
        r.Play(4); r.Play(0, false, 30); r.Play(0, false, 5, false); r.Play(9);
        Check(r.d.count() == 1, "morte no sonho", r.d.count());
    }
    {   // Spikes and acid that only cost a mask are not deaths; on the last mask they are.
        Run r;
        r.Play(3); r.Play(2); r.Play(1);
        Check(r.d.count() == 0, "espinho sem morrer", r.d.count());
        r.Play(0, true, 10);
        Check(r.d.count() == 1, "espinho na ultima mascara", r.d.count());
    }
    {   // Steel Soul: one death, then the game ends (PermaDeath scene = menu).
        Run r;
        r.Play(5); r.Play(0, true, 20); r.Menu(30);
        Check(r.d.count() == 1, "Steel Soul", r.d.count());
    }
    {   // Quitting to the menu and loading again (the save was written while dying, so it loads
        // with health 0 until the respawn): no extra death.
        Run r;
        r.Play(5); r.Play(0, true, 10);
        r.Menu(20);
        r.save = 0x2000;
        r.Play(0, false, 5); r.Play(0, false, 5, false); r.Play(5);
        Check(r.d.count() == 1, "menu e carregar o save", r.d.count());
        r.Play(0, true, 5);
        Check(r.d.count() == 2, "morte depois de recarregar", r.d.count());
    }
    {   // Another save loaded without going through a reading in the menu.
        Run r;
        r.Play(4);
        r.save = 0x3000;
        r.Play(0, false, 3);
        Check(r.d.count() == 0, "troca de save com vida 0", r.d.count());
    }
    {   // The dead flag alone (health missed) also counts, once.
        Run r;
        r.Play(2); r.Play(2, true, 3); r.Play(0, true, 10); r.Play(6);
        Check(r.d.count() == 1, "so o estado dead", r.d.count());
    }
    {   // Unreadable health (-1) followed by 0 is not a death.
        Run r;
        r.Play(-1); r.Play(0);
        Check(r.d.count() == 0, "vida ilegivel", r.d.count());
    }
    {   // Health reaching 0 while not playing (loading) is not a death.
        Run r;
        r.Play(3, false, 3, false); r.Play(0, false, 3, false);
        Check(r.d.count() == 0, "vida 0 carregando", r.d.count());
    }

    // Tables
    Check(hk::MapZoneName(5) == "Forgotten Crossroads", "zona CROSSROADS");
    Check(hk::MapZoneName(51) == "Junk Pit", "ultima zona (GODSEEKER_WASTE)");
    Check(hk::MapZoneName(52).empty() && hk::MapZoneName(-1).empty(), "zona fora da tabela");
    Check(hk::IsDreamZone(17) && hk::IsDreamZone(50) && !hk::IsDreamZone(18), "zonas de sonho");
    const hk::BossArena* b = hk::FindBossArena("GG_Gruz_Mother_V");
    Check(b && std::string(b->boss) == "Gruz Mother" && !b->flag, "variante _V do panteao");
    b = hk::FindBossArena("GG_Mantis_Lords_V");
    Check(b && std::string(b->boss) == "Sisters of Battle", "Sisters of Battle");
    b = hk::FindBossArena("Crossroads_10");
    Check(b && b->flag && std::string(b->flag) == "falseKnightDefeated", "False Knight com flag");
    Check(!hk::FindBossArena("Crossroads_01") && !hk::FindBossArena("") && !hk::FindBossArena("_V"), "cena sem chefe");
    Check(hk::BenchAreaId("Crossroads_47") == 3, "banco Crossroads");
    Check(hk::BenchAreaId("Deepnest_East_06") == 11, "banco Kingdom's Edge");
    Check(hk::BenchAreaId("Room_Tram_RG") == 14, "banco Resting Grounds");
    Check(hk::BenchAreaId("Fungus3_archive") == 7 && hk::BenchAreaId("Fungus3_50") == 6, "Archives x Gardens");
    Check(hk::BenchAreaId("xyz") == 0 && hk::BenchAreaName(0).empty(), "banco desconhecido");
    const auto& causes = hk::CauseSuggestions();
    for (const hk::BossArena& a : hk::BossArenas())
        Check(std::find(causes.begin(), causes.end(), std::string("Chefe: ") + a.boss) != causes.end(), a.boss);

    printf("Resultado: %s\n", failures ? "FALHOU" : "OK");
    return failures ? 1 : 0;
}
