// Runs the counter's pattern scanner against a DarkSoulsRemastered.exe on disk.
// Useful to check a new game version without launching it:
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
        fwprintf(stderr, L"uso: scan_test <DarkSoulsRemastered.exe>\n");
        return 2;
    }
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
    reader.SetModuleForAnalysis((uintptr_t)imageBase, sizeOfImage);
    bool ok = reader.AnalyzeImage(img);
    printf("SizeOfImage: %X\n%s", sizeOfImage, reader.ScanLog().c_str());
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");

    if (sizeOfImage == 0x4869400) {   // 1.01: compare with the addresses found by hand
        bool match = reader.GameManAddr() - imageBase == 0x1CEA4B8 &&
                     reader.GameDataManAddr() - imageBase == 0x1D00F50 &&
                     reader.WorldChrManAddr() - imageBase == 0x1CEE830;
        printf("Conferencia 1.01: %s\n", match ? "OK" : "DIFERENTE");
        // Boss health bars: MenuMan (RTTI NS_FRPG::MenuMan) + 0x1050, two slots of 0x14 bytes.
        bool bossMatch = reader.MenuManAddr() - imageBase == 0x1CFF7C8 &&
                         reader.BossGaugeOffset() == 0x1050 && reader.BossGaugeStride() == 0x14;
        printf("Conferencia 1.01 (barra de chefe): %s\n", bossMatch ? "OK" : "DIFERENTE");
        ok = ok && match && bossMatch;
    }
    return ok ? 0 : 1;
}
