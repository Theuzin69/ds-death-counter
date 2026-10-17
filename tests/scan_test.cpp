// Runs the counter's pattern scanner against a game exe on disk (DarkSoulsRemastered.exe or sekiro.exe).
// Useful to check a new game version without launching it:
//   build\tests\scan_test.exe "C:\...\DarkSoulsRemastered.exe"
// The game is chosen by the file name, like the counter does with the running process.
#include "game_reader.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: scan_test <DarkSoulsRemastered.exe | sekiro.exe>\n");
        return 2;
    }
    const wchar_t* fileName = argv[1];
    for (const wchar_t* c = argv[1]; *c; ++c)
        if (*c == L'\\' || *c == L'/') fileName = c + 1;
    const char* game = GameIdFromExeName(fileName);
    if (!game) {
        fwprintf(stderr, L"%ls nao e um jogo suportado\n", fileName);
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
    reader.SetModuleForAnalysis((uintptr_t)imageBase, sizeOfImage, game);
    bool ok = reader.AnalyzeImage(img);
    printf("Jogo: %s | SizeOfImage: %X\n%s", GameDisplayName(game), sizeOfImage, reader.ScanLog().c_str());

    if (strcmp(game, "sekiro") == 0) {
        // Every Sekiro signature must resolve (exactly one match, independent ones agreeing).
        const std::string& log = reader.ScanLog();
        bool all = log.find("NÃO ENCONTRADO") == std::string::npos && log.find("DIVERGENTE") == std::string::npos;
        printf("Todas as assinaturas: %s\n", all ? "OK" : "FALTANDO");
        ok = ok && all;
        if (sizeOfImage == 0x42D2000) {   // 1.05/1.06: compare with the addresses found by hand in 1.06
            const SekiroReader& s = reader.Sekiro();
            bool match = s.GameDataManAddr() - imageBase == 0x3D5AAC0 && s.GameManAddr() - imageBase == 0x3D5C0D0 &&
                         s.WorldChrManAddr() - imageBase == 0x3D7A1E0 && s.FieldAreaAddr() - imageBase == 0x3D5C0A0;
            printf("Conferencia 1.06: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    } else if (sizeOfImage == 0x4869400) {   // DSR 1.01: compare with the addresses found by hand
        bool match = reader.GameManAddr() - imageBase == 0x1CEA4B8 &&
                     reader.GameDataManAddr() - imageBase == 0x1D00F50 &&
                     reader.WorldChrManAddr() - imageBase == 0x1CEE830;
        printf("Conferencia 1.01: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");
    return ok ? 0 : 1;
}
