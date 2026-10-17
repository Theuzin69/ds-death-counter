// Runs the counter's Bloodborne scanner against the eboot.bin file the port loads (plaintext PS4 SELF).
// Nothing is executed: the file is only parsed and searched.
//   build\tests\bb_scan_test.exe "C:\...\Image0\eboot.bin"
#include "games/bloodborne.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: bb_scan_test <eboot.bin>\n");
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> img;
    std::string err;
    if (!bb::FlatImageFromSelf(file, img, err)) {
        fprintf(stderr, "eboot invalido: %s\n", err.c_str());
        return 2;
    }

    bb::Layout lay;
    std::string log;
    bool ok = bb::AnalyzeImage(img, lay, log);
    printf("Imagem: %zX bytes\n%s", img.size(), log.c_str());
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");

    if (img.size() == 0x56D30F4) {   // 1.09: compare with the values found by hand
        bool match = lay.gameDataMan == 0x553B130 && lay.offDeaths == 0x88 && lay.offTrueDeaths == 0x84 &&
                     lay.offPlayTime == 0x94 && lay.offClearCount == 0x68 && lay.offPlayerData == 0x8 &&
                     lay.offSoulLevel == 0x90 && lay.offName == 0xA8;
        printf("Conferencia 1.09: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    return ok ? 0 : 1;
}
