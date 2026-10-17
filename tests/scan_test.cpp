// Runs the counter's pattern scanner against a game executable on disk.
// Useful to check a new game version without launching it:
//   build\tests\scan_test.exe "C:\...\DarkSoulsRemastered.exe"
//   build\tests\scan_test.exe "C:\...\DarkSoulsII.exe"
// The game is picked from the file name (as the app does from the process name).
#include "game_reader.h"
#include "games/ds2.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: scan_test <DarkSoulsRemastered.exe | DarkSoulsII.exe>\n");
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (file.size() < 0x400) { fwprintf(stderr, L"arquivo invalido\n"); return 2; }

    const wchar_t* slash = wcsrchr(argv[1], L'\\');
    const wchar_t* fname = slash ? slash + 1 : argv[1];
    const GameInfo* game = &GameById("dsr");
    for (const GameInfo* g : AllGames())
        if (_wcsicmp(fname, g->exe) == 0) game = g;
    printf("Jogo: %s\n", game->name);

    // Map the PE sections the same way the Windows loader does.
    auto rd32 = [&](size_t at) { uint32_t v; memcpy(&v, &file[at], 4); return v; };
    auto rd16 = [&](size_t at) { uint16_t v; memcpy(&v, &file[at], 2); return v; };
    uint32_t pe = rd32(0x3C);
    uint16_t nsec = rd16(pe + 6), optSize = rd16(pe + 20);
    uint64_t imageBase;
    memcpy(&imageBase, &file[pe + 24 + 24], 8);
    uint32_t sizeOfImage = rd32(pe + 24 + 56);
    std::vector<uint8_t> img(sizeOfImage, 0);
    for (uint16_t i = 0; i < nsec; ++i) {
        size_t o = pe + 24 + optSize + i * 40;
        uint32_t vsize = rd32(o + 8), va = rd32(o + 12), rawSize = rd32(o + 16), rawPtr = rd32(o + 20);
        uint32_t n = vsize < rawSize ? vsize : rawSize;
        if (rawPtr + n <= file.size() && va + n <= img.size()) memcpy(&img[va], &file[rawPtr], n);
    }

    GameReader reader;
    reader.SetModuleForAnalysis((uintptr_t)imageBase, sizeOfImage, game->id);
    bool ok = reader.AnalyzeImage(img);
    printf("SizeOfImage: %X\n%s", sizeOfImage, reader.ScanLog().c_str());
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");

    if (game->id == std::string("dsr") && sizeOfImage == 0x4869400) {   // 1.01: compare with the addresses found by hand
        bool match = reader.GameManAddr() - imageBase == 0x1CEA4B8 &&
                     reader.GameDataManAddr() - imageBase == 0x1D00F50 &&
                     reader.WorldChrManAddr() - imageBase == 0x1CEE830;
        printf("Conferencia 1.01: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    if (game->id == std::string("ds2")) {
        // Every signature must be found (no "*" fallback) ...
        bool all = reader.ScanLog().find('*') == std::string::npos;
        printf("Todas as assinaturas encontradas: %s\n", all ? "OK" : "NAO");
        ok = ok && all;
        if (sizeOfImage == 0x20B6000) {   // Scholar of the First Sin 1.02: compare with the values found by hand
            const ds2::Layout& L = static_cast<ds2::Module*>(reader.module())->layout();
            bool match = L.gameManager - imageBase == 0x160B8D0 && L.loadState - imageBase == 0x186CCB0 &&
                         L.offPlayerCtrl == 0xD0 && L.offPlayerParam == 0x490 && L.offHp == 0x168 &&
                         L.offDeaths == 0x1A4 && L.offCounters == 0x104 &&
                         L.offGameData == 0xA8 && L.offCharInfo == 0xC0 && L.offName == 0x24 && L.offNgCycle == 0x68 &&
                         L.offSlotSummaries == 0xD8 && L.offSlotIndex == 0x1368 && L.summarySize == 0x1F0 &&
                         L.offTimeStamp == 0x1360 && L.offPlayTime == 0x1CC &&
                         L.offMapMan == 0x38 && L.offMapIndex == 0x170 && L.offPlaceBase == 0x178 && L.offPlaceDlc == 0x17C &&
                         L.offMapTable == 0x10 && L.offMapCount == 0x18 && L.offMapIds == 0x20 &&
                         L.offEventMan == 0x70 && L.offBonfireArea == 0x164 && L.offBonfireId == 0x16C;
            printf("Conferencia SotFS 1.02: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    }
    return ok ? 0 : 1;
}
