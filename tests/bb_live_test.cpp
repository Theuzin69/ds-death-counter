// Polls GameReader like the app does and prints what it sees, to check the live Bloodborne
// reader against bb_fake_port (or the real port). Read-only, like the app.
//   build\tests\bb_live_test.exe [seconds]
#include "game_reader.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    int seconds = argc > 1 ? _wtoi(argv[1]) : 30;
    GameReader reader;
    std::string last;
    bool shownDiag = false;
    ULONGLONG start = GetTickCount64();
    while (GetTickCount64() - start < (ULONGLONG)seconds * 1000) {
        reader.Update();
        GameSnapshot s;
        bool ok = reader.Read(s);
        char b[512];
        std::string name;
        for (wchar_t c : s.name) name += c < 0x80 ? (char)c : '?';
        snprintf(b, sizeof(b), "estado %d | jogo %s | leu %d | dados %d | personagem %d | no mundo %d | mortes %d | nome '%s' | nivel %d | NG %d | tempo %s",
                 (int)reader.state(), reader.GameId().c_str(), ok, s.gameDataOk, s.charLoaded, s.inWorld, s.deaths,
                 name.c_str(), s.soulLevel, s.ngPlus, s.playTimeMs ? ">0" : "0");
        if (b != last) {
            printf("%6.1fs  %s\n", (GetTickCount64() - start) / 1000.0, b);
            fflush(stdout);
            last = b;
        }
        if (s.charLoaded && !shownDiag) {
            printf("\n%s\n", reader.DiagnosticsText().c_str());
            shownDiag = true;
        }
        Sleep(100);
    }
    printf("\n%s", reader.DiagnosticsText().c_str());
    return 0;
}
