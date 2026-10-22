// Runs the Elden Ring save reader against an ER0000.sl2 on disk (read-only):
//   build\tests\er_save_test.exe "%APPDATA%\EldenRing\<steamid>\ER0000.sl2"
// Prints what the counter would read from each character slot. Exit code 0 when every
// slot in use was understood.
//
//   build\tests\er_save_test.exe --simulate "...\ER0000.sl2"
// Copies the file to %TEMP%, then plays "the game" on the copy: writes a death into one slot
// (new count, new bloodstain, new checksum), a half-written save and a second death, and checks
// what the counter's save watcher reports after each write. The original file is only read.
#include "game_reader.h"
#include "games/eldenring.h"
#include "games/eldenring_data.h"
#include "games/eldenring_save.h"
#include "util.h"
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>

namespace {

bool Md5(const uint8_t* data, size_t len, uint8_t out[16])
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = false;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, nullptr, 0))) {
        if (BCRYPT_SUCCESS(BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0))) {
            ok = BCRYPT_SUCCESS(BCryptHashData(h, (PUCHAR)data, (ULONG)len, 0)) && BCRYPT_SUCCESS(BCryptFinishHash(h, out, 16, 0));
            BCryptDestroyHash(h);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}

bool WriteAll(const std::wstring& path, const std::vector<uint8_t>& data)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write((const char*)data.data(), (std::streamsize)data.size());
    return (bool)f;
}

int g_failures = 0;
void Check(bool ok, const char* what)
{
    printf("  [%s] %s\n", ok ? "ok" : "FALHOU", what);
    if (!ok) ++g_failures;
}

// Polls like the app does (every 100 ms) until `done` or the timeout.
template <class F> void PollUntil(EldenRingModule& er, F done, int ms)
{
    for (int t = 0; t < ms && !done(); t += 100) {
        er.Poll();
        Sleep(100);
    }
}

int Simulate(const std::vector<uint8_t>& orig)
{
    ErSaveData d;
    if (!ErParseSave(orig, d)) { printf("FALHOU: %s\n", d.error.c_str()); return 1; }
    int target = -1, used = 0;
    for (int i = 0; i < 10; ++i) {
        if (d.slots[i].used) ++used;
        if (d.slots[i].valid && d.slots[i].detailOk) target = i;
    }
    if (target < 0) { printf("nenhum slot utilizável para a simulação\n"); return 1; }
    const ErSaveSlot before = d.slots[target];
    auto entryOff = [&](int i) { uint32_t v; memcpy(&v, orig.data() + 0x40 + i * 0x20 + 0x10, 4); return (size_t)v; };
    size_t slotAt = entryOff(target), data = slotAt + 0x10;

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring root = std::wstring(tmp) + L"dsdc_er_sim_" + std::to_wstring(GetCurrentProcessId());
    std::wstring dir = root + L"\\EldenRing\\" + std::to_wstring(d.steamId);
    CreateDirectoryW(root.c_str(), nullptr);
    CreateDirectoryW((root + L"\\EldenRing").c_str(), nullptr);
    CreateDirectoryW(dir.c_str(), nullptr);
    std::wstring file = dir + L"\\ER0000.sl2";
    if (!WriteAll(file, orig)) { printf("não foi possível escrever a cópia em %%TEMP%%\n"); return 1; }
    SetEnvironmentVariableW(L"APPDATA", root.c_str());   // the watcher looks in %APPDATA%\EldenRing
    printf("Cópia: %s\nSlot usado na simulação: %d (%s, %u mortes)\n", WideToUtf8(file).c_str(), target + 1,
           WideToUtf8(before.name).c_str(), before.deaths);

    EldenRingModule er;
    GameSnapshot s;
    printf("1. Primeira leitura\n");
    PollUntil(er, [&] { return er.save().HasData(); }, 3000);
    Check(er.save().HasData(), "save encontrado e lido");
    er.ReadWithoutProcess(s);
    if (used > 1)
        Check(!s.charLoaded && er.save().ActiveSlot() < 0, "vários personagens: espera o jogo gravar para saber qual é");
    else
        Check(s.charLoaded && s.saveSlot == target, "um só personagem: já identificado");

    // "The game" writes a death: counter +1, play time +5 s, bloodstain in Stormveil Castle, new MD5.
    std::vector<uint8_t> buf = orig;
    auto put32 = [&](size_t at, uint32_t v) { memcpy(buf.data() + at, &v, 4); };
    uint32_t deaths = before.deaths + 1;
    put32(data + before.deathsAt, deaths);
    uint64_t pt = before.playTimeMs + 5000;
    memcpy(buf.data() + data + 8, &pt, 8);
    put32(data + before.bloodstainAt + 0x30, 0);
    put32(data + before.bloodstainAt + 0x34, 1234);
    put32(data + before.bloodstainAt + 0x38, 0x0A000000);   // m10_00_00_00
    Md5(buf.data() + data, 0x280000, buf.data() + slotAt);
    Sleep(50);
    WriteAll(file, buf);
    printf("2. Morte gravada no slot %d\n", target + 1);
    PollUntil(er, [&] { return er.save().readCount() >= 2; }, 4000);
    s = GameSnapshot{};
    er.ReadWithoutProcess(s);
    Check(er.save().ActiveSlot() == target, "slot em uso = o que o jogo regravou");
    Check(s.charLoaded && s.saveSlot == target, "personagem identificado");
    Check(s.deaths == (int)deaths, "contagem nova lida");
    Check(s.areaName == "Stormveil Castle", "área da morte = mapa da nova mancha de sangue");
    Check(s.name == before.name, "nome do personagem");

    // A write caught in the middle: data changed, checksum not yet. Must not be trusted.
    std::vector<uint8_t> torn = buf;
    uint32_t bogus = deaths + 50;
    memcpy(torn.data() + data + before.deathsAt, &bogus, 4);
    Sleep(50);
    WriteAll(file, torn);
    printf("3. Arquivo pela metade (checksum não confere)\n");
    int readsBefore = er.save().readCount();
    PollUntil(er, [&] { return false; }, 2500);
    s = GameSnapshot{};
    er.ReadWithoutProcess(s);
    Check(er.save().readCount() == readsBefore, "leitura adiada (tentando de novo)");
    Check(s.deaths == (int)deaths, "contagem não mudou");

    // The game finishes the write: second death.
    deaths += 1;
    put32(data + before.deathsAt, deaths);
    pt += 5000;
    memcpy(buf.data() + data + 8, &pt, 8);
    Md5(buf.data() + data, 0x280000, buf.data() + slotAt);
    Sleep(50);
    WriteAll(file, buf);
    printf("4. Gravação terminada (segunda morte)\n");
    PollUntil(er, [&] { return er.save().readCount() > readsBefore; }, 4000);
    s = GameSnapshot{};
    er.ReadWithoutProcess(s);
    Check(s.deaths == (int)deaths, "segunda morte lida");
    Check(s.areaName != "Stormveil Castle" && !s.areaName.empty(), "mancha de sangue igual: área = mapa do save");

    DeleteFileW(file.c_str());
    RemoveDirectoryW(dir.c_str());
    RemoveDirectoryW((root + L"\\EldenRing").c_str());
    RemoveDirectoryW(root.c_str());
    printf("Resultado da simulação: %s\n", g_failures ? "FALHOU" : "OK");
    return g_failures ? 1 : 0;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    bool simulate = argc >= 3 && wcscmp(argv[1], L"--simulate") == 0;
    if (argc < 2 || (argc >= 3 && !simulate)) {
        fwprintf(stderr, L"uso: er_save_test [--simulate] <ER0000.sl2>\n");
        return 2;
    }
    std::ifstream f(argv[simulate ? 2 : 1], std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (simulate) return Simulate(file);

    ErSaveData d;
    bool ok = ErParseSave(file, d);
    printf("Arquivo: %zu bytes | %s%s\n", file.size(), ok ? "OK" : "FALHOU: ", d.error.c_str());
    if (!ok) return 1;
    printf("Steam id: %llu\n", (unsigned long long)d.steamId);
    int used = 0, valid = 0;
    for (int i = 0; i < 10; ++i) {
        const ErSaveSlot& s = d.slots[i];
        if (!s.used) continue;
        ++used;
        valid += s.valid;
        printf("Slot %d: %s | nível %d | versão %u | tempo %s\n", i + 1, WideToUtf8(s.name).c_str(), s.level, s.version,
               FormatPlayTime((uint32_t)s.playTimeMs).c_str());
        if (!s.valid) {
            printf("  ERRO: %s\n", s.error.c_str());
            continue;
        }
        printf("  mortes: %u (slot+0x%X)\n  mapa: %s (%s)\n  última graça: %u (%s)\n", s.deaths, s.deathsAt,
               ErMapIdText(s.mapId).c_str(), ErMapName(s.mapId).c_str(), s.lastGrace, ErGraceLabel(s.lastGrace).c_str());
        if (s.detailOk)
            printf("  NG+: %d | mancha de sangue: %s em %s (%s)\n", s.ngPlus, s.bloodstainActive ? "sim" : "não",
                   ErMapIdText(s.bloodstainMap).c_str(), ErMapName(s.bloodstainMap).c_str());
        else
            printf("  (leitura detalhada indisponível nesta versão do save)\n");
    }
    printf("Resultado: %d de %d slot(s) lidos\n", valid, used);
    return used > 0 && valid == used ? 0 : 1;
}
