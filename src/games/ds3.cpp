#include "games/ds3.h"
#include "game_reader.h"
#include <cmath>
#include <cstdio>

// How the signatures were found and checked: docs/how-it-was-made.md ("Dark Souls III").
// All of them were derived from the code of DarkSoulsIII.exe 1.15.0 and match exactly once
// there; globals found by several independent signatures must agree, or they count as missing.

namespace {

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

template <class T> bool ReadT(HANDLE proc, uintptr_t addr, T& v)
{
    SIZE_T n = 0;
    return addr && ReadProcessMemory(proc, (LPCVOID)addr, &v, sizeof(T), &n) && n == sizeof(T);
}

uintptr_t ReadPtr(HANDLE proc, uintptr_t addr)
{
    uintptr_t v = 0;
    if (!ReadT(proc, addr, v) || !ValidPtr(v)) return 0;
    return v;
}

// The only match of `pat`, or SIZE_MAX.
size_t FindOne(const std::vector<uint8_t>& img, const char* pat)
{
    auto h = FindPattern(img, pat);
    return h.size() == 1 ? h[0] : SIZE_MAX;
}

// Target of a call/jmp rel32 whose displacement is at `at` (instruction ends at at + 4).
size_t CallTarget(const std::vector<uint8_t>& img, size_t at)
{
    return at + 4 + ImageI32(img, at);
}

// Results of independent signatures for one global: all the ones found must agree.
struct Votes {
    uintptr_t value = 0;
    int found = 0, total = 0;
    bool conflict = false;
    void Add(bool ok, uintptr_t v)
    {
        ++total;
        if (!ok) return;
        if (found && v != value) conflict = true;
        value = v;
        ++found;
    }
    uintptr_t Result() const { return (found && !conflict) ? value : 0; }
};

// --- Signatures (DarkSoulsIII.exe 1.15.0) -------------------------------------------------

// GameDataMan, three independent places:
// SoulMemory's signature.
const char* kGameDataManA = "48 8B 0D ? ? ? ? 4C 8D 44 24 40 45 33 C9 48 8B D3 40 88 74 24 28 44 88 74 24 20";
// The function behind the event-script binding "AddDeathCount": saturating add to the death counter.
const char* kDeathAdder = "48 8B 15 ? ? ? ? 48 85 D2 74 ? 44 8B 82 98 00 00 00 83 C8 FF 2B C1 44 3B C0 76 ? "
                          "C7 82 98 00 00 00 FF FF FF FF C3";
// Play time: mov rcx,[GameDataMan]; ...; add [rcx+time],eax; mov rax,[GameDataMan]; cmp [rax+time],999:59:59.000
const char* kPlayTime = "48 8B 0D ? ? ? ? 0F 28 C6 F3 0F 59 05 ? ? ? ? F3 48 0F 2C C0 01 81 ? ? ? ? "
                        "48 8B 05 ? ? ? ? 81 B8 ? ? ? ? 18 A0 93 D6";
// WorldChrMan: SoulMemory's signature, and a small "is the player present" function ([WorldChrMan + player] != 0).
const char* kWorldChrManA = "48 8B 1D ? ? ? ? 48 8B F9 48 85 DB ? ? 8B 11 85 D2 ? ? 8D";
const char* kWorldChrManB = "48 8B 0D ? ? ? ? 32 C0 48 85 C9 74 ? 48 83 B9 ? ? ? ? 00 0F B6 C0 BA 01 00 00 00 0F 45 C2 F3 C3";
// GameMan: SetRespawnPoint (also clears the temporary override right after it) and GetRespawnPoint
// (override if >= 0, else the respawn point), plus the new-game code that stores the first free save slot.
const char* kSetRespawn = "48 8B 05 ? ? ? ? 8B D9 89 88 ? ? ? ? C7 80 ? ? ? ? FF FF FF FF 85 C9 78";
const char* kGetRespawn = "48 8B 0D ? ? ? ? 84 C0 74 ? 83 B9 ? ? ? ? 00 7D ? B8 ? ? ? ? 48 83 C4 28 C3 "
                          "8B 81 ? ? ? ? 85 C0 79 ? 8B 81";
const char* kNewGameSlot = "8B D8 EB 03 83 CB FF 8B CB E8 ? ? ? ? 83 FB FF 74 ? 8B D3 48 8B CF E8";
const char* kSlotSetter = "48 8B 05 ? ? ? ? 89 88 ? ? ? ? C3";
// HP: a ChrIns method passing [modules + data module] to SprjChrDataModule::SetHp, which clamps to max HP.
const char* kSetHpCaller = "48 8B 8B ? ? ? ? 33 D2 48 8B 49 ? E8";
const char* kSetHp = "89 54 24 10 53 48 83 EC 20 48 8B D9 48 81 C1 ? ? ? ? C7 44 24 30 00 00 00 00 85 D2 "
                     "79 07 48 8D 44 24 30 EB 0B 3B 11 48 8D 44 24 38 48 0F 4F C1 8B 00 89 83";
// Current map: the play region of the floor collision under a character, stored when the floor changes.
const char* kPlayRegion = "E8 ? ? ? ? 8B 08 89 8E ? ? ? ? 48 8B CB E8 ? ? ? ? 85 C0 78 06 88 86";
// FieldArea (SoulMemory's signature) and "current map id": FieldArea -> world info + loaded block index.
const char* kFieldAreaA = "4C 8B 3D ? ? ? ? 8B 45 87 83 F8 FF 74 69 48 8D 4D 8F 48 89 4D 9F 89 45 8F 48 8D 55 8F 49 8B 4F 10";
const char* kFieldAreaB = "48 8B 0D ? ? ? ? 48 85 C9 74 ? 44 8B 41 ? 48 8B 49 ? 48 8D 54 24 ? E8 ? ? ? ? "
                          "8B 4C 24 ? 8B C1 C1 E9 18 C1 E8 10 0F B6 D0 E8";
// World info: index check against the block count, block = list + index * stride, then the block's map id getter.
const char* kBlockLookup = "45 85 C0 78 ? 44 3B 41 ? 7D ? 49 63 C8 48 8D 54 24 ? BB 01 00 00 00 89 5C 24 ? "
                           "48 6B C9 ? 48 03 48 ? E8";
const char* kBlockMapId = "8B 41 ? 89 02 48 8B C2 C3";

} // namespace

std::string Ds3Reader::VersionLabel(uint32_t moduleSize)
{
    return moduleSize == 0x63DF000 ? "1.15" : "";   // other builds: the exe's file version is shown
}

bool Ds3Reader::Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log)
{
    *this = Ds3Reader();   // analysis is retried while the game starts up
    char line[256];
    auto rip = [&](size_t at, int relOff, int len) { return base + at + len + ImageI32(img, at + relOff); };
    auto logGlobal = [&](const char* name, const Votes& v) {
        signaturesFound += v.found;
        signaturesTotal += v.total;
        if (v.conflict)
            snprintf(line, sizeof(line), "%-12s -> DIVERGENTE (assinaturas discordam)\n", name);
        else if (!v.found)
            snprintf(line, sizeof(line), "%-12s -> NÃO ENCONTRADO\n", name);
        else
            snprintf(line, sizeof(line), "%-12s -> exe+%llX (%d/%d assinaturas)\n", name,
                     (unsigned long long)(v.value - base), v.found, v.total);
        log += line;
    };
    auto count = [&](bool ok) { ++signaturesTotal; if (ok) ++signaturesFound; return ok; };

    Votes gdm;
    size_t h = FindOne(img, kGameDataManA);
    gdm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    h = FindOne(img, kDeathAdder);
    gdm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    h = FindOne(img, kPlayTime);
    bool timeOk = h != SIZE_MAX && rip(h, 3, 7) == rip(h + 29, 3, 7) && ImageI32(img, h + 25) == ImageI32(img, h + 38);
    gdm.Add(timeOk, timeOk ? rip(h, 3, 7) : 0);
    if (timeOk) off.playTime = ImageI32(img, h + 25);
    gameDataMan = gdm.Result();
    logGlobal("GameDataMan", gdm);

    Votes wcm;
    h = FindOne(img, kWorldChrManA);
    wcm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    h = FindOne(img, kWorldChrManB);
    wcm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) off.playerIns = ImageI32(img, h + 17);
    worldChrMan = wcm.Result();
    logGlobal("WorldChrMan", wcm);

    Votes gm;
    int respawnA = 0, respawnB = 0;
    h = FindOne(img, kSetRespawn);
    gm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) respawnA = ImageI32(img, h + 11);
    h = FindOne(img, kGetRespawn);
    gm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) respawnB = ImageI32(img, h + 13);
    bool slotOk = false;
    h = FindOne(img, kNewGameSlot);
    if (h != SIZE_MAX) {
        size_t setter = CallTarget(img, h + 10);
        if (PatternAt(img, setter, kSlotSetter)) {
            gm.Add(true, rip(setter, 3, 7));
            off.saveSlot = ImageI32(img, setter + 9);
            slotOk = true;
        }
    }
    if (!slotOk) gm.Add(false, 0);
    if (respawnA && respawnB && respawnA != respawnB) gm.conflict = true;
    if (respawnA || respawnB) off.respawn = respawnA ? respawnA : respawnB;
    gameMan = gm.Result();
    logGlobal("GameMan", gm);

    bool hpOk = false;
    h = FindOne(img, kSetHpCaller);
    if (h != SIZE_MAX) {
        size_t setHp = CallTarget(img, h + 14);
        if (PatternAt(img, setHp, kSetHp)) {
            off.modules = ImageI32(img, h + 3);
            off.dataModule = img[h + 12];
            off.hp = ImageI32(img, setHp + 53);
            hpOk = true;
        }
    }
    count(hpOk);
    count(hpOk);   // the caller and SetHp itself
    h = FindOne(img, kPlayRegion);
    bool regionOk = count(h != SIZE_MAX);
    if (regionOk) off.playRegion = ImageI32(img, h + 9);

    Votes fa;
    h = FindOne(img, kFieldAreaA);
    fa.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    int worldInfoA = h != SIZE_MAX ? img[h + 33] : -1;
    h = FindOne(img, kFieldAreaB);
    fa.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) {
        off.blockIndex = img[h + 15];
        off.worldInfo = img[h + 19];
        if (worldInfoA >= 0 && worldInfoA != off.worldInfo) fa.conflict = true;
    }
    fieldArea = fa.Result();
    logGlobal("FieldArea", fa);
    h = FindOne(img, kBlockLookup);
    if (h != SIZE_MAX && PatternAt(img, CallTarget(img, h + 37), kBlockMapId)) {
        off.blockCount = img[h + 8];
        off.blockStride = img[h + 31];
        off.blockList = img[h + 35];
        off.blockMapId = img[CallTarget(img, h + 37) + 2];
        hasBlockMap_ = true;
    }
    count(hasBlockMap_);

    const char* fb = " (padrão)";
    snprintf(line, sizeof(line), "DS3 | mortes +%X | tempo +%X%s | jogador +%X | slot +%X%s | fogueira +%X\n",
             off.deaths, off.playTime, timeOk ? "" : fb, off.playerIns, off.saveSlot, slotOk ? "" : fb, off.respawn);
    log += line;
    snprintf(line, sizeof(line), "DS3 | módulos +%X/+%X | hp +%X%s | região +%X%s | bloco +%X/+%X%s\n", off.modules,
             off.dataModule, off.hp, hpOk ? "" : fb, off.playRegion, regionOk ? "" : fb, off.worldInfo, off.blockIndex,
             hasBlockMap_ ? "" : fb);
    log += line;
    snprintf(line, sizeof(line), "DS3 | assinaturas encontradas: %d/%d\n", signaturesFound, signaturesTotal);
    log += line;

    // Counting needs GameDataMan; WorldChrMan tells when a character is actually in the world.
    return gameDataMan && worldChrMan;
}

bool Ds3Reader::Read(HANDLE proc, GameSnapshot& s)
{
    lastBlockMap_ = 0;
    if (uintptr_t gdm = ReadPtr(proc, gameDataMan)) {
        s.gameDataOk = true;
        ReadT(proc, gdm + off.deaths, s.deaths);
        ReadT(proc, gdm + off.playTime, s.playTimeMs);
        ReadT(proc, gdm + 0x78, s.ngPlus);
        if (s.ngPlus < 0 || s.ngPlus > 9999) s.ngPlus = 0;
        uintptr_t pgd = ReadPtr(proc, gdm + 0x10);   // PlayerGameData
        if (pgd) {
            wchar_t nameBuf[16] = {};
            SIZE_T n = 0;
            if (ReadProcessMemory(proc, (LPCVOID)(pgd + 0x88), nameBuf, sizeof(nameBuf), &n) && n == sizeof(nameBuf)) {
                std::wstring name;
                bool ok = true;
                for (int i = 0; i < 16 && nameBuf[i]; ++i) {
                    if (nameBuf[i] < 0x20 || nameBuf[i] == 0xFFFF) { ok = false; break; }
                    name += nameBuf[i];
                }
                if (ok) s.name = name;
            }
            ReadT(proc, pgd + 0x70, s.soulLevel);
            if (s.soulLevel < 0 || s.soulLevel > 10000) s.soulLevel = 0;
        }
        // The name is only used to tell characters apart; counting works without it.
        s.charLoaded = pgd && s.playTimeMs > 0 && s.deaths >= 0 && s.deaths < 10000000;
    }

    if (uintptr_t gm = ReadPtr(proc, gameMan)) {
        ReadT(proc, gm + off.saveSlot, s.saveSlot);
        if (s.saveSlot < 0 || s.saveSlot > 9) s.saveSlot = -1;
        ReadT(proc, gm + off.respawn, s.lastBonfire);
        if (s.lastBonfire < 0) s.lastBonfire = 0;
    }

    // Map block loaded by FieldArea (set when a map loads or the player warps). Map ids are AA BB CC DD,
    // area in the high byte. Only a fallback: the play region below follows the player across seamless areas.
    int area = -1, block = -1;
    if (uintptr_t fa = ReadPtr(proc, fieldArea); fa && hasBlockMap_) {
        uintptr_t worldInfo = ReadPtr(proc, fa + off.worldInfo);
        int index = -1, count = 0;
        uintptr_t list = 0;
        if (worldInfo && ReadT(proc, fa + off.blockIndex, index) && ReadT(proc, worldInfo + off.blockCount, count) &&
            index >= 0 && index < count && (list = ReadPtr(proc, worldInfo + off.blockList)) != 0) {
            uint32_t mapId = 0;
            if (ReadT(proc, list + (uintptr_t)index * off.blockStride + off.blockMapId, mapId) && mapId != 0xFFFFFFFF) {
                lastBlockMap_ = mapId;
                area = (mapId >> 24) & 0xFF;
                block = (mapId >> 16) & 0xFF;
            }
        }
    }

    uintptr_t wcm = ReadPtr(proc, worldChrMan);
    if (uintptr_t pins = wcm ? ReadPtr(proc, wcm + off.playerIns) : 0) {
        s.inWorld = true;
        if (uintptr_t modules = ReadPtr(proc, pins + off.modules)) {
            if (uintptr_t data = ReadPtr(proc, modules + off.dataModule)) {
                ReadT(proc, data + off.hp, s.hp);
                if (s.hp < 0 || s.hp > 100000) s.hp = -1;
            }
            // Module 13 is SprjChrPhysicsModule; its position is a float4 (x, height, z, 1).
            if (uintptr_t physics = ReadPtr(proc, modules + 0x68)) {
                float p[3];
                SIZE_T n = 0;
                if (ReadProcessMemory(proc, (LPCVOID)(physics + 0x80), p, sizeof(p), &n) && n == sizeof(p) &&
                    std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]) &&
                    std::fabs(p[0]) < 1e5f && std::fabs(p[1]) < 1e5f && std::fabs(p[2]) < 1e5f) {
                    s.pos = { p[0], p[1], p[2] };
                    s.hasPos = true;
                }
            }
        }
        // The play region id is a decimal number AABKCC: AA = map area, B = map block.
        int region = 0;
        if (ReadT(proc, pins + off.playRegion, region) && region > 0) {
            int a = region / 10000 % 100, b = region / 1000 % 10;
            if (a >= 30 && a <= 69) { area = a; block = b; }
        }
        lastRegion_ = region;
    } else {
        lastRegion_ = 0;
    }

    if (area >= 0) {
        s.mapArea = area;
        s.mapBlock = block;
        s.mapRaw = ((uint32_t)area << 24) | ((uint32_t)block << 16);   // same packing as the game's map ids
        s.areaName = Ds3MapName(area, block);
        if (s.areaName.empty()) {
            char b[32];
            snprintf(b, sizeof(b), "Mapa m%02d_%02d", area, block);
            s.areaName = b;
        }
    }
    return true;
}

std::string Ds3Reader::LiveDiagnostics() const
{
    char b[160];
    snprintf(b, sizeof(b), "DS3 | região do chão: %d | bloco carregado (FieldArea): ", lastRegion_);
    std::string t = b;
    if (lastBlockMap_) {
        snprintf(b, sizeof(b), "m%02u_%02u_%02u_%02u\n", (lastBlockMap_ >> 24) & 0xFF, (lastBlockMap_ >> 16) & 0xFF,
                 (lastBlockMap_ >> 8) & 0xFF, lastBlockMap_ & 0xFF);
        t += b;
    } else {
        t += "-\n";
    }
    return t;
}

namespace {
std::unique_ptr<GameModule> CreateDs3() { return std::make_unique<Ds3Module>(); }
}

// Bonfire / respawn point ids only tell the map they belong to, so the column shows that map.
const GameInfo kDs3Info = {
    "ds3", L"DarkSoulsIII.exe", "Dark Souls III", "DS3", CreateDs3,
    Ds3BonfireName, Ds3AllMapNames, Ds3CauseSuggestions, "Última fogueira", true, true, false,
};
