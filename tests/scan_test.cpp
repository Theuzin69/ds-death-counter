// Runs the counter's pattern scanner against a game exe on disk (DarkSoulsRemastered.exe or
// DarkSoulsIII.exe, recognized by the file name). Useful to check a new game version without launching it:
//   build\tests\scan_test.exe "C:\...\DarkSoulsRemastered.exe"
#include "game_reader.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: scan_test <DarkSoulsRemastered.exe | DarkSoulsIII.exe>\n");
        return 2;
    }
    const GameInfo* game = FindGameByExe(argv[1]);
    if (!game) { fwprintf(stderr, L"nome de executavel desconhecido: %s\n", argv[1]); return 2; }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (file.size() < 0x400) { fwprintf(stderr, L"arquivo invalido\n"); return 2; }

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
    reader.SetModuleForAnalysis((uintptr_t)imageBase, sizeOfImage, game);
    bool ok = reader.AnalyzeImage(img);
    printf("Jogo: %s\nSizeOfImage: %X\n%s", game->title, sizeOfImage, reader.ScanLog().c_str());
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");

    if (game->id == std::string("dsr") && sizeOfImage == 0x4869400) {   // 1.01: compare with the addresses found by hand
        bool match = reader.GameManAddr() - imageBase == 0x1CEA4B8 &&
                     reader.GameDataManAddr() - imageBase == 0x1D00F50 &&
                     reader.WorldChrManAddr() - imageBase == 0x1CEE830;
        printf("Conferencia 1.01: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    if (game->id == std::string("ds3")) {
        // Every DS3 signature must match exactly once (and agree with the others for the same global).
        const Ds3Reader& d = reader.Ds3();
        bool all = d.signaturesFound == d.signaturesTotal && d.gameMan != 0;
        printf("Assinaturas DS3: %d/%d %s\n", d.signaturesFound, d.signaturesTotal, all ? "OK" : "FALTANDO");
        ok = ok && all;
        if (sizeOfImage == 0x63DF000) {   // 1.15: compare with the addresses and offsets found by hand
            const auto& o = d.off;
            bool match = d.gameDataMan - imageBase == 0x4740178 && d.worldChrMan - imageBase == 0x4768E78 &&
                         d.gameMan - imageBase == 0x4743AB0 && o.deaths == 0x98 && o.playTime == 0xA4 &&
                         o.playerIns == 0x80 && o.saveSlot == 0xA60 && o.respawn == 0xACC && o.modules == 0x1F90 &&
                         o.dataModule == 0x18 && o.hp == 0xD8 && o.playRegion == 0x1ABC;
            printf("Conferencia 1.15: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    }
    return ok ? 0 : 1;
}
