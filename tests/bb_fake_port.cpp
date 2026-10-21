// Stand-in for the Bloodborne PC port, to test the counter's live reader without the game.
// Build output is named bbport.exe like the port. It maps the eboot image the way the port
// does (eboot address 0 at 0x800000000, or another address with --other-base), never runs
// any of it, and plays a short script with a fake GameDataMan:
//   0-4 s title screen (no character), then a character whose play time runs,
//   a death every 4 s (3 in total), then back to the title screen (play time stops).
//   build\tests\fake_port\bbport.exe "C:\...\Image0\eboot.bin" [--other-base]
#include "games/bloodborne.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: bbport <eboot.bin> [--other-base]\n");
        return 2;
    }
    bool otherBase = argc > 2 && wcscmp(argv[2], L"--other-base") == 0;
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> img;
    std::string err;
    bb::Layout lay;
    std::string log;
    if (!bb::FlatImageFromSelf(file, img, err) || !bb::AnalyzeImage(img, lay, log)) {
        fprintf(stderr, "eboot invalido: %s%s\n", err.c_str(), log.c_str());
        return 2;
    }
    file.clear();
    file.shrink_to_fit();

    // The real port runs this code, so its first page is executable; here it is never called.
    void* want = (void*)(otherBase ? 0x900000000ull : 0x800000000ull);
    auto* base = (uint8_t*)VirtualAlloc(want, img.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!base) { fprintf(stderr, "VirtualAlloc falhou (%lu)\n", GetLastError()); return 1; }
    memcpy(base, img.data(), img.size());
    DWORD old = 0;
    VirtualProtect(base, 0x1000, PAGE_EXECUTE_READ, &old);

    static uint8_t gdm[0x400], pgd[0x400];
    *(uint8_t**)(gdm + lay.offPlayerData) = pgd;
    *(uint8_t**)(base + lay.gameDataMan) = gdm;
    auto deaths = (int32_t*)(gdm + lay.offDeaths);
    auto trueDeaths = (int32_t*)(gdm + lay.offTrueDeaths);
    auto playTime = (uint32_t*)(gdm + lay.offPlayTime);
    printf("imagem em %p, GameDataMan em %p\n", (void*)base, (void*)gdm);
    fflush(stdout);

    ULONGLONG start = GetTickCount64();
    for (;;) {
        ULONGLONG t = GetTickCount64() - start;
        if (t >= 4000 && *playTime == 0) {   // character loaded from the save
            const wchar_t name[] = L"Caçador Teste";
            memcpy(pgd + lay.offName, name, sizeof(name));
            *(int32_t*)(pgd + lay.offSoulLevel) = 4;
            *(int32_t*)(gdm + lay.offClearCount) = 1;
            *deaths = 7;
            *trueDeaths = 7;
            *playTime = 985000;
            printf("personagem carregado\n");
            fflush(stdout);
        }
        if (t >= 4000 && t < 26000) *playTime += 16;   // in the world
        int due = t < 9000 ? 0 : (int)std::min<ULONGLONG>(3, (t - 9000) / 4000 + 1);
        if (*playTime && 7 + due > *deaths) {
            ++*deaths;
            ++*trueDeaths;
            printf("morte (%d)\n", *deaths);
            fflush(stdout);
        }
        if (t >= 32000) break;
        Sleep(16);
    }
    return 0;
}
