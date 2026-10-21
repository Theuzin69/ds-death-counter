// Runs the counter's pattern scanner against a game exe on disk (DarkSoulsRemastered.exe or
// eldenring.exe). Useful to check a new game version without launching it:
//   build\tests\scan_test.exe "C:\...\DarkSoulsRemastered.exe"
//   build\tests\scan_test.exe "C:\...\eldenring.exe"
#include "game_reader.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <vector>

#pragma comment(lib, "version.lib")

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: scan_test <DarkSoulsRemastered.exe | eldenring.exe>\n");
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

    // Which game: by file name.
    const wchar_t* base = wcsrchr(argv[1], L'\\');
    base = base ? base + 1 : argv[1];
    bool elden = _wcsicmp(base, L"eldenring.exe") == 0;
    uint32_t verMS = 0, verLS = 0;
    DWORD dummy = 0, sz = GetFileVersionInfoSizeW(argv[1], &dummy);
    if (sz) {
        std::vector<uint8_t> data(sz);
        VS_FIXEDFILEINFO* ffi = nullptr;
        UINT len = 0;
        if (GetFileVersionInfoW(argv[1], 0, sz, data.data()) && VerQueryValueW(data.data(), L"\\", (void**)&ffi, &len) && ffi) {
            verMS = ffi->dwProductVersionMS;
            verLS = ffi->dwProductVersionLS;
        }
    }

    GameReader reader;
    reader.SetModuleForAnalysis((uintptr_t)imageBase, sizeOfImage,
                                elden ? GameReader::Game::EldenRing : GameReader::Game::DarkSoulsRemastered, verMS, verLS);
    bool ok = reader.AnalyzeImage(img);
    printf("Jogo: %s\nSizeOfImage: %X\n%s", elden ? "Elden Ring" : "Dark Souls Remastered", sizeOfImage, reader.ScanLog().c_str());

    if (elden) {
        const EldenRingGame& er = reader.eldenRing();
        bool unique = er.allSignaturesUnique();
        printf("Assinaturas: %s\n", unique ? "todas encontradas uma única vez e concordando" : "ALGUMA FALHOU");
        ok = ok && unique;
        if (verMS == 0x00010002 && verLS == 0) {   // 1.02.0: compare with the addresses checked by hand
            bool match = er.GameDataManAddr() - imageBase == 0x3C481B8 && er.WorldChrManAddr() - imageBase == 0x3C50268 &&
                         er.playerInsOffset() == 0x18468;
            printf("Conferencia 1.02.0: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    } else if (sizeOfImage == 0x4869400) {   // 1.01: compare with the addresses found by hand
        bool match = reader.GameManAddr() - imageBase == 0x1CEA4B8 &&
                     reader.GameDataManAddr() - imageBase == 0x1D00F50 &&
                     reader.WorldChrManAddr() - imageBase == 0x1CEE830;
        printf("Conferencia 1.01: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");
    return ok ? 0 : 1;
}
