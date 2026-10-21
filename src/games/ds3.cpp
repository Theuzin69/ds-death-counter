#include "games/ds3.h"
#include "game_reader.h"
#include <cmath>
#include <cstdio>
#include <cstring>

// Every global and offset below was found in DarkSoulsIII.exe 1.15 and checked against the
// exe file with tests/scan_test. Each signature matches exactly once; when several point at
// the same thing they must agree, otherwise that item is treated as not found.

namespace {

int32_t I32(const std::vector<uint8_t>& img, size_t at)
{
    int32_t v = 0;
    if (at + 4 <= img.size()) memcpy(&v, img.data() + at, 4);
    return v;
}

// Offset of the single match of `pattern` (or SIZE_MAX if it matches 0 or 2+ times).
size_t FindOne(const std::vector<uint8_t>& img, const char* pattern)
{
    auto hits = FindPattern(img, pattern);
    return hits.size() == 1 ? hits[0] : SIZE_MAX;
}

// True if the bytes at `at` match `pattern` (used to check the target of a call).
bool MatchAt(const std::vector<uint8_t>& img, size_t at, const char* pattern)
{
    for (size_t h : FindPattern(img, pattern))
        if (h == at) return true;
    return false;
}

// Collects the results of independent signatures for one value: all found ones must agree.
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

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

} // namespace

bool Ds3Reader::Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log)
{
    *this = Ds3Reader{};   // analysis is retried while the game starts up
    char line[256];
    auto rip = [&](size_t at, int relOff, int len) { return base + at + len + I32(img, at + relOff); };
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

    // GameDataMan: per-character data. Three independent places in the code.
    Votes gdm;
    size_t h = FindOne(img, "48 8B 0D ? ? ? ? 4C 8D 44 24 40 45 33 C9 48 8B D3 40 88 74 24 28 44 88 74 24 20");
    gdm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    // The function behind the Lua binding "AddDeathCount": saturating add to GameDataMan+0x98.
    h = FindOne(img, "48 8B 15 ? ? ? ? 48 85 D2 74 ? 44 8B 82 98 00 00 00 83 C8 FF 2B C1 44 3B C0 76 ? "
                     "C7 82 98 00 00 00 FF FF FF FF C3");
    gdm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    // Play time: frame time added to GameDataMan+0xA4, capped at 999:59:59.000 (0xD693A018 ms).
    h = FindOne(img, "48 8B 0D ? ? ? ? 0F 28 C6 F3 0F 59 05 ? ? ? ? F3 48 0F 2C C0 01 81 ? ? ? ? "
                     "48 8B 05 ? ? ? ? 81 B8 ? ? ? ? 18 A0 93 D6");
    if (h != SIZE_MAX && rip(h, 3, 7) == rip(h + 29, 3, 7) && I32(img, h + 25) == I32(img, h + 38)) {
        gdm.Add(true, rip(h, 3, 7));
        off.playTime = I32(img, h + 25);
    } else {
        gdm.Add(false, 0);
    }
    gameDataMan = gdm.Result();
    logGlobal("GameDataMan", gdm);

    // WorldChrMan: the player character. The second signature is a small "is the player
    // present" function that also gives the PlayerIns offset.
    Votes wcm;
    h = FindOne(img, "48 8B 1D ? ? ? ? 48 8B F9 48 85 DB ? ? 8B 11 85 D2 ? ? 8D");
    wcm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    h = FindOne(img, "48 8B 0D ? ? ? ? 32 C0 48 85 C9 74 ? 48 83 B9 ? ? ? ? 00 0F B6 C0 BA 01 00 00 00 0F 45 C2 F3 C3");
    wcm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) off.playerIns = I32(img, h + 17);
    worldChrMan = wcm.Result();
    logGlobal("WorldChrMan", wcm);

    // GameMan: save slot and respawn point (last bonfire).
    Votes gm;
    int respawnA = 0, respawnB = 0;
    h = FindOne(img, "48 8B 05 ? ? ? ? 8B D9 89 88 ? ? ? ? C7 80 ? ? ? ? FF FF FF FF 85 C9 78");   // SetRespawnPoint
    gm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) respawnA = I32(img, h + 11);
    h = FindOne(img, "48 8B 0D ? ? ? ? 84 C0 74 ? 83 B9 ? ? ? ? 00 7D ? B8 ? ? ? ? 48 83 C4 28 C3 "
                     "8B 81 ? ? ? ? 85 C0 79 ? 8B 81");                                            // GetRespawnPoint
    gm.Add(h != SIZE_MAX, h != SIZE_MAX ? rip(h, 3, 7) : 0);
    if (h != SIZE_MAX) respawnB = I32(img, h + 13);
    // New game: slot = first free save slot (or -1), stored by a one-line setter.
    h = FindOne(img, "8B D8 EB 03 83 CB FF 8B CB E8 ? ? ? ? 83 FB FF 74 ? 8B D3 48 8B CF E8");
    bool slotOk = false;
    if (h != SIZE_MAX) {
        size_t setter = h + 14 + I32(img, h + 10);
        if (setter + 14 <= img.size() && MatchAt(img, setter, "48 8B 05 ? ? ? ? 89 88 ? ? ? ? C3")) {
            gm.Add(true, rip(setter, 3, 7));
            off.saveSlot = I32(img, setter + 9);
            slotOk = true;
        }
    }
    if (!slotOk) gm.Add(false, 0);
    if (respawnA && respawnB && respawnA != respawnB) gm.conflict = true;
    if (respawnA || respawnB) off.respawn = respawnA ? respawnA : respawnB;
    gameMan = gm.Result();
    logGlobal("GameMan", gm);

    // HP: a caller of ChrDataModule::SetHp gives the module offsets, SetHp itself the HP offset.
    bool hpOk = false;
    h = FindOne(img, "48 8B 8B ? ? ? ? 33 D2 48 8B 49 ? E8");
    if (h != SIZE_MAX) {
        size_t setHp = h + 18 + I32(img, h + 14);
        if (setHp + 64 <= img.size() &&
            MatchAt(img, setHp, "89 54 24 10 53 48 83 EC 20 48 8B D9 48 81 C1 ? ? ? ? C7 44 24 30 00 00 00 00 85 D2 "
                                "79 07 48 8D 44 24 30 EB 0B 3B 11 48 8D 44 24 38 48 0F 4F C1 8B 00 89 83")) {
            off.modules = I32(img, h + 3);
            off.dataModule = img[h + 12];
            off.hp = I32(img, setHp + 53);
            hpOk = true;
        }
    }
    // Play region of the floor collision under the player (gives the current map).
    bool regionOk = false;
    h = FindOne(img, "E8 ? ? ? ? 8B 08 89 8E ? ? ? ? 48 8B CB E8 ? ? ? ? 85 C0 78 06 88 86");
    if (h != SIZE_MAX) {
        off.playRegion = I32(img, h + 9);
        regionOk = true;
    }
    signaturesTotal += 3;   // SetHp caller, SetHp, play region writer
    signaturesFound += (hpOk ? 2 : 0) + (regionOk ? 1 : 0);
    snprintf(line, sizeof(line), "DS3 | mortes +%X | tempo +%X | jogador +%X | slot +%X%s | fogueira +%X\n",
             off.deaths, off.playTime, off.playerIns, off.saveSlot, slotOk ? "" : " (padrão)", off.respawn);
    log += line;
    snprintf(line, sizeof(line), "DS3 | módulos +%X/+%X | hp +%X%s | região +%X%s\n", off.modules, off.dataModule,
             off.hp, hpOk ? "" : " (padrão)", off.playRegion, regionOk ? "" : " (padrão)");
    log += line;
    snprintf(line, sizeof(line), "DS3 | assinaturas encontradas: %d/%d\n", signaturesFound, signaturesTotal);
    log += line;

    // Counting needs GameDataMan; WorldChrMan tells when a character is actually in the world.
    return gameDataMan && worldChrMan;
}

void Ds3Reader::Read(HANDLE proc, GameSnapshot& s) const
{
    auto read = [&](uintptr_t addr, void* out, size_t n) {
        SIZE_T got = 0;
        return addr && ReadProcessMemory(proc, (LPCVOID)addr, out, n, &got) && got == n;
    };
    auto ptr = [&](uintptr_t addr) -> uintptr_t {
        uintptr_t v = 0;
        return read(addr, &v, sizeof(v)) && ValidPtr(v) ? v : 0;
    };

    if (uintptr_t gdm = ptr(gameDataMan)) {
        s.gameDataOk = true;
        read(gdm + off.deaths, &s.deaths, 4);
        read(gdm + off.playTime, &s.playTimeMs, 4);
        read(gdm + 0x78, &s.ngPlus, 4);
        if (s.ngPlus < 0 || s.ngPlus > 9999) s.ngPlus = 0;
        uintptr_t pgd = ptr(gdm + 0x10);   // PlayerGameData
        if (pgd) {
            wchar_t nameBuf[16] = {};
            if (read(pgd + 0x88, nameBuf, sizeof(nameBuf))) {
                std::wstring name;
                bool ok = true;
                for (int i = 0; i < 16 && nameBuf[i]; ++i) {
                    if (nameBuf[i] < 0x20 || nameBuf[i] == 0xFFFF) { ok = false; break; }
                    name += nameBuf[i];
                }
                if (ok) s.name = name;
            }
            read(pgd + 0x70, &s.soulLevel, 4);
            if (s.soulLevel < 0 || s.soulLevel > 10000) s.soulLevel = 0;
        }
        s.charLoaded = pgd && s.playTimeMs > 0 && s.deaths >= 0 && s.deaths < 10000000;
    }

    if (uintptr_t gm = ptr(gameMan)) {
        read(gm + off.saveSlot, &s.saveSlot, 4);
        if (s.saveSlot < 0 || s.saveSlot > 9) s.saveSlot = -1;
        read(gm + off.respawn, &s.lastBonfire, 4);
        if (s.lastBonfire < 0) s.lastBonfire = 0;
    }

    uintptr_t pins = ptr(worldChrMan) ? ptr(ptr(worldChrMan) + off.playerIns) : 0;
    if (!pins) return;
    s.inWorld = true;
    if (uintptr_t modules = ptr(pins + off.modules)) {
        if (uintptr_t data = ptr(modules + off.dataModule)) {
            read(data + off.hp, &s.hp, 4);
            if (s.hp < 0 || s.hp > 100000) s.hp = -1;
        }
        // Module 13 is SprjChrPhysicsModule; its position is a float4 (x, height, z, 1).
        if (uintptr_t physics = ptr(modules + 0x68)) {
            float p[3];
            if (read(physics + 0x80, p, sizeof(p)) && std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]) &&
                std::fabs(p[0]) < 1e5f && std::fabs(p[1]) < 1e5f && std::fabs(p[2]) < 1e5f) {
                s.pos = { p[0], p[1], p[2] };
                s.hasPos = true;
            }
        }
    }
    // The play region id is a decimal AABxCC number: AA = map area, B = map block.
    int region = 0;
    if (read(pins + off.playRegion, &region, 4) && region > 0) {
        s.mapRaw = (uint32_t)region;
        int area = region / 10000 % 100, block = region / 1000 % 10;
        if (area >= 30 && area <= 69) {
            s.mapArea = area;
            s.mapBlock = block;
            s.areaName = Ds3MapName(area, block);
            if (s.areaName.empty()) {
                char b[32];
                snprintf(b, sizeof(b), "Mapa m%02d_%02d", area, block);
                s.areaName = b;
            }
        }
    }
}
