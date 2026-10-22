#include "games/sekiro.h"
#include "games/sekiro_data.h"
#include "game_reader.h"
#include <cmath>
#include <cstdio>

// How the signatures were found and checked: docs/how-it-was-made.md ("Sekiro").
// All of them were derived from the code of sekiro.exe 1.06 and match exactly once there;
// structures with two independent signatures must agree, or the feature is turned off.

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

// Target of a rel32 field at `at` whose instruction ends at `end`.
size_t Rel(const std::vector<uint8_t>& img, size_t at, size_t end)
{
    return end + ImageI32(img, at);
}

// The only match of `pat`, or SIZE_MAX (count in `hits`).
size_t FindOne(const std::vector<uint8_t>& img, const char* pat, size_t* hits = nullptr)
{
    auto h = FindPattern(img, pat);
    if (hits) *hits = h.size();
    return h.size() == 1 ? h[0] : SIZE_MAX;
}

const char* kNotFound = "NÃO ENCONTRADO";

// --- Signatures (sekiro.exe 1.06) ---------------------------------------------------------

// The event-script binding table entry for "AddDeathCount": lea r8,[name]; lea rdx,[entry]; ...; call [r10+X]
const char* kBindingReg = "4C 8D 05 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B C8 41 FF 52";
// Binding function: mov rcx,[global]; test rcx,rcx; jne impl
const char* kBindingThunk = "48 8B 0D ? ? ? ? 48 85 C9 0F 85";
// impl: mov ecx,1; jmp adder
const char* kAddOne = "B9 01 00 00 00 E9";
// adder: saturating add of ecx to [GameDataMan + deaths]
const char* kDeathAdder = "48 8B 15 ? ? ? ? 48 85 D2 74 ? 44 8B 82 ? ? ? ? 8B C1 F7 D0 44 3B C0 76 ? "
                          "C7 82 ? ? ? ? FF FF FF FF C3";
// Play time: add [rcx+time],eax; mov rax,[GameDataMan]; cmp [rax+time],999:59:59.000
const char* kPlayTime = "01 81 ? ? ? ? 48 8B 05 ? ? ? ? 81 B8 ? ? ? ? 18 A0 93 D6";
// Clear count (NG+ cycle): incremented and capped at 9999
const char* kClearCount = "48 8B 15 ? ? ? ? C7 44 24 ? 0F 27 00 00 C7 44 24 ? 00 00 00 00 8B 4A ? 83 C1 01";
// Save routine: slot -1 means GameMan's current slot, which must be 0..9
const char* kSaveSlot = "48 8B 05 ? ? ? ? 0F 85 ? ? ? ? 83 B8 ? ? ? ? 00 75 ? 83 FF FF 75 ? 8B B8 ? ? ? ? 83 FF 09 0F 87";
// Respawn point: GameMan override if >= 0, else the return point (last Sculptor's Idol)
const char* kReturnPoint = "48 8B 05 ? ? ? ? 8B 88 ? ? ? ? 85 C9 78 ? 8B C1 C3 8B 80 ? ? ? ? C3";
// Two unrelated reads of WorldChrMan -> main player (PlayerIns)
const char* kWorldChrManA = "48 8B 05 ? ? ? ? 4C 8B B0 ? ? ? ? 48 8B 5E 08 49 8B CE E8";
const char* kWorldChrManB = "48 8B 0D ? ? ? ? 48 8B B1 ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 48 8B CE E8";
// ChrDataModule::SetHp (clamps to the max HP stored right after it) and the ChrIns thunk that calls it
const char* kSetHp = "48 89 5C 24 ? 89 54 24 ? 57 48 83 EC 20 8B B9 ? ? ? ? 48 8B D9 48 81 C1 ? ? ? ?";
const char* kChrModuleThunk = "48 8B 89 ? ? ? ? 48 8B 49 ? E9";
// ChrIns -> module bag -> physics module -> position
const char* kPosition = "48 8B 88 ? ? ? ? 48 8B 41 ? 0F 28 40 ? 48 8B 05 ? ? ? ? 0F 29 44 24 ? 48 85 C0 75";
// Current map: FieldArea -> world info + current block index -> block list -> map id
const char* kFieldAreaA = "C7 01 FF FF FF FF 48 8B 0D ? ? ? ? 48 85 C9 74 ? 44 8B 41 ? 48 8D 54 24 ? 48 8B 49 ? "
                          "E8 ? ? ? ? 8B 10 89 13";
const char* kFieldAreaB = "48 8B 0D ? ? ? ? 48 85 C9 74 ? 44 8B 41 ? 48 8D 54 24 ? 48 8B 49 ? E8 ? ? ? ? "
                          "8B 4C 24 ? 8B C1 C1 E8 10";
const char* kBlockLookup = "45 85 C0 78 ? 44 3B 41 ? 7D ? 49 63 C0 BB 01 00 00 00 48 69 C8 ? ? ? ? 89 5C 24 ? "
                           "48 03 4A ? 48 8D 54 24 ? E8";
const char* kBlockMapId = "8B 41 ? 89 02 48 8B C2 C3";

} // namespace

std::string SekiroReader::VersionLabel(uint32_t moduleSize)
{
    switch (moduleSize) {
        case 0x4097000: return "1.02";
        case 0x4098000: return "1.03";
        case 0x42D2000: return "1.05 / 1.06";
    }
    return "?";
}

bool SekiroReader::Analyze(const std::vector<uint8_t>& img, uintptr_t base, std::string& log)
{
    *this = SekiroReader();
    char line[320];
    auto add = [&](const char* fmt, auto... args) {
        snprintf(line, sizeof(line), fmt, args...);
        log += line;
    };
    auto exe = [&](uintptr_t g) { return (unsigned long long)(g - base); };

    // --- Death counter: follow the game's own "AddDeathCount" script binding to the code that
    //     increments the counter. The same adder shape is used for two other counters, so the
    //     binding name is what tells them apart.
    {
        std::string fail;
        size_t name = SIZE_MAX, hits = 0;
        for (size_t h : FindPattern(img, "00 41 64 64 44 65 61 74 68 43 6F 75 6E 74 00")) { name = h + 1; ++hits; }
        if (hits != 1) fail = "texto AddDeathCount";
        size_t reg = SIZE_MAX;
        if (fail.empty()) {
            hits = 0;
            for (size_t h : FindPattern(img, kBindingReg))
                if (Rel(img, h + 3, h + 7) == name) { reg = h; ++hits; }
            if (hits != 1) fail = "registro do AddDeathCount";
        }
        size_t fn = SIZE_MAX;
        if (fail.empty()) {
            // lea rax,[fn]; mov [entry+8],rax shortly before the registration call
            size_t entry = Rel(img, reg + 10, reg + 14);
            for (size_t st = reg > 0x80 ? reg - 0x80 : 0; st < reg; ++st)
                if (PatternAt(img, st, "48 8D 05 ? ? ? ? 48 89 05") && Rel(img, st + 10, st + 14) == entry + 8)
                    fn = Rel(img, st + 3, st + 7);
            if (fn == SIZE_MAX || !PatternAt(img, fn, kBindingThunk)) fail = "função do AddDeathCount";
        }
        size_t adder = SIZE_MAX;
        if (fail.empty()) {
            size_t impl = Rel(img, fn + 12, fn + 16);
            if (impl < img.size() && PatternAt(img, impl, kAddOne)) adder = Rel(img, impl + 6, impl + 10);
            if (adder >= img.size() || !PatternAt(img, adder, kDeathAdder) ||
                ImageI32(img, adder + 15) != ImageI32(img, adder + 30))
                fail = "soma do contador";
        }
        if (fail.empty()) {
            gameDataMan_ = base + Rel(img, adder + 3, adder + 7);
            offDeaths_ = ImageI32(img, adder + 15);
            add("AddDeathCount -> exe+%llX | GameDataMan exe+%llX | mortes +%X\n",
                (unsigned long long)adder, exe(gameDataMan_), offDeaths_);
        } else {
            add("AddDeathCount -> %s (%s)\n", kNotFound, fail.c_str());
        }
    }

    // --- Play time and clear count: two more independent references to GameDataMan.
    {
        size_t n = 0, h = FindOne(img, kPlayTime, &n);
        if (h != SIZE_MAX && ImageI32(img, h + 2) == ImageI32(img, h + 15)) {
            uintptr_t g = base + Rel(img, h + 9, h + 13);
            if (g == gameDataMan_) {
                offPlayTime_ = ImageI32(img, h + 2);
                add("Tempo de jogo -> GameDataMan +%X (confere)\n", offPlayTime_);
            } else {
                add("Tempo de jogo -> DIVERGENTE (exe+%llX)\n", exe(g));
            }
        } else {
            add("Tempo de jogo -> %s (%zu ocorrências)\n", kNotFound, n);
        }
        h = FindOne(img, kClearCount, &n);
        if (h != SIZE_MAX) {
            uintptr_t g = base + Rel(img, h + 3, h + 7);
            if (g == gameDataMan_) {
                offClearCount_ = img[h + 25];
                add("Ciclo (NG+) -> GameDataMan +%X (confere)\n", offClearCount_);
            } else {
                add("Ciclo (NG+) -> DIVERGENTE (exe+%llX)\n", exe(g));
            }
        } else {
            add("Ciclo (NG+) -> %s (%zu ocorrências)\n", kNotFound, n);
        }
    }

    // --- GameMan: save slot and the respawn point.
    {
        size_t n1 = 0, n2 = 0;
        size_t a = FindOne(img, kSaveSlot, &n1), b = FindOne(img, kReturnPoint, &n2);
        uintptr_t ga = a != SIZE_MAX ? base + Rel(img, a + 3, a + 7) : 0;
        uintptr_t gb = b != SIZE_MAX ? base + Rel(img, b + 3, b + 7) : 0;
        if (ga && gb && ga != gb) {
            add("GameMan -> DIVERGENTE (exe+%llX / exe+%llX)\n", exe(ga), exe(gb));
        } else if (ga) {
            gameMan_ = ga;
            offSaveSlot_ = ImageI32(img, a + 29);
            if (gb) {
                offReturnOverride_ = ImageI32(img, b + 9);
                offReturnPoint_ = ImageI32(img, b + 22);
                add("GameMan -> exe+%llX | slot +%X | ponto de retorno +%X (+%X) | confere\n", exe(gameMan_),
                    offSaveSlot_, offReturnPoint_, offReturnOverride_);
            } else {
                add("GameMan -> exe+%llX | slot +%X | ponto de retorno %s\n", exe(gameMan_), offSaveSlot_, kNotFound);
            }
        } else {
            add("GameMan -> %s (%zu / %zu ocorrências)\n", kNotFound, n1, n2);
        }
    }

    // --- WorldChrMan -> PlayerIns (in world), HP and position.
    {
        size_t n1 = 0, n2 = 0;
        size_t a = FindOne(img, kWorldChrManA, &n1), b = FindOne(img, kWorldChrManB, &n2);
        uintptr_t ga = a != SIZE_MAX ? base + Rel(img, a + 3, a + 7) : 0;
        uintptr_t gb = b != SIZE_MAX ? base + Rel(img, b + 3, b + 7) : 0;
        int pa = a != SIZE_MAX ? ImageI32(img, a + 10) : -1, pb = b != SIZE_MAX ? ImageI32(img, b + 10) : -1;
        if (ga && gb && (ga != gb || pa != pb)) {
            add("WorldChrMan -> DIVERGENTE (exe+%llX +%X / exe+%llX +%X)\n", exe(ga), pa, exe(gb), pb);
        } else if (ga || gb) {
            worldChrMan_ = ga ? ga : gb;
            offPlayer_ = ga ? pa : pb;
            add("WorldChrMan -> exe+%llX | jogador +%X%s\n", exe(worldChrMan_), offPlayer_,
                ga && gb ? " | confere" : "");
        } else {
            add("WorldChrMan -> %s (%zu / %zu ocorrências)\n", kNotFound, n1, n2);
        }

        size_t n = 0, setHp = FindOne(img, kSetHp, &n);
        size_t thunk = SIZE_MAX, thunks = 0;
        if (setHp != SIZE_MAX)
            for (size_t h : FindPattern(img, kChrModuleThunk))
                if (Rel(img, h + 12, h + 16) == setHp) { thunk = h; ++thunks; }
        if (setHp != SIZE_MAX && thunks == 1 && ImageI32(img, setHp + 26) == ImageI32(img, setHp + 16) + 4) {
            offModules_ = ImageI32(img, thunk + 3);
            offDataModule_ = img[thunk + 10];
            offHp_ = ImageI32(img, setHp + 16);
        }
        size_t p = FindOne(img, kPosition, &n);
        if (p != SIZE_MAX && (offModules_ < 0 || ImageI32(img, p + 3) == offModules_)) {
            if (offModules_ < 0) offModules_ = ImageI32(img, p + 3);
            offPhysModule_ = img[p + 10];
            offPos_ = img[p + 14];
        }
        char hp[64] = "", pos[64] = "";
        if (offHp_ >= 0) snprintf(hp, sizeof(hp), "+%X -> +%X", offDataModule_, offHp_);
        if (offPos_ >= 0) snprintf(pos, sizeof(pos), "+%X -> +%X", offPhysModule_, offPos_);
        add("PlayerIns -> módulos +%X | HP %s | posição %s\n", offModules_, offHp_ >= 0 ? hp : kNotFound,
            offPos_ >= 0 ? pos : kNotFound);
    }

    // --- FieldArea: map id of the block the player is in.
    {
        size_t n1 = 0, n2 = 0, n3 = 0;
        size_t a = FindOne(img, kFieldAreaA, &n1), b = FindOne(img, kFieldAreaB, &n2);
        size_t look = FindOne(img, kBlockLookup, &n3);
        std::string fail;
        if (a == SIZE_MAX || b == SIZE_MAX || look == SIZE_MAX) {
            fail = kNotFound;
        } else if (Rel(img, a + 9, a + 13) != Rel(img, b + 3, b + 7) || img[a + 21] != img[b + 15] ||
                   img[a + 30] != img[b + 24]) {
            fail = "DIVERGENTE";
        } else {
            // Both callers use the lookup function that contains kBlockLookup.
            size_t fnA = Rel(img, a + 32, a + 36);
            size_t getter = Rel(img, look + 40, look + 44);
            if (!(look > fnA && look - fnA < 0x40) || getter >= img.size() || !PatternAt(img, getter, kBlockMapId))
                fail = "DIVERGENTE";
            else {
                fieldArea_ = base + Rel(img, a + 9, a + 13);
                offBlockIndex_ = img[a + 21];
                offWorldInfo_ = img[a + 30];
                offBlockCount_ = img[look + 8];
                blockStride_ = ImageI32(img, look + 22);
                offBlockList_ = img[look + 33];
                offBlockMapId_ = img[getter + 2];
            }
        }
        if (fail.empty())
            add("FieldArea -> exe+%llX | bloco +%X | mundo +%X -> +%X/+%X passo %X -> mapa +%X | confere\n",
                exe(fieldArea_), offBlockIndex_, offWorldInfo_, offBlockCount_, offBlockList_, blockStride_,
                offBlockMapId_);
        else
            add("FieldArea -> %s (%zu / %zu / %zu ocorrências)\n", fail.c_str(), n1, n2, n3);
    }

    // Counting needs the counter, the save slot (to tell characters apart) and the player pointer.
    return gameDataMan_ && offDeaths_ >= 0 && gameMan_ && worldChrMan_;
}

bool SekiroReader::Read(HANDLE proc, GameSnapshot& s) const
{
    if (uintptr_t gdm = ReadPtr(proc, gameDataMan_)) {
        s.gameDataOk = true;
        uint32_t deaths = 0;
        bool deathsOk = ReadT(proc, gdm + offDeaths_, deaths) && deaths < 10000000;
        s.deaths = deathsOk ? (int)deaths : 0;
        if (offPlayTime_ >= 0) ReadT(proc, gdm + offPlayTime_, s.playTimeMs);
        if (offClearCount_ >= 0) {
            ReadT(proc, gdm + offClearCount_, s.ngPlus);
            if (s.ngPlus < 0 || s.ngPlus > 9999) s.ngPlus = 0;
        }
        // GameDataMan+0x8 is the PlayerGameData of the loaded save. Sekiro has no character
        // name or level: the save slot alone identifies the character.
        uintptr_t pgd = ReadPtr(proc, gdm + 0x8);
        s.charLoaded = pgd && deathsOk && (offPlayTime_ < 0 || s.playTimeMs > 0);
    }

    if (uintptr_t gm = ReadPtr(proc, gameMan_)) {
        ReadT(proc, gm + offSaveSlot_, s.saveSlot);
        if (s.saveSlot < 0 || s.saveSlot > 9) s.saveSlot = -1;
        if (offReturnPoint_ >= 0) {
            int ovr = -1, ret = 0;
            ReadT(proc, gm + offReturnOverride_, ovr);
            ReadT(proc, gm + offReturnPoint_, ret);
            s.lastBonfire = ovr >= 0 ? ovr : ret;
            if (s.lastBonfire < 0) s.lastBonfire = 0;
        }
    }

    if (uintptr_t fa = ReadPtr(proc, fieldArea_)) {
        int index = -1, count = 0;
        uintptr_t wi = ReadPtr(proc, fa + offWorldInfo_);
        if (wi && ReadT(proc, fa + offBlockIndex_, index) && ReadT(proc, wi + offBlockCount_, count) &&
            index >= 0 && index < count && count < 1000) {
            if (uintptr_t list = ReadPtr(proc, wi + offBlockList_))
                ReadT(proc, list + (uintptr_t)index * blockStride_ + offBlockMapId_, s.mapRaw);
        }
        // Map ids are packed as AA BB CC DD (mAA_BB_CC_DD), area in the high byte.
        int area = (s.mapRaw >> 24) & 0xFF, block = (s.mapRaw >> 16) & 0xFF;
        if (s.mapRaw != 0 && s.mapRaw != 0xFFFFFFFF && area >= 10 && area < 100 && block < 10) {
            s.mapArea = area;
            s.mapBlock = block;
            s.areaName = SekiroMapName(area, block);
            if (s.areaName.empty()) {
                char b[32];
                snprintf(b, sizeof(b), "Mapa m%02d_%02d", area, block);
                s.areaName = b;
            }
        }
    }

    if (uintptr_t wcm = ReadPtr(proc, worldChrMan_)) {
        if (uintptr_t pins = ReadPtr(proc, wcm + offPlayer_)) {
            s.inWorld = true;
            uintptr_t modules = offModules_ >= 0 ? ReadPtr(proc, pins + offModules_) : 0;
            if (modules && offHp_ >= 0) {
                if (uintptr_t data = ReadPtr(proc, modules + offDataModule_)) {
                    ReadT(proc, data + offHp_, s.hp);
                    if (s.hp < 0 || s.hp > 100000) s.hp = -1;
                }
            }
            if (modules && offPos_ >= 0) {
                if (uintptr_t phys = ReadPtr(proc, modules + offPhysModule_)) {
                    float p[3];
                    SIZE_T n = 0;
                    if (ReadProcessMemory(proc, (LPCVOID)(phys + offPos_), p, sizeof(p), &n) && n == sizeof(p) &&
                        std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]) &&
                        std::fabs(p[0]) < 1e5f && std::fabs(p[1]) < 1e5f && std::fabs(p[2]) < 1e5f) {
                        s.pos = { p[0], p[1], p[2] };
                        s.hasPos = true;
                    }
                }
            }
        }
    }
    return true;
}

namespace {
std::unique_ptr<GameModule> CreateSekiro() { return std::make_unique<SekiroModule>(); }
}

// The respawn point is the last Sculptor's Idol; its exact name isn't known, only its area.
const GameInfo kSekiroInfo = {
    "sekiro", L"sekiro.exe", "Sekiro: Shadows Die Twice", "Sekiro", CreateSekiro,
    SekiroIdolArea, SekiroAllMapNames, SekiroCauseSuggestions, "Último ídolo (área)", false, false, false,
};

