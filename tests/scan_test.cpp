// Runs the counter's pattern scanner against a game executable on disk.
// Useful to check a new game version without launching it:
//   build\tests\scan_test.exe "C:\...\DarkSoulsRemastered.exe"
//   build\tests\scan_test.exe "C:\...\DarkSoulsII.exe"
//   build\tests\scan_test.exe "C:\...\DarkSoulsIII.exe"
//   build\tests\scan_test.exe "C:\...\sekiro.exe"
// The game is picked from the file name (as the app does from the process name).
#include "game_reader.h"
#include "games/ds2.h"
#include "games/ds3.h"
#include "games/sekiro.h"
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
        // Boss health bars: MenuMan (RTTI NS_FRPG::MenuMan) + 0x1050, two slots of 0x14 bytes.
        bool bossMatch = reader.MenuManAddr() - imageBase == 0x1CFF7C8 &&
                         reader.BossGaugeOffset() == 0x1050 && reader.BossGaugeStride() == 0x14;
        printf("Conferencia 1.01 (barra de chefe): %s\n", bossMatch ? "OK" : "DIFERENTE");
        ok = ok && match && bossMatch;
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
    if (game->id == std::string("ds3")) {
        // Every signature must match exactly once (globals: all of their signatures, agreeing).
        const Ds3Reader& d = static_cast<Ds3Module*>(reader.module())->reader();
        bool all = d.signaturesFound == d.signaturesTotal && d.gameMan != 0 && d.fieldArea != 0;
        printf("Assinaturas DS3: %d/%d %s\n", d.signaturesFound, d.signaturesTotal, all ? "OK" : "FALTANDO");
        ok = ok && all;
        if (sizeOfImage == 0x63DF000) {   // 1.15.0: compare with the addresses and offsets found by hand
            const auto& o = d.off;
            bool match = d.gameDataMan - imageBase == 0x4740178 && d.worldChrMan - imageBase == 0x4768E78 &&
                         d.gameMan - imageBase == 0x4743AB0 && d.fieldArea - imageBase == 0x4743A80 &&
                         o.deaths == 0x98 && o.playTime == 0xA4 && o.playerIns == 0x80 && o.saveSlot == 0xA60 &&
                         o.respawn == 0xACC && o.modules == 0x1F90 && o.dataModule == 0x18 && o.hp == 0xD8 &&
                         o.playRegion == 0x1ABC && o.worldInfo == 0x10 && o.blockIndex == 0x20 && o.blockCount == 0x18 &&
                         o.blockList == 0x20 && o.blockStride == 0x70 && o.blockMapId == 8;
            printf("Conferencia DS3 1.15.0: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    }
    if (game->id == std::string("sekiro")) {
        // Every signature must resolve (exactly one match, independent ones agreeing).
        const std::string& log = reader.ScanLog();
        bool all = log.find("NÃO ENCONTRADO") == std::string::npos && log.find("DIVERGENTE") == std::string::npos;
        printf("Todas as assinaturas: %s\n", all ? "OK" : "FALTANDO");
        ok = ok && all;
        if (sizeOfImage == 0x42D2000) {   // 1.05/1.06: compare with the addresses found by hand in 1.06
            const SekiroReader& sk = static_cast<SekiroModule*>(reader.module())->reader();
            bool match = sk.GameDataManAddr() - imageBase == 0x3D5AAC0 && sk.GameManAddr() - imageBase == 0x3D5C0D0 &&
                         sk.WorldChrManAddr() - imageBase == 0x3D7A1E0 && sk.FieldAreaAddr() - imageBase == 0x3D5C0A0;
            printf("Conferencia Sekiro 1.06: %s\n", match ? "OK" : "DIFERENTE");
            ok = ok && match;
        }
    }
    return ok ? 0 : 1;
}
