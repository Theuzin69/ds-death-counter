// Polls GameReader like the app does while Hollow Knight runs and prints every change, to check
// the live reader without the app (and without touching data.json). Read-only, like the app.
//   build\tests\hk_live_test.exe [seconds]
#include "game_reader.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    int seconds = argc > 1 ? _wtoi(argv[1]) : 60;
    GameReader reader;
    std::string last;
    bool shownDiag = false;
    ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < (ULONGLONG)seconds * 1000) {
        reader.Update();
        GameSnapshot s;
        bool ok = reader.Read(s);
        char b[768];
        std::string name;
        for (wchar_t c : s.name) name += c < 0x80 ? (char)c : '?';
        snprintf(b, sizeof(b), "estado %d | jogo %s | leu %d | dados %d | save %d | no mundo %d | mortes %d | '%s' slot %d | "
                 "vida %d | area '%s' | banco %d | causa '%s' | tempo %us",
                 (int)reader.state(), reader.GameId().c_str(), ok, s.gameDataOk, s.charLoaded, s.inWorld, s.deaths,
                 name.c_str(), s.saveSlot, s.hp, s.areaName.c_str(), s.lastBonfire, s.cause.c_str(), s.playTimeMs / 1000);
        if (b != last) {
            printf("%6.1fs  %s\n", (GetTickCount64() - start) / 1000.0, b);
            fflush(stdout);
            last = b;
        }
        if (ok && !shownDiag) {
            printf("\n%s\n", reader.DiagnosticsText().c_str());
            shownDiag = true;
        }
        Sleep(100);
    }
    printf("\n%s", reader.DiagnosticsText().c_str());
    return 0;
}
