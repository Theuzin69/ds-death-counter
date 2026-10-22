// Tests for Undertale support (src/games/undertale.*).
//   build\tests\ut_test.exe
//       Logic only: undertale.ini parsing, rooms/areas, battle groups, game over context, and the
//       module counting from an ini in a temporary folder (the real save folder is never touched).
//   build\tests\ut_test.exe "C:\Program Files (x86)\Steam\steamapps\common\Undertale"
//       Also reads data.win and analyzes UNDERTALE.exe from that folder (read-only) and compares
//       with the values found by hand in Undertale 1.08.
//   build\tests\ut_test.exe --live [seconds]
//       Attaches to the running game like the app does and prints every change (room, battle group,
//       game over count, area, cause). Read-only.
#include "game_reader.h"
#include "games/undertale.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
void Check(bool ok, const char* what)
{
    printf("  [%s] %s\n", ok ? "ok" : "FALHOU", what);
    if (!ok) ++g_failures;
}

const char* kIni =
    "[Papyrus]\r\nM1=\"1.000000\"\r\n[General]\r\nRoom=\"116.000000\"\r\nKills=\"54.000000\"\r\nTime=\"185796.000000\"\r\n"
    "Love=\"9.000000\"\r\nName=\"Frisk\"\r\nGameover=\"12.000000\"\r\n[reset]\r\nreset=\"1.000000\"\r\n";

void WriteFile(const std::wstring& path, const std::string& text)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(text.data(), (std::streamsize)text.size());
}

void TestIni()
{
    printf("undertale.ini\n");
    ut::IniData d;
    Check(ut::ParseIni(kIni, d) && d.gameOvers == 12 && d.name == "Frisk" && d.love == 9 && d.room == 116 &&
          d.timeFrames == 185796, "arquivo completo");
    std::string cut(kIni);
    cut.resize(cut.size() - 30);
    Check(!ut::ParseIni(cut, d), "arquivo cortado no meio da escrita é rejeitado");
    Check(!ut::ParseIni("", d), "arquivo vazio é rejeitado");
    Check(ut::ParseIni("[General]\r\nName=\"Chara\"\r\nLove=\"1.000000\"\r\n", d) && d.gameOvers == 0 && d.name == "Chara",
          "sem a chave Gameover = 0 game overs");
    Check(ut::ParseIni("[reset]\r\nreset=\"1.000000\"\r\n", d) && d.gameOvers == 0 && d.name.empty(),
          "ini recriado pelo True Reset");
}

void TestTables()
{
    printf("salas, áreas e batalhas\n");
    Check(ut::RoomName(310) == "room_gameover" && ut::RoomName(306) == "room_battle" && ut::RoomName(9999).empty(),
          "nomes das salas (ordem 1.08)");
    Check(ut::KindOfRoom("room_gameover") == ut::RoomKind::GameOver && ut::KindOfRoom("room_battle") == ut::RoomKind::Battle &&
          ut::KindOfRoom("room_intromenu") == ut::RoomKind::Menu && ut::KindOfRoom("room_ruins1") == ut::RoomKind::World,
          "tipos de sala");
    Check(ut::AreaOfRoom("room_basement5") == "Ruins" && ut::AreaOfRoom("room_fogroom") == "Snowdin" &&
          ut::AreaOfRoom("room_water_undynefinal") == "Waterfall" && ut::AreaOfRoom("room_fire_hotellobby") == "Hotland" &&
          ut::AreaOfRoom("room_fire_core_final") == "Core" && ut::AreaOfRoom("room_basement4_final") == "New Home" &&
          ut::AreaOfRoom("room_sanscorridor") == "New Home" && ut::AreaOfRoom("room_truelab_hub") == "True Lab",
          "áreas");
    // Every walking room of the game gets an area, except a few special ones.
    int missing = 0;
    for (int r = 0; r < 336; ++r) {
        std::string n = ut::RoomName(r);
        if (ut::KindOfRoom(n) == ut::RoomKind::World && ut::AreaOfRoom(n).empty()) ++missing;
    }
    printf("  (%d salas de exploração sem área: testes, Gaster, Asriel...)\n", missing);
    Check(missing < 40, "quase todas as salas têm área");
    Check(ut::BattleGroupEnemies(22) == "Chefe: Toriel" && ut::BattleGroupEnemies(27) == "Chefe: Papyrus" &&
          ut::BattleGroupEnemies(95) == "Chefe: Sans" && ut::BattleGroupEnemies(101) == "Chefe: Asgore" &&
          ut::BattleGroupEnemies(92) == "Chefe: Undyne the Undying",
          "chefes");
    Check(ut::BattleGroupEnemies(6) == "Froggit e Whimsun" && ut::BattleGroupEnemies(15) == "Loox, Vegetoid e Migosp" &&
          ut::BattleGroupEnemies(19).empty() && ut::BattleGroupEnemies(-1).empty(),
          "grupos de inimigos");
    Check(ut::SaveRoomLabel(116) == "Waterfall - Quiet Area" && ut::SaveRoomLabel(68) == "Snowdin - Town" &&
          ut::SaveRoomLabel(45) == "Snowdin",
          "nome do último save");
}

void TestDeathContext()
{
    printf("contexto do game over\n");
    ULONGLONG t = 1000;
    {
        ut::DeathContext c;
        c.Update("room_basement4", 5, t += 100);
        c.Update("room_basement4", 22, t += 100);   // the overworld sets the group, then goes to battle
        c.Update("room_battle", 22, t += 100);
        Check(c.area() == "Ruins" && c.cause() == "Chefe: Toriel", "durante a batalha");
        c.Update("room_gameover", 22, t += 100);
        Check(c.inGameOver() && c.area() == "Ruins" && c.cause() == "Chefe: Toriel", "na tela de game over");
        c.Update("room_ruins19", 22, t += 100);     // continued from the save
        Check(c.area() == "Ruins" && c.cause() == "Chefe: Toriel", "logo depois de continuar (leitura atrasada do ini)");
        c.Update("room_ruins19", 22, t += 11000);
        Check(c.cause().empty(), "depois de alguns segundos a causa some");
    }
    {
        ut::DeathContext c;
        c.Update("room_water_undynebridge", 30, t += 100);
        c.Update("room_water_undynebridge", 47, t += 100);   // Undyne's spears: battle inside the walking room
        c.Update("room_gameover", 47, t += 100);
        Check(c.area() == "Waterfall" && c.cause() == "Chefe: Undyne", "batalha dentro de uma sala de exploração");
    }
    {
        ut::DeathContext c;
        c.Update("room_tundra3", 30, t += 100);
        c.Update("room_gameover", 30, t += 100);
        Check(c.area() == "Snowdin" && c.cause().empty(), "game over fora de batalha: sem causa inventada");
    }
    {
        ut::DeathContext c;
        c.Update("room_fire_core_final", -1, t += 100);
        c.Update("room_battle", -1, t += 100);      // globals unreadable
        c.Update("room_gameover", -1, t += 100);
        Check(c.area() == "Core" && c.cause().empty(), "sem as variáveis globais: só a área");
    }
}

void TestModuleIni()
{
    printf("contagem pelo undertale.ini (pasta temporária)\n");
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring dir = std::wstring(tmp) + L"ut_test_" + std::to_wstring(GetCurrentProcessId());
    CreateDirectoryW(dir.c_str(), nullptr);
    std::wstring ini = dir + L"\\undertale.ini";
    UndertaleModule m(dir);
    auto read = [&](int waitMs) {
        Sleep(waitMs);
        GameSnapshot s;
        s.game = "ut";
        m.ReadWithoutProcess(s);
        return s;
    };

    GameSnapshot s = read(0);
    Check(s.deaths == 0 && !s.charLoaded, "sem ini: 0 e nenhum personagem");
    WriteFile(ini, kIni);
    s = read(300);
    Check(s.deaths == 12 && s.charLoaded && s.name == L"Frisk" && s.saveSlot == 0 && s.soulLevel == 9 &&
          s.lastBonfire == 116 && s.areaName == "Waterfall" && s.playTimeMs == 185796u * 1000 / 30,
          "ini lido (Frisk, 12 game overs, LV 9, save em Waterfall)");
    std::string more(kIni);
    more.replace(more.find("12.000000"), 9, "13.000000");
    std::string cut = more.substr(0, more.size() - 25);
    WriteFile(ini, cut);
    s = read(300);
    Check(s.deaths == 12, "ini pela metade: mantém o último valor");
    WriteFile(ini, more);
    s = read(300);
    Check(s.deaths == 13, "game over: 13");
    // A complete-looking file without Gameover (cut exactly at a line end) must not lower the count at once.
    WriteFile(ini, "[General]\r\nName=\"Frisk\"\r\n");
    s = read(300);
    Check(s.deaths == 13, "contagem menor não vale na primeira leitura");
    WriteFile(ini, more);
    s = read(300);
    Check(s.deaths == 13, "voltou ao normal: nada mudou");
    DeleteFileW(ini.c_str());   // True Reset
    s = read(300);
    s = read(300);
    s = read(300);
    Check(s.deaths == 0 && !s.charLoaded, "True Reset (ini apagado): volta a 0 depois de confirmar");
    RemoveDirectoryW(dir.c_str());
}

// Maps a 32-bit exe the way the loader does (sections at their virtual addresses).
bool MapPe32(const std::wstring& path, std::vector<uint8_t>& img, uint32_t& base)
{
    std::ifstream f(path, std::ios::binary);
    std::vector<uint8_t> file((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (file.size() < 0x400) return false;
    auto rd32 = [&](size_t at) { uint32_t v; memcpy(&v, &file[at], 4); return v; };
    auto rd16 = [&](size_t at) { uint16_t v; memcpy(&v, &file[at], 2); return v; };
    uint32_t pe = rd32(0x3C);
    if (rd16(pe + 24) != 0x10B) return false;   // PE32
    uint16_t nsec = rd16(pe + 6), optSize = rd16(pe + 20);
    base = rd32(pe + 24 + 28);
    img.assign(rd32(pe + 24 + 56), 0);
    for (uint16_t i = 0; i < nsec; ++i) {
        size_t o = pe + 24 + optSize + i * 40;
        uint32_t vsize = rd32(o + 8), va = rd32(o + 12), rawSize = rd32(o + 16), rawPtr = rd32(o + 20);
        uint32_t n = vsize < rawSize ? vsize : rawSize;
        if (rawPtr + n <= file.size() && va + n <= img.size()) memcpy(&img[va], &file[rawPtr], n);
    }
    return true;
}

void TestGameFiles(const std::wstring& folder)
{
    printf("arquivos do jogo (somente leitura)\n");
    UndertaleModule m(L"");
    bool loaded = m.LoadDataWin(folder + L"\\data.win");
    const ut::DataWin& d = m.dataWin();
    printf("  data.win: bytecode %d, %zu salas, slots battlegroup %d charname %d lv %d %s\n", d.bytecode, d.rooms.size(),
           d.slotBattleGroup, d.slotCharName, d.slotLv, d.error.c_str());
    Check(loaded && d.rooms.size() > 310 && d.rooms[310] == "room_gameover" && d.rooms[306] == "room_battle",
          "salas do data.win (310 = room_gameover)");
    bool sameOrder = loaded;
    for (size_t r = 0; r < d.rooms.size() && sameOrder; ++r) sameOrder = d.rooms[r] == ut::RoomName((int)r);
    Check(sameOrder, "ordem das salas igual à tabela embutida (1.08)");
    Check(d.slotBattleGroup >= 0 && d.slotCharName >= 0 && d.slotLv >= 0, "slots das variáveis globais");
    if (d.bytecode == 16 && d.rooms.size() == 336)
        Check(d.slotBattleGroup == 241 && d.slotCharName == 123 && d.slotLv == 54, "slots conferem com a 1.08 (241/123/54)");

    std::vector<uint8_t> img;
    uint32_t base = 0;
    if (!MapPe32(folder + L"\\UNDERTALE.exe", img, base)) { Check(false, "UNDERTALE.exe (PE de 32 bits)"); return; }
    GameReader reader;
    reader.SetModuleForAnalysis(base, (uint32_t)img.size(), "ut");
    bool ok = reader.AnalyzeImage(img);
    printf("%s", reader.ScanLog().c_str());
    auto* ut = static_cast<UndertaleModule*>(reader.module());
    Check(ok && ut->RoomAddr() && ut->GlobalInstanceAddr(), "análise do executável");
    if (img.size() == 0x652000)
        Check(ut->RoomAddr() - base == 0x618EA0 && ut->GlobalInstanceAddr() - base == 0x40894C && ut->VarsOffset() == 4 &&
              ut->MapOffset() == 0x44,
              "endereços conferem com os achados à mão na 1.08");
}

int Live(int seconds)
{
    printf("Abra o Undertale. Lendo por %d s (somente leitura)...\n", seconds);
    GameReader reader;
    std::string last;
    for (int t = 0; t < seconds * 10; ++t) {
        reader.Update();
        GameSnapshot s;
        std::string line;
        if (reader.IsReady() && reader.Read(s) && s.game == "ut") {
            char b[512];
            snprintf(b, sizeof(b), "game overs %d | nome %ls | LV %d | no jogo %d | área \"%s\" | causa \"%s\"\n", s.deaths,
                     s.name.c_str(), s.soulLevel, s.inWorld, s.areaName.c_str(), s.cause.c_str());
            line = b + reader.module()->LiveDiagnostics();
        } else {
            line = "aguardando o jogo (estado " + std::to_string((int)reader.state()) + ")\n";
        }
        if (line != last) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            printf("--- %02d:%02d:%02d\n%s", st.wHour, st.wMinute, st.wSecond, line.c_str());
            last = line;
        }
        Sleep(100);
    }
    return 0;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc >= 2 && wcscmp(argv[1], L"--live") == 0) return Live(argc >= 3 ? _wtoi(argv[2]) : 120);
    TestIni();
    TestTables();
    TestDeathContext();
    TestModuleIni();
    if (argc >= 2) TestGameFiles(argv[1]);
    printf(g_failures ? "\n%d FALHA(S)\n" : "\nTudo OK\n", g_failures);
    return g_failures ? 1 : 0;
}
