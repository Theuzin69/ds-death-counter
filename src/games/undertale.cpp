#include "games/undertale.h"
#include "game_reader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>

namespace {

int32_t I32(const std::vector<uint8_t>& img, size_t at)
{
    int32_t v = 0;
    if (at + 4 <= img.size()) memcpy(&v, img.data() + at, 4);
    return v;
}

bool ValidPtr32(uint32_t p) { return p >= 0x10000 && p < 0xFFFF0000u; }

std::wstring Utf8ToWideStr(const std::string& s)
{
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

// Names typed in the naming screen: printable text only (anything else means a bad read).
bool PlausibleName(const std::string& s)
{
    if (s.empty() || s.size() > 32) return false;
    for (unsigned char c : s)
        if (c < 0x20 || c == 0x7F) return false;
    return true;
}

// Reads a file without ever getting in the game's way: full sharing, opened only for the read.
bool ReadWholeFile(const std::wstring& path, std::string& out, DWORD maxSize)
{
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(f, &size) && size.QuadPart <= maxSize;
    if (ok) {
        out.resize((size_t)size.QuadPart);
        DWORD got = 0;
        ok = out.empty() || (ReadFile(f, out.data(), (DWORD)out.size(), &got, nullptr) && got == out.size());
    }
    CloseHandle(f);
    return ok;
}

std::unique_ptr<GameModule> CreateUndertale() { return std::make_unique<UndertaleModule>(); }

} // namespace

const GameInfo kUndertaleInfo = {
    "ut", L"UNDERTALE.exe", "Undertale", "UT", CreateUndertale,
    ut::SaveRoomLabel, ut::AllAreaNames, ut::CauseSuggestions, "Último save", true, false, false,
};

namespace ut {

// ---------------------------------------------------------------------------------------------
// undertale.ini

// GameMaker writes the whole file on every ini_close: `[Section]` and `Key="1.000000"` lines, each
// ending in \r\n. A read that catches the file half-written has a last line without its newline.
bool ParseIni(const std::string& text, IniData& out)
{
    out = IniData{};
    if (text.empty() || text.back() != '\n') return false;
    std::string section;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (line.front() == '[') {
            if (line.back() != ']') return false;
            section = line.substr(1, line.size() - 2);
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) return false;
        std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        if (section != "General") continue;
        double num = atof(value.c_str());
        if (key == "Gameover") out.gameOvers = (int)std::clamp(num, 0.0, 10000000.0);
        else if (key == "Name") out.name = value;
        else if (key == "Love") out.love = (int)std::clamp(num, 0.0, 99.0);
        else if (key == "Time") out.timeFrames = (uint32_t)std::clamp(num, 0.0, 4.0e9);
        else if (key == "Room") out.room = (int)std::clamp(num, -1.0, 100000.0);
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// Where and to whom the last game over happened

void DeathContext::Update(const std::string& room, int battleGroup, ULONGLONG now)
{
    now_ = now;
    RoomKind kind = KindOfRoom(room);
    if (kind == RoomKind::GameOver) {
        if (!inGameOver_) {
            inGameOver_ = true;
            goArea_ = AreaOfRoom(lastWorldRoom_);
            if (prevKind_ == RoomKind::Battle)
                goCause_ = battleRoom_ == "room_floweybattle" ? std::string("Chefe: Flowey") : BattleGroupEnemies(battleGroup_);
            else if (prevKind_ == RoomKind::World && battleGroup >= 0 && battleGroup != worldEntryGroup_)
                goCause_ = BattleGroupEnemies(battleGroup);   // battle inside a walking room (Undyne's spears)
            else
                goCause_.clear();
        }
        return;
    }
    if (kind == RoomKind::Battle) {
        battleRoom_ = room;
        if (battleGroup >= 0) battleGroup_ = battleGroup;
    } else if (kind == RoomKind::World) {
        if (prevKind_ != RoomKind::World || room != lastWorldRoom_) worldEntryGroup_ = battleGroup;
        lastWorldRoom_ = room;
    }
    if (inGameOver_) {
        inGameOver_ = false;
        leftGameOver_ = now;
    }
    prevKind_ = kind;
}

// The game adds the game over to its count while the screen is up; a short grace period after
// it covers a late read of the ini.
bool DeathContext::recentGameOver() const
{
    return inGameOver_ || (leftGameOver_ != 0 && now_ - leftGameOver_ < 10000);
}

std::string DeathContext::area() const
{
    return recentGameOver() ? goArea_ : AreaOfRoom(lastWorldRoom_);
}

std::string DeathContext::cause() const
{
    if (recentGameOver()) return goCause_;
    if (prevKind_ == RoomKind::Battle) return BattleGroupEnemies(battleGroup_);
    return {};
}

// ---------------------------------------------------------------------------------------------
// data.win: room names and the slots of a few global variables

bool DataWin::Load(const std::wstring& path)
{
    *this = DataWin{};
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) { error = "data.win não encontrado"; return false; }
    LARGE_INTEGER fsize{};
    GetFileSizeEx(f, &fsize);
    auto readAt = [&](uint64_t at, void* buf, DWORD n) {
        if (at + n > (uint64_t)fsize.QuadPart) return false;
        OVERLAPPED ov{};
        ov.Offset = (DWORD)at;
        ov.OffsetHigh = (DWORD)(at >> 32);
        DWORD got = 0;
        return ReadFile(f, buf, n, &got, &ov) && got == n;
    };
    auto u32 = [&](uint64_t at) { uint32_t v = 0; return readAt(at, &v, 4) ? v : 0u; };
    // Strings are stored as <u32 length><bytes>, and pointed to at the bytes.
    auto str = [&](uint32_t ptr) {
        uint32_t len = ptr >= 4 ? u32(ptr - 4) : 0;
        std::string s(std::min<uint32_t>(len, 256), '\0');
        if (s.empty() || !readAt(ptr, s.data(), (DWORD)s.size())) s.clear();
        return s;
    };

    uint64_t gen8 = 0, room = 0, vari = 0, variLen = 0;
    char tag[4];
    if (!readAt(0, tag, 4) || memcmp(tag, "FORM", 4) != 0) {
        error = "data.win sem cabeçalho FORM";
    } else {
        for (uint64_t at = 8; at + 8 <= (uint64_t)fsize.QuadPart;) {
            if (!readAt(at, tag, 4)) break;
            uint32_t len = u32(at + 4);
            if (!memcmp(tag, "GEN8", 4)) gen8 = at + 8;
            else if (!memcmp(tag, "ROOM", 4)) room = at + 8;
            else if (!memcmp(tag, "VARI", 4)) { vari = at + 8; variLen = len; }
            at += 8 + (uint64_t)len;
        }
        uint8_t gen[2] = {};
        if (gen8 && readAt(gen8, gen, 2)) bytecode = gen[1];
        if (room) {
            uint32_t n = std::min<uint32_t>(u32(room), 5000);
            std::vector<uint32_t> offs(n);
            if (n && readAt(room + 4, offs.data(), n * 4))
                for (uint32_t o : offs) rooms.push_back(str(u32(o)));
        }
        // VARI (bytecode 15+): 12-byte header, then { name, instance type, id, occurrences, first use }.
        // For globals (instance type -5) the runner uses that id as the variable's slot.
        if (vari && bytecode >= 15 && bytecode <= 16 && variLen < (16u << 20)) {
            std::vector<int32_t> v(variLen / 4);
            if (readAt(vari, v.data(), (DWORD)(v.size() * 4))) {
                for (size_t i = 3; i + 5 <= v.size(); i += 5) {
                    if (v[i + 1] != -5) continue;
                    std::string name = str((uint32_t)v[i]);
                    if (name == "battlegroup") slotBattleGroup = v[i + 2];
                    else if (name == "charname") slotCharName = v[i + 2];
                    else if (name == "lv") slotLv = v[i + 2];
                }
            }
        }
        if (rooms.empty()) error = "salas não encontradas no data.win";
        else if (slotBattleGroup < 0) error = "variáveis globais não encontradas no data.win (bytecode " + std::to_string(bytecode) + ")";
    }
    CloseHandle(f);
    return !rooms.empty();
}

} // namespace ut

// ---------------------------------------------------------------------------------------------
// The module

UndertaleModule::UndertaleModule()
{
    wchar_t local[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
    if (n > 0 && n < MAX_PATH) saveFolder_ = std::wstring(local) + L"\\UNDERTALE";
}

bool UndertaleModule::Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t, std::string& log)
{
    char line[256];
    roomAddr_ = globalAddr_ = 0;

    // Current room: the runner registers each built-in variable with
    //   push setter; push getter; push "room"; call AddBuiltin
    // and room's getter is: mov eax,[esp+0C]; mov [eax+0C],0 (real); cvtsi2sd xmm0,[Current_Room]; movsd [eax],xmm0
    for (size_t s : FindPattern(img, "00 72 6F 6F 6D 00")) {
        uint32_t va = (uint32_t)(base + s + 1);
        char push[32];
        snprintf(push, sizeof(push), "68 %02X %02X %02X %02X", va & 0xFF, (va >> 8) & 0xFF, (va >> 16) & 0xFF, va >> 24);
        for (size_t p : FindPattern(img, push)) {
            if (p < 10 || img[p - 5] != 0x68 || img[p - 10] != 0x68) continue;
            size_t getter = (uint32_t)I32(img, p - 4) - base;
            if (getter < img.size() &&
                PatternAt(img, getter, "8B 44 24 0C C7 40 0C 00 00 00 00 F2 0F 2A 05 ? ? ? ? F2 0F 11 00"))
                roomAddr_ = (uint32_t)I32(img, getter + 15);
        }
    }
    if (roomAddr_)
        snprintf(line, sizeof(line), "Sala atual    -> exe+%llX\n", (unsigned long long)(roomAddr_ - base));
    else
        snprintf(line, sizeof(line), "Sala atual    -> NÃO ENCONTRADA\n");
    log += line;

    // Global variables: the VM's "push global" handler masks the variable id, subtracts 100000
    // (globals are numbered from there) and asks the global instance for that slot:
    //   lea ecx,[edi-100000]; ...; push ecx; mov ecx,[g_pGlobal]; call CInstance::GetYYVarRef
    auto hits = FindPattern(img, "8D 8F 60 79 FE FF 89 5E 08 89 5E 04 89 1E C7 46 0C FF FF FF 00 51 8B 0D ? ? ? ? E8");
    if (hits.size() == 1) {
        size_t h = hits[0];
        globalAddr_ = (uint32_t)I32(img, h + 0x18);
        // GetYYVarRef: mov edx,[ecx+vars]; test edx,edx; je hashmap; ...; shl eax,4 (16-byte RValues) ...
        // and the hash map version: mov edi,ecx; cmp dword [edi+map],0
        size_t fn = h + 0x21 + I32(img, h + 0x1D);
        if (PatternAt(img, fn, "8B 51 ? 85 D2 74 ? 8B 44 24 04 C1 E0 04 03 C2 C2 04 00")) {
            offVars_ = img[fn + 2];
            size_t jmp = fn + 7 + (int8_t)img[fn + 6];
            if (jmp + 5 <= img.size() && img[jmp] == 0xE9) {
                size_t slow = jmp + 5 + I32(img, jmp + 1);
                for (size_t k = slow; k < slow + 16 && k + 6 <= img.size(); ++k)
                    if (PatternAt(img, k, "8B F9 83 7F ? 00")) { offMap_ = img[k + 4]; break; }
            }
        }
        snprintf(line, sizeof(line), "Globais       -> exe+%llX | variáveis +%X | tabela +%X\n",
                 (unsigned long long)(globalAddr_ - base), offVars_, offMap_);
    } else {
        snprintf(line, sizeof(line), "Globais       -> %s (sem causa automática)\n", hits.empty() ? "NÃO ENCONTRADAS" : "AMBÍGUAS");
    }
    log += line;
    return roomAddr_ != 0;
}

bool UndertaleModule::LoadDataWin(const std::wstring& path)
{
    dataWinTried_ = true;
    return dataWin_.Load(path);
}

// Reads undertale.ini when it changed (checked four times a second).
bool UndertaleModule::PollIni()
{
    ULONGLONG now = GetTickCount64();
    if (now < nextIniCheck_) return iniOk_;
    nextIniCheck_ = now + 250;
    if (saveFolder_.empty()) return false;
    std::wstring path = saveFolder_ + L"\\undertale.ini";

    WIN32_FILE_ATTRIBUTE_DATA fa{};
    ut::IniData parsed;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa)) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) return iniOk_;
        // No ini yet (never saved nor died) or deleted by a True Reset: nothing counted yet.
        fa = {};
    } else {
        uint64_t size = ((uint64_t)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
        if (iniOk_ && pendingLower_ == 0 && CompareFileTime(&fa.ftLastWriteTime, &iniTime_) == 0 && size == iniSize_)
            return true;
        std::string text;
        if (!ReadWholeFile(path, text, 1 << 20) || !ut::ParseIni(text, parsed)) return iniOk_;   // half-written: next time
    }

    // A smaller count only counts when a read half a second later still says so.
    if (iniOk_ && parsed.gameOvers < ini_.gameOvers) {
        if (pendingLower_ != parsed.gameOvers + 1) {
            pendingLower_ = parsed.gameOvers + 1;
            lowerSince_ = now;
            return true;
        }
        if (now - lowerSince_ < 500) return true;
    }
    pendingLower_ = 0;
    ini_ = parsed;
    iniOk_ = true;
    iniTime_ = fa.ftLastWriteTime;
    iniSize_ = ((uint64_t)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
    return true;
}

// Reads one variable of the runner's global instance. 32-bit process: every pointer is 4 bytes.
// RValue = { 8-byte value, int flags, int kind } with kind 0 = real, 1 = string (pointer to
// { const char* text; int refs; int length }).
bool UndertaleModule::ReadGlobal(HANDLE proc, int slot, double* num, std::string* str)
{
    auto rd = [&](uint32_t a, void* v, size_t n) {
        SIZE_T got = 0;
        return a && ReadProcessMemory(proc, (LPCVOID)(uintptr_t)a, v, n, &got) && got == n;
    };
    uint32_t g = 0, vars = 0, rv = 0;
    if (slot < 0 || !globalAddr_ || !rd((uint32_t)globalAddr_, &g, 4) || !ValidPtr32(g)) return false;
    if (rd(g + offVars_, &vars, 4) && ValidPtr32(vars)) {
        rv = vars + (uint32_t)slot * 16;
    } else {
        // CHashMap<int, RValue*>: { size, used, mask, threshold, elements }, element { key, value, hash },
        // hash = key + 1, open addressing from hash & mask.
        uint32_t map = 0, hdr[5] = {};
        if (!rd(g + offMap_, &map, 4) || !ValidPtr32(map) || !rd(map, hdr, sizeof(hdr))) return false;
        uint32_t size = hdr[0], mask = hdr[2], elems = hdr[4];
        if (size == 0 || size > (1u << 20) || mask != size - 1 || !ValidPtr32(elems)) return false;
        uint32_t want = ((uint32_t)slot + 1) & 0x7FFFFFFF;
        for (uint32_t i = 0, idx = want & mask; i < 64 && i < size; ++i, idx = (idx + 1) & mask) {
            uint32_t e[3] = {};
            if (!rd(elems + idx * 12, e, sizeof(e)) || e[2] == 0) return false;
            if (e[2] == want && e[0] == (uint32_t)slot) { rv = e[1]; break; }
        }
        if (!ValidPtr32(rv)) return false;
    }
    uint8_t v[16] = {};
    if (!rd(rv, v, sizeof(v))) return false;
    uint32_t kind = 0;
    memcpy(&kind, v + 12, 4);
    kind &= 0xFFFFFF;
    if (num) {
        if (kind != 0) return false;
        memcpy(num, v, 8);
        return *num == *num;   // not NaN
    }
    if (str) {
        uint32_t ref = 0, text = 0;
        memcpy(&ref, v, 4);
        char buf[64] = {};
        if (kind != 1 || !ValidPtr32(ref) || !rd(ref, &text, 4) || !ValidPtr32(text)) return false;
        SIZE_T got = 0;
        ReadProcessMemory(proc, (LPCVOID)(uintptr_t)text, buf, sizeof(buf) - 1, &got);
        *str = std::string(buf, strnlen(buf, got));
        return true;
    }
    return false;
}

void UndertaleModule::FillFromIni(GameSnapshot& s) const
{
    s.gameDataOk = iniOk_;
    s.deaths = ini_.gameOvers;
    s.playTimeMs = (uint32_t)std::min<uint64_t>((uint64_t)ini_.timeFrames * 1000 / 30, 0xFFFFFFFFu);
    s.soulLevel = ini_.love;
    s.saveSlot = 0;   // a single save
    s.lastBonfire = std::max(ini_.room, 0);
    s.name = Utf8ToWideStr(ini_.name);
}

bool UndertaleModule::ReadWithoutProcess(GameSnapshot& s)
{
    PollIni();
    FillFromIni(s);
    s.charLoaded = iniOk_ && !s.name.empty();
    s.inWorld = s.charLoaded;   // no way to tell the title screen apart
    s.areaName = ut::AreaOfRoom(ut::RoomName(ini_.room));   // area of the last save
    return true;
}

bool UndertaleModule::Read(HANDLE proc, GameSnapshot& s)
{
    PollIni();
    if (!dataWinTried_) {
        wchar_t exe[MAX_PATH * 2];
        DWORD n = (DWORD)std::size(exe);
        dataWinTried_ = true;
        if (QueryFullProcessImageNameW(proc, 0, exe, &n)) {
            std::wstring path(exe, n);
            size_t slash = path.find_last_of(L"\\/");
            if (slash != std::wstring::npos) dataWin_.Load(path.substr(0, slash) + L"\\data.win");
        }
    }

    room_ = -1;
    int32_t room = -1;
    SIZE_T got = 0;
    if (roomAddr_ && ReadProcessMemory(proc, (LPCVOID)roomAddr_, &room, 4, &got) && got == 4 && room >= 0 && room < 5000)
        room_ = room;
    std::string roomName = !dataWin_.rooms.empty() ? (room_ >= 0 && room_ < (int)dataWin_.rooms.size() ? dataWin_.rooms[room_] : "")
                                                   : ut::RoomName(room_);
    ut::RoomKind kind = ut::KindOfRoom(roomName);

    battleGroup_ = -1;
    double num = 0;
    if (ReadGlobal(proc, dataWin_.slotBattleGroup, &num, nullptr) && num >= 0 && num < 100000) battleGroup_ = (int)num;
    std::string name;
    globalsOk_ = ReadGlobal(proc, dataWin_.slotCharName, nullptr, &name) && PlausibleName(name);
    charName_ = globalsOk_ ? name : std::string();
    int lv = 0;
    if (globalsOk_ && ReadGlobal(proc, dataWin_.slotLv, &num, nullptr) && num >= 1 && num <= 99) lv = (int)num;

    ctx_.Update(roomName, battleGroup_, GetTickCount64());

    FillFromIni(s);
    // The name in memory is right from the naming screen on; the ini only has it after a save.
    if (!charName_.empty() && kind != ut::RoomKind::Menu) s.name = Utf8ToWideStr(charName_);
    if (lv) s.soulLevel = lv;
    s.charLoaded = iniOk_ && !s.name.empty();
    s.inWorld = room_ >= 0 && kind != ut::RoomKind::Menu;
    s.areaName = ctx_.area();
    if (s.areaName.empty()) s.areaName = ut::AreaOfRoom(ut::RoomName(ini_.room));
    s.cause = ctx_.cause();
    return true;
}

std::string UndertaleModule::VersionLabel(uint32_t) const
{
    return dataWin_.bytecode ? "GameMaker bytecode " + std::to_string(dataWin_.bytecode) : std::string();
}

std::string UndertaleModule::LiveDiagnostics() const
{
    char b[512];
    std::string t;
    std::string roomName = room_ >= 0 && room_ < (int)dataWin_.rooms.size() ? dataWin_.rooms[room_] : ut::RoomName(room_);
    snprintf(b, sizeof(b), "undertale.ini: %s | Gameover %d | Name \"%s\" | LV %d | sala do save %d (%s)\n",
             iniOk_ ? "lido" : "não lido", ini_.gameOvers, ini_.name.c_str(), ini_.love, ini_.room,
             ut::SaveRoomLabel(ini_.room).c_str());
    t += b;
    snprintf(b, sizeof(b), "data.win: %s | %zu salas | slots battlegroup %d, charname %d, lv %d\n",
             dataWin_.error.empty() ? (dataWin_.rooms.empty() ? "não lido" : "ok") : dataWin_.error.c_str(),
             dataWin_.rooms.size(), dataWin_.slotBattleGroup, dataWin_.slotCharName, dataWin_.slotLv);
    t += b;
    snprintf(b, sizeof(b), "Sala %d (%s) | área %s | battlegroup %d (%s) | nome na memória: %s\n", room_,
             roomName.c_str(), ctx_.area().c_str(), battleGroup_, ut::BattleGroupEnemies(battleGroup_).c_str(),
             globalsOk_ ? charName_.c_str() : "(ilegível)");
    t += b;
    if (ctx_.inGameOver()) t += "Tela de GAME OVER: causa \"" + ctx_.cause() + "\"\n";
    return t;
}
