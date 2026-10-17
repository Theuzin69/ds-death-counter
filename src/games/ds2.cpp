#include "ds2.h"
#include "game_reader.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ds2 {
namespace {

// Size of the Steam 1.02 module (the build the signatures were checked against).
constexpr uint32_t kSize102 = 0x20B6000;

int32_t I32(const std::vector<uint8_t>& img, size_t at)
{
    int32_t v;
    memcpy(&v, img.data() + at, 4);
    return v;
}

// Byte signatures, each checked to match exactly once in DarkSoulsII.exe (tests/scan_test).
// '?' bytes are operands that may change between game patches; the offsets the counter
// needs are read out of them.
const char* kGameManA = "48 8B 35 ? ? ? ? 48 8B E9 48 85 F6";
const char* kGameManB = "48 8B 05 ? ? ? ? 48 8B 58 38 48 85 DB 74 ? F6";
// SaveSlotSummary::GetPlayTimeSeconds(): summaries[slot].playTime + (QPC now - stamp) / QPF
const char* kPlayTime = "48 63 99 ? ? ? ? 48 8B F9 48 69 DB ? ? ? ? 48 03 D9 75 ? 33 C0 48 8B 5C 24 ? 48 83 C4 ? 5F C3 "
                        "48 8D 4C 24 ? FF 15 ? ? ? ? 48 8B 44 24 ? 33 D2 48 2B 87 ? ? ? ? 48 F7 35 ? ? ? ? 03 83 ? ? ? ?";
// Save-summary update wrapper: GameManagerImp -> +0xA8 (game data) -> +0xD8 (slot summaries)
const char* kSlotSummaries = "48 8B 1D ? ? ? ? 41 8B F8 48 8B F2 48 85 DB 74 ? 48 8B 9B ? ? ? ? 48 85 DB 74 ? "
                             "48 8B 9B ? ? ? ? 48 85 DB 74 ? 45 85 C0 75 ? 48 8B CB E8 ? ? ? ? 41 B9 01 00 00 00";
// GameManagerImp -> PlayerCtrl (+0xD0) -> PlayerParam (+0x490)
const char* kPlayer = "48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B B1 ? ? ? ? B9 ? ? ? ? E8";
// PlayerParam loaded from the save: total deaths (+0x1A4) right before the counter array (+0x104)
const char* kDeaths = "44 38 9F ? ? ? ? 75 ? C7 45 ? FF E0 F5 05 44 89 5D ? 85 C9 79 ? 48 8D 45 ? EB ? 48 8D 45 ? "
                      "48 8D 55 ? 81 F9 FF E0 F5 05 48 0F 4E C2 8B 00 89 87 ? ? ? ? 48 8D 8F ? ? ? ?";
// Same loader: [PlayerParam] -> PlayerCtrl, HP block at +0x168
const char* kHp = "48 8B 17 8B 4B ? 48 81 C2 ? ? ? ? 39 4A 0C";
// Character info copied into the save summary: +0xC0 info, name +0x24, class +0x64, NG cycle +0x68
const char* kCharInfo = "48 8B 9B ? ? ? ? 48 85 DB 0F 84 ? ? ? ? 48 8D 53 ? 48 8D 4C 24 ? 41 B8 1F 00 00 00 "
                        "E8 ? ? ? ? 8B 43 ? 48 8D 54 24 ? 89 84 24 ? ? ? ? 8B 43 ?";
// Save summary location: GameManagerImp -> +0x38 (map manager) -> current place name id
const char* kMapMan = "48 8B 05 ? ? ? ? 48 8B 48 ? 48 85 C9 74 ? E8 ? ? ? ? 44 8B C0 85 C0 78 ?";
const char* kPlace = "44 8B 81 ? ? ? ? 48 8D 54 24 ? 48 8B D9 E8 ? ? ? ? 8B 44 24 ? 25 00 00 00 FF "
                     "3D 00 00 00 32 75 ? 8B 83 ? ? ? ? 48 83 C4 ? 5B C3 8B 83 ? ? ? ?";
const char* kMapTable = "48 8B 41 ? 48 85 C0 74 ? 45 85 C0 79 ? 44 3B 40 ? 7D ? 48 8B 40 ? 49 63 C8 8B 0C 88";
// Save summary location from the last bonfire: GameManagerImp -> +0x70, area +0x164, type +0x168, id +0x16C
const char* kBonfire = "48 8B 58 ? 48 85 DB 0F 84 ? ? ? ? 48 8D 8B ? ? ? ? E8 ? ? ? ? 84 C0 74 ? C7 85 ? ? ? ? ? ? ? ? "
                       "E9 ? ? ? ? 83 BB ? ? ? ? 02";
const char* kBonfireId = "44 39 B3 ? ? ? ? 75 ? 0F B7 93 ? ? ? ? 48 8D 4C 24 ? E8";
const char* kLoadState = "48 89 05 ? ? ? ? B0 01 48 83 C4 28";

template <class T> bool Rd(HANDLE p, uintptr_t addr, T& v)
{
    SIZE_T n = 0;
    return addr && ReadProcessMemory(p, (LPCVOID)addr, &v, sizeof(T), &n) && n == sizeof(T);
}

uintptr_t Ptr(HANDLE p, uintptr_t addr)
{
    uintptr_t v = 0;
    if (!Rd(p, addr, v) || v < 0x10000 || v >= 0x7FFFFFFFFFFFull) return 0;
    return v;
}

} // namespace

bool Module::Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t size, std::string& log)
{
    (void)size;
    L_ = Layout{};
    char line[256];
    auto add = [&](const char* fmt, auto... args) {
        snprintf(line, sizeof(line), fmt, args...);
        log += line;
    };
    // Unique hit of a pattern, or npos.
    auto one = [&](const char* pat) -> size_t {
        auto hits = FindPattern(img, pat);
        return hits.size() == 1 ? hits[0] : std::string::npos;
    };
    // Reads an offset operand when the pattern was found and the value looks sane.
    auto offset = [&](size_t hit, int at, bool byte, int lo, int hi, int& out) {
        if (hit == std::string::npos) return false;
        int v = byte ? img[hit + at] : I32(img, hit + at);
        if (v < lo || v > hi) return false;
        out = v;
        return true;
    };
    auto rip = [&](size_t hit, int at, int len) { return base + hit + len + I32(img, hit + at); };

    // GameManagerImp: two independent code sites must agree.
    size_t a = one(kGameManA), b = one(kGameManB);
    uintptr_t ga = a != std::string::npos ? rip(a, 3, 7) : 0;
    uintptr_t gb = b != std::string::npos ? rip(b, 3, 7) : 0;
    if (ga && gb && ga != gb) {
        add("GameManagerImp -> DIVERGENTE (exe+%llX / exe+%llX)\n", (unsigned long long)(ga - base),
            (unsigned long long)(gb - base));
        return false;
    }
    L_.gameManager = ga ? ga : gb;
    if (!L_.gameManager) {
        add("GameManagerImp -> NÃO ENCONTRADO\n");
        return false;
    }
    add("GameManagerImp -> exe+%llX (%s)\n", (unsigned long long)(L_.gameManager - base),
        ga && gb ? "2 assinaturas" : "1 assinatura");

    // Offsets read from the code. When a pattern isn't found the 1.02 value stays and the
    // log says so ("padrão").
    auto mark = [](bool ok) { return ok ? "" : "*"; };
    size_t h = one(kPlayer);
    bool okCtrl = offset(h, 3, false, 0x10, 0x400, L_.offPlayerCtrl) && offset(h, 15, false, 0x10, 0x1000, L_.offPlayerParam);
    h = one(kDeaths);
    bool okDeaths = offset(h, 52, false, 0x100, 0x400, L_.offDeaths) && offset(h, 59, false, 0x80, 0x400, L_.offCounters);
    bool okHp = offset(one(kHp), 9, false, 0x40, 0x400, L_.offHp);
    add("PlayerCtrl +%X%s | PlayerParam +%X%s | mortes +%X%s (contadores +%X) | HP +%X%s\n", L_.offPlayerCtrl, mark(okCtrl),
        L_.offPlayerParam, mark(okCtrl), L_.offDeaths, mark(okDeaths), L_.offCounters, L_.offHp, mark(okHp));

    h = one(kSlotSummaries);
    bool okSumm = h != std::string::npos && rip(h, 3, 7) == L_.gameManager &&
                  offset(h, 21, false, 0x10, 0x400, L_.offGameData) && offset(h, 33, false, 0x10, 0x400, L_.offSlotSummaries);
    h = one(kPlayTime);
    bool okTime = offset(h, 3, false, 0x100, 0x10000, L_.offSlotIndex) && offset(h, 13, false, 0x40, 0x1000, L_.summarySize) &&
                  offset(h, 56, false, 0x100, 0x10000, L_.offTimeStamp) && offset(h, 69, false, 0x10, 0x1000, L_.offPlayTime);
    h = one(kCharInfo);
    bool okInfo = offset(h, 3, false, 0x10, 0x400, L_.offCharInfo) && offset(h, 19, true, 0, 0xFF, L_.offName) &&
                  offset(h, 53, true, 0, 0xFF, L_.offNgCycle);
    add("dados +%X%s | resumos +%X%s | slot +%X tempo +%X (passo %X)%s | info +%X nome +%X NG +%X%s\n", L_.offGameData,
        mark(okSumm), L_.offSlotSummaries, mark(okSumm), L_.offSlotIndex, L_.offPlayTime, L_.summarySize, mark(okTime),
        L_.offCharInfo, L_.offName, L_.offNgCycle, mark(okInfo));

    h = one(kMapMan);
    bool okMap = h != std::string::npos && rip(h, 3, 7) == L_.gameManager && offset(h, 10, true, 0, 0xFF, L_.offMapMan);
    h = one(kPlace);
    okMap = okMap && offset(h, 3, false, 0x40, 0x1000, L_.offMapIndex) && offset(h, 38, false, 0x40, 0x1000, L_.offPlaceDlc) &&
            offset(h, 50, false, 0x40, 0x1000, L_.offPlaceBase);
    h = one(kMapTable);
    okMap = okMap && offset(h, 3, true, 0, 0xFF, L_.offMapTable) && offset(h, 17, true, 0, 0xFF, L_.offMapCount) &&
            offset(h, 23, true, 0, 0xFF, L_.offMapIds);
    h = one(kBonfire);
    bool okBonfire = offset(h, 3, true, 0, 0xFF, L_.offEventMan) && offset(h, 16, false, 0x40, 0x1000, L_.offBonfireArea);
    okBonfire = offset(one(kBonfireId), 12, false, 0x40, 0x1000, L_.offBonfireId) && okBonfire;
    add("mapa +%X idx +%X lugar +%X/+%X%s | fogueira +%X id +%X%s\n", L_.offMapMan, L_.offMapIndex, L_.offPlaceBase,
        L_.offPlaceDlc, mark(okMap), L_.offEventMan, L_.offBonfireId, mark(okBonfire));

    h = one(kLoadState);
    if (h != std::string::npos) L_.loadState = rip(h, 3, 7);
    if (!(okCtrl && okDeaths && okHp && okSumm && okTime && okInfo && okMap && okBonfire))
        add("* = assinatura não encontrada: usando o valor da versão 1.02\n");
    return true;
}

bool Module::Read(HANDLE p, GameSnapshot& s)
{
    counter0_ = placeId_ = mapIndex_ = ngRaw_ = bonfireArea_ = loading_ = -1;
    savedSeconds_ = 0;
    uintptr_t gm = Ptr(p, L_.gameManager);
    if (!gm) return true;   // still booting
    s.gameDataOk = true;

    uintptr_t ctrl = Ptr(p, gm + L_.offPlayerCtrl);
    uintptr_t param = ctrl ? Ptr(p, ctrl + L_.offPlayerParam) : 0;
    uintptr_t data = Ptr(p, gm + L_.offGameData);
    uintptr_t info = data ? Ptr(p, data + L_.offCharInfo) : 0;
    uintptr_t summ = data ? Ptr(p, data + L_.offSlotSummaries) : 0;

    if (param) {
        Rd(p, param + L_.offDeaths, s.deaths);
        Rd(p, param + L_.offCounters, counter0_);
        uint16_t sl = 0;
        if (Rd(p, param + L_.offSoulLevel, sl) && sl < 1000) s.soulLevel = sl;
    }
    if (info) {
        wchar_t buf[32] = {};
        SIZE_T n = 0;
        if (ReadProcessMemory(p, (LPCVOID)(info + L_.offName), buf, sizeof(buf), &n)) {
            std::wstring name;
            bool ok = true;
            for (int i = 0; i < 31 && buf[i]; ++i) {
                if (buf[i] < 0x20 || buf[i] == 0xFFFF) { ok = false; break; }
                name += buf[i];
            }
            if (ok) s.name = name;
        }
        // Stored as 1 for the first playthrough.
        if (Rd(p, info + L_.offNgCycle, ngRaw_) && ngRaw_ >= 1 && ngRaw_ < 1000) s.ngPlus = ngRaw_ - 1;
    }
    if (summ) {
        int slot = -1;
        if (Rd(p, summ + L_.offSlotIndex, slot) && slot >= 0 && slot <= 9) {
            s.saveSlot = slot;
            // Same formula as the game's own GetPlayTimeSeconds(): the play time stored in the
            // save summary plus the time since it was last stored. QueryPerformanceCounter is
            // system-wide, so the game's time stamp can be compared with ours.
            uint32_t secs = 0;
            int64_t stamp = 0;
            if (Rd(p, summ + (uintptr_t)slot * L_.summarySize + L_.offPlayTime, secs) && secs < 3600000) {
                savedSeconds_ = secs;
                uint64_t total = secs;
                LARGE_INTEGER now{}, freq{};
                if (Rd(p, summ + L_.offTimeStamp, stamp) && stamp > 0 && QueryPerformanceCounter(&now) &&
                    QueryPerformanceFrequency(&freq) && freq.QuadPart > 0 && now.QuadPart > stamp)
                    total += (uint64_t)(now.QuadPart - stamp) / (uint64_t)freq.QuadPart;
                s.playTimeMs = (uint32_t)(std::min<uint64_t>(total, 3599999) * 1000);
            }
        }
    }
    s.charLoaded = param && info && s.saveSlot >= 0 && s.playTimeMs > 0 && s.deaths >= 0 && s.deaths < 100000000;

    // PlayerCtrl exists only while a character is in the world.
    s.inWorld = ctrl && param;
    if (ctrl) {
        Rd(p, ctrl + L_.offHp, s.hp);
        if (s.hp < 0 || s.hp > 100000) s.hp = -1;
        if (uintptr_t pos = Ptr(p, ctrl + L_.offPosData)) {
            float f[3];
            if (Rd(p, pos + L_.offPos, f) && std::isfinite(f[0]) && std::isfinite(f[1]) && std::isfinite(f[2]) &&
                std::fabs(f[0]) < 1e5f && std::fabs(f[1]) < 1e5f && std::fabs(f[2]) < 1e5f) {
                s.pos = { f[0], f[1], f[2] };
                s.hasPos = true;
            }
        }
    }
    if (L_.loadState)
        if (uintptr_t ls = Ptr(p, L_.loadState)) Rd(p, ls + 0x11C, loading_);

    // Current map: map index -> map id table (mAA_BB_CC_DD packed as AA BB CC DD).
    if (uintptr_t mm = Ptr(p, gm + L_.offMapMan)) {
        uintptr_t tbl = Ptr(p, mm + L_.offMapTable);
        uintptr_t ids = tbl ? Ptr(p, tbl + L_.offMapIds) : 0;
        int count = 0;
        if (Rd(p, mm + L_.offMapIndex, mapIndex_) && ids && Rd(p, tbl + L_.offMapCount, count) && mapIndex_ >= 0 &&
            mapIndex_ < count && count < 4096 && Rd(p, ids + (uintptr_t)mapIndex_ * 4, s.mapRaw)) {
            int area = (s.mapRaw >> 24) & 0xFF, block = (s.mapRaw >> 16) & 0xFF;
            if (area >= 10 && area <= 60) {
                s.mapArea = area;
                s.mapBlock = block;
                s.areaName = MapName(area, block);
                if (s.areaName.empty()) {
                    char b[32];
                    snprintf(b, sizeof(b), "Mapa m%02d_%02d", area, block);
                    s.areaName = b;
                }
            }
            // The game shows DLC maps (0x32xxxxxx = m50) with the second place name id.
            Rd(p, mm + ((s.mapRaw & 0xFF000000) == 0x32000000 ? L_.offPlaceDlc : L_.offPlaceBase), placeId_);
        }
    }
    if (uintptr_t ev = Ptr(p, gm + L_.offEventMan)) {
        uint16_t id = 0;
        if (Rd(p, ev + L_.offBonfireId, id)) s.lastBonfire = id;
        Rd(p, ev + L_.offBonfireArea, bonfireArea_);
    }
    return true;
}

std::string Module::VersionLabel(uint32_t moduleSize) const
{
    return moduleSize == kSize102 ? "Scholar of the First Sin 1.02" : "Scholar of the First Sin";
}

std::string Module::LiveDiagnostics() const
{
    char b[400];
    snprintf(b, sizeof(b),
             "DS2: contador[0] %d (deve ser igual às mortes) | tempo no save %us | NG bruto %d | mapa idx %d | "
             "lugar %d | área da última fogueira 0x%08X | carregando %d\n",
             counter0_, savedSeconds_, ngRaw_, mapIndex_, placeId_, (unsigned)bonfireArea_, loading_);
    return b;
}

} // namespace ds2

namespace {
std::unique_ptr<GameModule> CreateDs2() { return std::make_unique<ds2::Module>(); }
}

const GameInfo kDs2Info = {
    "ds2", L"DarkSoulsII.exe", "Dark Souls II: Scholar of the First Sin", "DS2", CreateDs2,
    ds2::BonfireName, ds2::AllMapNames, ds2::CauseSuggestions,
};
