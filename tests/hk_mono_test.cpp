// Runs the counter's Mono analysis against Hollow Knight's runtime DLL on disk. Nothing is
// loaded or executed: the file is only parsed and searched.
//   build\tests\hk_mono_test.exe "C:\...\Hollow Knight\MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll"
#include "games/hollowknight.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) {
        fwprintf(stderr, L"uso: hk_mono_test <mono-2.0-bdwgc.dll>\n");
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<uint8_t> img;
    if (!hk::MappedImageFromFile(file, img)) {
        fprintf(stderr, "DLL invalida\n");
        return 2;
    }

    hk::MonoLayout L;
    std::string log;
    bool ok = hk::AnalyzeMono(img, L, log);
    printf("Imagem: %zX bytes\n%s", img.size(), log.c_str());
    printf("Resultado: %s\n", ok ? "OK" : "FALHOU");

    if (file.size() == 7830960) {   // Steam build of 2026-02 (Unity 6000.0.61f1): values found by hand
        bool match = L.loadedAssemblies == 0x740F90 && L.assemblyImage == 0x60 && L.imageName == 0x30 &&
                     L.imageClassCache == 0x4D0 && L.classParent == 0x30 && L.className == 0x48 &&
                     L.classNamespace == 0x50 && L.classVtableSize == 0x5C && L.classFields == 0x98 &&
                     L.classRuntimeInfo == 0xD0 && L.classDefFieldCount == 0x100 && L.classDefNextCache == 0x108 &&
                     L.fieldName == 0x8 && L.fieldOffset == 0x18 && L.fieldSize == 0x20 && L.vtableData == 0x48;
        printf("Conferencia Unity 6000.0.61f1: %s\n", match ? "OK" : "DIFERENTE");
        ok = ok && match;
    }
    return ok ? 0 : 1;
}
