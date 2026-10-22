#include "games/hollowknight.h"
#include "game_reader.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <iterator>
#include <unordered_map>

// How the Mono structures and the death signals were found: docs/how-it-was-made.md ("Hollow Knight").

namespace hk {

namespace {

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

uint16_t U16(const std::vector<uint8_t>& img, size_t at)
{
    uint16_t v = 0;
    if (at + 2 <= img.size()) memcpy(&v, img.data() + at, 2);
    return v;
}

uint32_t U32(const std::vector<uint8_t>& img, size_t at)
{
    uint32_t v = 0;
    if (at + 4 <= img.size()) memcpy(&v, img.data() + at, 4);
    return v;
}

uint8_t B(const std::vector<uint8_t>& img, size_t at) { return at < img.size() ? img[at] : 0; }

// Exported functions of a PE32+ image in mapped layout: name -> RVA.
std::map<std::string, uint32_t> Exports(const std::vector<uint8_t>& img)
{
    std::map<std::string, uint32_t> out;
    if (img.size() < 0x400 || img[0] != 'M' || img[1] != 'Z') return out;
    uint32_t pe = U32(img, 0x3C);
    if (U32(img, pe) != 0x4550 || U16(img, pe + 0x18) != 0x20B) return out;   // "PE\0\0", PE32+
    uint32_t dir = U32(img, pe + 0x18 + 0x70);
    if (!dir || dir + 0x28 > img.size()) return out;
    uint32_t nNames = U32(img, dir + 0x18), funcs = U32(img, dir + 0x1C), names = U32(img, dir + 0x20),
             ords = U32(img, dir + 0x24);
    for (uint32_t i = 0; i < nNames && i < 0x10000; ++i) {
        uint32_t nameRva = U32(img, names + i * 4);
        if (!nameRva || nameRva >= img.size()) continue;
        const char* n = (const char*)img.data() + nameRva;
        size_t len = strnlen(n, std::min<size_t>(img.size() - nameRva, 256));
        out[std::string(n, len)] = U32(img, funcs + U16(img, ords + i * 2) * 4);
    }
    return out;
}

// Start of the code of an exported function, following "jmp rel32" thunks.
size_t FunctionAt(const std::vector<uint8_t>& img, uint32_t rva)
{
    size_t at = rva;
    for (int i = 0; i < 3 && B(img, at) == 0xE9; ++i) at = at + 5 + (int32_t)U32(img, at + 1);
    return at < img.size() ? at : 0;
}

// "mov rax, [rcx+disp]; ret": a plain field getter. Returns disp or -1.
int Getter(const std::vector<uint8_t>& img, size_t fn)
{
    if (B(img, fn) == 0x48 && B(img, fn + 1) == 0x8B && B(img, fn + 2) == 0x41 && B(img, fn + 4) == 0xC3)
        return B(img, fn + 3);
    if (B(img, fn) == 0x48 && B(img, fn + 1) == 0x8B && B(img, fn + 2) == 0x81 && B(img, fn + 7) == 0xC3)
        return (int)U32(img, fn + 3);
    return -1;
}

bool InRange(int v, int lo, int hi) { return v >= lo && v <= hi; }

} // namespace

bool MappedImageFromFile(const std::vector<uint8_t>& file, std::vector<uint8_t>& img)
{
    if (file.size() < 0x400 || file[0] != 'M' || file[1] != 'Z') return false;
    uint32_t pe = U32(file, 0x3C);
    if (U32(file, pe) != 0x4550 || U16(file, pe + 0x18) != 0x20B) return false;
    uint16_t nSections = U16(file, pe + 6), optSize = U16(file, pe + 0x14);
    uint32_t imageSize = U32(file, pe + 0x18 + 0x38), headerSize = U32(file, pe + 0x18 + 0x3C);
    if (imageSize == 0 || imageSize > 0x10000000 || headerSize > file.size()) return false;
    img.assign(imageSize, 0);
    memcpy(img.data(), file.data(), std::min<size_t>(headerSize, imageSize));
    size_t sec = pe + 0x18 + optSize;
    for (uint16_t i = 0; i < nSections; ++i, sec += 40) {
        uint32_t va = U32(file, sec + 12), rawSize = U32(file, sec + 16), raw = U32(file, sec + 20);
        if (raw >= file.size() || va >= imageSize) continue;
        size_t n = std::min<size_t>({ rawSize, file.size() - raw, imageSize - va });
        memcpy(img.data() + va, file.data() + raw, n);
    }
    return true;
}

bool AnalyzeMono(const std::vector<uint8_t>& img, MonoLayout& L, std::string& log)
{
    L = MonoLayout{};
    auto ex = Exports(img);
    auto fn = [&](const char* name) -> size_t {
        auto it = ex.find(name);
        return it == ex.end() ? 0 : FunctionAt(img, it->second);
    };
    char line[200];
    auto put = [&](const char* what, long long v, bool hex = true) {
        if (v < 0) snprintf(line, sizeof(line), "  %-24s NÃO ENCONTRADO\n", what);
        else snprintf(line, sizeof(line), hex ? "  %-24s +%llX\n" : "  %-24s %lld\n", what, v);
        log += line;
    };
    if (ex.empty()) {
        log += "  tabela de exportação não encontrada\n";
        return false;
    }

    // mono_assembly_foreach walks the global list: mov reg,[rip+list]; test reg,reg
    if (size_t f = fn("mono_assembly_foreach")) {
        for (size_t i = f; i < f + 0x100 && !L.loadedAssemblies; ++i) {
            uint8_t m = B(img, i + 2);
            if (B(img, i) != 0x48 || B(img, i + 1) != 0x8B || (m & 0xC7) != 0x05) continue;
            int reg = (m >> 3) & 7;
            if (B(img, i + 7) == 0x48 && B(img, i + 8) == 0x85 && B(img, i + 9) == (0xC0 | reg << 3 | reg))
                L.loadedAssemblies = (uint32_t)(i + 7 + (int32_t)U32(img, i + 3));
        }
    }
    snprintf(line, sizeof(line), L.loadedAssemblies ? "  %-24s dll+%X\n" : "  %-24s NÃO ENCONTRADO\n",
             "loaded_assemblies", L.loadedAssemblies);
    log += line;

    // Plain getters.
    L.assemblyImage = Getter(img, fn("mono_assembly_get_image"));
    L.imageName = Getter(img, fn("mono_image_get_name"));
    L.className = Getter(img, fn("mono_class_get_name"));
    L.classNamespace = Getter(img, fn("mono_class_get_namespace"));
    L.classParent = Getter(img, fn("mono_class_get_parent"));
    L.fieldName = Getter(img, fn("mono_field_get_name"));

    // mono_field_get_offset: (class init call); mov eax,[rbx+offset]
    if (size_t f = fn("mono_field_get_offset")) {
        if (B(img, f) == 0x8B && B(img, f + 1) == 0x41 && B(img, f + 3) == 0xC3) L.fieldOffset = B(img, f + 2);
        for (size_t i = f; i < f + 0x40 && L.fieldOffset < 0; ++i)
            if (B(img, i) == 0xE8 && B(img, i + 5) == 0x8B && (B(img, i + 6) & 0xF8) == 0x40 && (B(img, i + 6) & 7) != 4)
                L.fieldOffset = B(img, i + 7);
    }

    // mono_vtable_get_static_field_data: movsxd rax,[klass+vtable_size]; mov rax,[vt+rax*8+vtable]
    if (size_t f = fn("mono_vtable_get_static_field_data")) {
        for (size_t i = f; i < f + 0x30; ++i) {
            if (L.classVtableSize < 0 && B(img, i) == 0x48 && B(img, i + 1) == 0x63 && (B(img, i + 2) & 0xC0) == 0x40 &&
                (B(img, i + 2) & 7) != 4)
                L.classVtableSize = B(img, i + 3);
            if (L.vtableData < 0 && B(img, i) == 0x48 && B(img, i + 1) == 0x8B && (B(img, i + 2) & 0xC7) == 0x44 &&
                (B(img, i + 3) >> 6) == 3)
                L.vtableData = B(img, i + 4);
        }
    }

    // mono_class_get_fields: shl rdx,5 (sizeof field); add rdx,[klass+fields]
    if (size_t f = fn("mono_class_get_fields")) {
        for (size_t i = f; i < f + 0x100 && L.classFields < 0; ++i) {
            if (B(img, i) == 0x48 && B(img, i + 1) == 0xC1 && (B(img, i + 2) & 0xF8) == 0xE0 && B(img, i + 3) <= 7 &&
                B(img, i + 4) == 0x48 && B(img, i + 5) == 0x03 && (B(img, i + 6) & 0xC0) == 0x80 && (B(img, i + 6) & 7) != 4) {
                L.fieldSize = 1 << B(img, i + 3);
                L.classFields = (int)U32(img, i + 7);
            }
        }
    }

    // mono_class_num_fields -> field count getter: mov eax,[klass+field_count]; add rsp,N; ret
    if (size_t f = fn("mono_class_num_fields")) {
        for (size_t i = f; i < f + 0x100 && L.classDefFieldCount < 0; ++i) {
            uint8_t m = B(img, i + 1);
            if (B(img, i) == 0x8B && (m & 0xF8) == 0x80 && (m & 7) != 4 &&
                ((B(img, i + 6) == 0x48 && B(img, i + 7) == 0x83 && B(img, i + 8) == 0xC4 && B(img, i + 10) == 0xC3) ||
                 B(img, i + 6) == 0xC3))
                L.classDefFieldCount = (int)U32(img, i + 2);
        }
    }

    // mono_class_vtable: mov rbx,[klass+runtime_info]; test rbx,rbx; ...; movzx eax,word [rbx] (max_domain)
    if (size_t f = fn("mono_class_vtable")) {
        for (size_t i = f; i < f + 0x140 && L.classRuntimeInfo < 0; ++i) {
            uint8_t m = B(img, i + 2);
            if (B(img, i) != 0x48 || B(img, i + 1) != 0x8B || (m & 0xC0) != 0x80 || (m & 7) == 4) continue;
            int reg = (m >> 3) & 7;
            if (B(img, i + 7) != 0x48 || B(img, i + 8) != 0x85 || B(img, i + 9) != (0xC0 | reg << 3 | reg)) continue;
            for (size_t j = i + 10; j < i + 0x30; ++j)
                if (B(img, j) == 0x0F && B(img, j + 1) == 0xB7 && (B(img, j + 2) & 0xC7) == reg && reg != 4 && reg != 5) {
                    L.classRuntimeInfo = (int)U32(img, i + 3);
                    break;
                }
        }
    }

    // mono_image_init sets up the class cache inline: mov qword [image+cache.size],11 (initial
    // size; table/num_entries follow), and stores class_next_value right before it.
    if (size_t f = fn("mono_image_init")) {
        for (size_t i = f; i < f + 0x300 && L.imageClassCache < 0; ++i) {
            uint8_t m = B(img, i + 2);
            if (B(img, i) == 0x48 && B(img, i + 1) == 0xC7 && (m & 0xF8) == 0x80 && (m & 7) != 4 && U32(img, i + 7) == 11)
                L.imageClassCache = (int)U32(img, i + 3) - 0x18;
        }
        if (L.imageClassCache > 0) {
            // mov [image+cache.next_value],rax ... after lea rax,[class_next_value]
            uint32_t nextField = (uint32_t)L.imageClassCache + 0x10;
            for (size_t i = f; i < f + 0x300 && L.classDefNextCache < 0; ++i) {
                uint8_t m = B(img, i + 2);
                if (B(img, i) != 0x48 || B(img, i + 1) != 0x89 || (m & 0xF8) != 0x80 || (m & 7) == 4 || U32(img, i + 3) != nextField)
                    continue;
                for (size_t j = i; j + 0x20 > i && j > f; --j) {
                    if (B(img, j) == 0x48 && B(img, j + 1) == 0x8D && B(img, j + 2) == 0x05) {
                        size_t target = j + 7 + (int32_t)U32(img, j + 3);
                        // class_next_value: lea rax,[klass+next_class_cache]; ret
                        if (B(img, target) == 0x48 && B(img, target + 1) == 0x8D && B(img, target + 2) == 0x81 &&
                            B(img, target + 7) == 0xC3)
                            L.classDefNextCache = (int)U32(img, target + 3);
                        else if (B(img, target) == 0x48 && B(img, target + 1) == 0x8D && B(img, target + 2) == 0x41 &&
                                 B(img, target + 4) == 0xC3)
                            L.classDefNextCache = B(img, target + 3);
                        break;
                    }
                }
            }
        }
    }

    put("assembly->image", L.assemblyImage);
    put("image->nome", L.imageName);
    put("image->class_cache", L.imageClassCache);
    put("class->parent", L.classParent);
    put("class->name", L.className);
    put("class->namespace", L.classNamespace);
    put("class->vtable_size", L.classVtableSize);
    put("class->fields", L.classFields);
    put("class->runtime_info", L.classRuntimeInfo);
    put("classdef->field_count", L.classDefFieldCount);
    put("classdef->next_cache", L.classDefNextCache);
    put("field->name", L.fieldName);
    put("field->offset", L.fieldOffset);
    put("sizeof(field)", L.fieldSize);
    put("vtable->vtable", L.vtableData);

    return L.loadedAssemblies && InRange(L.assemblyImage, 0x8, 0x200) && InRange(L.imageName, 0x8, 0x200) &&
           InRange(L.imageClassCache, 0x100, 0x2000) && InRange(L.classParent, 0x8, 0x100) &&
           InRange(L.className, 0x8, 0x100) && InRange(L.classNamespace, 0x8, 0x100) &&
           InRange(L.classVtableSize, 0x8, 0x100) && InRange(L.classFields, 0x20, 0x200) &&
           InRange(L.classRuntimeInfo, 0x20, 0x200) && InRange(L.classDefFieldCount, 0x80, 0x200) &&
           InRange(L.classDefNextCache, 0x80, 0x200) && InRange(L.fieldName, 0x8, 0x40) &&
           InRange(L.fieldOffset, 0x8, 0x40) && InRange(L.fieldSize, 0x10, 0x40) && InRange(L.vtableData, 0x20, 0x100);
}

bool DeathDetector::Update(const Sample& s)
{
    if (!s.inGame || !s.save) {
        // Menu, quitting, loading a save: nothing to compare with until the Knight is back.
        haveBase_ = false;
        dying_ = false;
        return false;
    }
    bool sameSave = haveBase_ && s.save == save_;
    bool death = false;
    if (sameSave && !dying_ && s.playing)
        death = (lastHealth_ > 0 && s.health == 0) || (!lastDead_ && s.dead);
    if (death) {
        ++count_;
        dying_ = true;
    } else if (!sameSave) {
        // A save loaded mid-death (the game saves when the Knight dies): wait for the respawn.
        dying_ = s.health == 0 || s.dead;
    } else if (dying_ && s.health > 0 && !s.dead) {
        dying_ = false;   // respawned
    }
    haveBase_ = true;
    save_ = s.save;
    lastHealth_ = s.health;
    lastDead_ = s.dead;
    return death;
}

} // namespace hk

namespace {

using hk::MonoLayout;

template <class T> bool ReadT(HANDLE proc, uintptr_t addr, T& v)
{
    SIZE_T n = 0;
    return addr && ReadProcessMemory(proc, (LPCVOID)addr, &v, sizeof(T), &n) && n == sizeof(T);
}

uintptr_t ReadPtr(HANDLE proc, uintptr_t addr)
{
    uintptr_t v = 0;
    if (!ReadT(proc, addr, v) || !hk::ValidPtr(v)) return 0;
    return v;
}

// System.String -> UTF-8. Scene names and versions are ASCII; anything else becomes '?'.
std::string ReadMonoString(HANDLE proc, uintptr_t str)
{
    int32_t len = 0;
    if (!hk::ValidPtr(str) || !ReadT(proc, str + 0x10, len) || len < 0 || len > 256) return {};
    std::vector<wchar_t> w(len);
    SIZE_T n = 0;
    if (len && (!ReadProcessMemory(proc, (LPCVOID)(str + 0x14), w.data(), len * sizeof(wchar_t), &n) || n != len * sizeof(wchar_t)))
        return {};
    std::string out;
    for (wchar_t c : w) out += (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    return out;
}

// Reads through 4 KB pages kept for the duration of one lookup: the class cache walk touches
// thousands of small structures and names that sit close together.
class PageCache {
public:
    explicit PageCache(HANDLE proc) : proc_(proc) {}

    bool Read(uintptr_t addr, void* out, size_t len)
    {
        if (!hk::ValidPtr(addr)) return false;
        uint8_t* dst = (uint8_t*)out;
        while (len) {
            uintptr_t page = addr & ~(uintptr_t)0xFFF;
            const std::vector<uint8_t>& p = Page(page);
            if (p.empty()) return false;
            size_t off = addr - page, n = std::min(len, (size_t)0x1000 - off);
            memcpy(dst, p.data() + off, n);
            dst += n; addr += n; len -= n;
        }
        return true;
    }
    uintptr_t Ptr(uintptr_t addr)
    {
        uintptr_t v = 0;
        return Read(addr, &v, sizeof(v)) && hk::ValidPtr(v) ? v : 0;
    }
    int32_t I32(uintptr_t addr, int32_t fallback = -1)
    {
        int32_t v = 0;
        return Read(addr, &v, sizeof(v)) ? v : fallback;
    }
    std::string CStr(uintptr_t addr)
    {
        std::string s;
        char c = 0;
        while (s.size() < 256 && Read(addr + s.size(), &c, 1) && c) s += c;
        return s;
    }

private:
    const std::vector<uint8_t>& Page(uintptr_t page)
    {
        auto it = pages_.find(page);
        if (it != pages_.end()) return it->second;
        std::vector<uint8_t> buf(0x1000);
        SIZE_T n = 0;
        if (!ReadProcessMemory(proc_, (LPCVOID)page, buf.data(), buf.size(), &n) || n != buf.size()) buf.clear();
        return pages_.emplace(page, std::move(buf)).first->second;
    }

    HANDLE proc_;
    std::unordered_map<uintptr_t, std::vector<uint8_t>> pages_;
};

struct FieldInfo {
    int offset = -1;
    bool isStatic = false;
};

const char* const kGameStates[] = { "INACTIVE", "MAIN_MENU", "LOADING", "ENTERING_LEVEL", "PLAYING",
                                    "PAUSED", "EXITING_LEVEL", "CUTSCENE", "PRIMER" };
enum { kStateInactive = 0, kStateMainMenu = 1, kStateLoading = 2, kStatePlaying = 4 };

bool IsMenuScene(const std::string& scene)
{
    static const char* const menus[] = { "Menu_Title", "Quit_To_Menu", "Pre_Menu_Intro", "PermaDeath",
                                         "PermaDeath_Unlock", "Menu_Credits", "End_Credits", "End_Game_Completion" };
    for (const char* m : menus)
        if (scene == m) return true;
    return scene.empty();
}

class HollowKnightModule : public GameModule {
public:
    bool Analyze(const std::vector<uint8_t>&, uintptr_t, uint32_t, std::string& log) override
    {
        // Nothing to find in the exe (a small Unity launcher): the game lives in Mono, which
        // Read() resolves once the runtime has loaded the game's classes.
        log +="Unity/Mono: classes do jogo são localizadas pelo runtime Mono (veja abaixo)\n";
        return true;
    }
    bool Read(HANDLE proc, GameSnapshot& s) override;
    std::string VersionLabel(uint32_t) const override { return version_; }
    std::string LiveDiagnostics() const override;

private:
    bool Resolve(HANDLE proc);
    bool LoadMonoLayout(HANDLE proc);
    std::unordered_map<std::string, FieldInfo> FieldsOf(PageCache& pc, uintptr_t klass) const;
    uintptr_t StaticData(PageCache& pc, uintptr_t klass) const;

    // Mono runtime
    uintptr_t monoBase_ = 0;
    MonoLayout L_;
    bool monoOk_ = false;
    std::string monoLog_;
    std::string status_ = "aguardando o runtime Mono";
    ULONGLONG nextResolve_ = 0;
    int resolveAttempts_ = 0;
    bool resolved_ = false;

    // Static field storage of GameManager (holds GameManager._instance)
    uintptr_t gmStatic_ = 0;
    // Field offsets inside the game objects
    int offInstance_ = -1;
    int gmPlayerData_ = -1, gmGameState_ = -1, gmSceneName_ = -1, gmHero_ = -1, gmSceneManager_ = -1,
        gmProfileId_ = -1, gmSessionStart_ = -1, gmSessionTimer_ = -1;
    int pdHealth_ = -1, pdMaxHealth_ = -1, pdPermadeath_ = -1, pdMapZone_ = -1, pdRespawnScene_ = -1,
        pdVersion_ = -1, pdPlayTime_ = -1;
    int heroCachedPtr_ = -1, heroStates_ = -1;
    int stDead_ = -1, stHazardDeath_ = -1;
    int smMapZone_ = -1;
    std::map<std::string, int> bossFlags_;   // PlayerData defeat flag -> offset

    // Last reading (Diagnostics)
    hk::DeathDetector detector_;
    uintptr_t sceneStr_ = 0, respawnStr_ = 0, versionStr_ = 0;
    std::string scene_, respawn_, version_;
    int gameState_ = -1, profile_ = -1, health_ = -1, maxHealth_ = -1, zone_ = -1, permadeath_ = -1;
    bool heroOk_ = false, dead_ = false, hazardDeath_ = false;
    std::string cause_;
};

bool HollowKnightModule::LoadMonoLayout(HANDLE proc)
{
    DWORD pid = GetProcessId(proc);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me{ sizeof(me) };
    uintptr_t base = 0;
    uint32_t size = 0;
    bool legacyMono = false;
    for (BOOL ok = Module32FirstW(snap, &me); ok; ok = Module32NextW(snap, &me)) {
        if (_wcsicmp(me.szModule, L"mono-2.0-bdwgc.dll") == 0) {
            base = (uintptr_t)me.modBaseAddr;
            size = me.modBaseSize;
        } else if (_wcsicmp(me.szModule, L"mono.dll") == 0) {
            legacyMono = true;
        }
    }
    CloseHandle(snap);
    if (!base) {
        status_ = legacyMono ? "mono.dll antigo (versão do jogo anterior à 1.5): não suportado"
                             : "runtime Mono ainda não carregado";
        return false;
    }

    std::vector<uint8_t> img(size, 0);
    for (uintptr_t addr = base; addr < base + size;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(proc, (LPCVOID)addr, &mbi, sizeof(mbi))) break;
        uintptr_t end = std::min<uintptr_t>((uintptr_t)mbi.BaseAddress + mbi.RegionSize, base + size);
        if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            SIZE_T n = 0;
            ReadProcessMemory(proc, (LPCVOID)addr, img.data() + (addr - base), end - addr, &n);
        }
        addr = end;
    }
    monoLog_.clear();
    char line[128];
    snprintf(line, sizeof(line), "Mono: mono-2.0-bdwgc.dll em %llX (tamanho %X)\n", (unsigned long long)base, size);
    monoLog_ += line;
    monoOk_ = hk::AnalyzeMono(img, L_, monoLog_);
    monoBase_ = base;
    if (!monoOk_) status_ = "estrutura do runtime Mono não reconhecida (versão não suportada)";
    return monoOk_;
}

std::unordered_map<std::string, FieldInfo> HollowKnightModule::FieldsOf(PageCache& pc, uintptr_t klass) const
{
    std::unordered_map<std::string, FieldInfo> out;
    // The class and its parents (UnityEngine.Object's m_CachedPtr is a few levels up).
    for (int depth = 0; klass && depth < 8; ++depth, klass = pc.Ptr(klass + L_.classParent)) {
        int count = pc.I32(klass + L_.classDefFieldCount, 0);
        uintptr_t fields = pc.Ptr(klass + L_.classFields);
        if (!fields || count <= 0 || count > 10000) continue;
        for (int i = 0; i < count; ++i) {
            uintptr_t f = fields + (uintptr_t)i * L_.fieldSize;
            if (pc.Ptr(f + 0x10) != klass) continue;   // MonoClassField.parent: sanity check
            std::string name = pc.CStr(pc.Ptr(f + L_.fieldName));
            if (name.empty() || out.count(name)) continue;
            FieldInfo fi;
            fi.offset = pc.I32(f + L_.fieldOffset);
            uint16_t attrs = 0;
            if (uintptr_t type = pc.Ptr(f)) pc.Read(type + 8, &attrs, sizeof(attrs));   // MonoType.attrs
            fi.isStatic = (attrs & 0x10) != 0;                                          // FIELD_ATTRIBUTE_STATIC
            out.emplace(std::move(name), fi);
        }
    }
    return out;
}

uintptr_t HollowKnightModule::StaticData(PageCache& pc, uintptr_t klass) const
{
    // MonoClassRuntimeInfo { uint16 max_domain; MonoVTable* domain_vtables[]; }. Unity runs the
    // game in one domain; take the vtable that belongs to this class.
    uintptr_t info = pc.Ptr(klass + L_.classRuntimeInfo);
    if (!info) return 0;
    uint16_t maxDomain = 0;
    if (!pc.Read(info, &maxDomain, sizeof(maxDomain))) return 0;
    int vtableSize = pc.I32(klass + L_.classVtableSize, -1);
    if (vtableSize < 0 || vtableSize > 0x10000) return 0;
    for (int d = 0; d <= maxDomain && d < 16; ++d) {
        uintptr_t vt = pc.Ptr(info + 8 + d * 8);
        if (!vt || pc.Ptr(vt) != klass) continue;
        if (uintptr_t data = pc.Ptr(vt + L_.vtableData + (uintptr_t)vtableSize * 8)) return data;
    }
    return 0;
}

bool HollowKnightModule::Resolve(HANDLE proc)
{
    ULONGLONG now = GetTickCount64();
    if (now < nextResolve_) return false;
    nextResolve_ = now + 2000;
    ++resolveAttempts_;

    if (!monoOk_) {
        if (monoBase_) return false;   // runtime found but not understood: retrying won't help
        if (!LoadMonoLayout(proc)) return false;
    }

    PageCache pc(proc);
    // Loaded assemblies -> Assembly-CSharp's image.
    uintptr_t image = 0;
    int n = 0;
    for (uintptr_t node = pc.Ptr(monoBase_ + L_.loadedAssemblies); node && n < 1000 && !image; node = pc.Ptr(node + 8), ++n) {
        uintptr_t img = pc.Ptr(pc.Ptr(node) + L_.assemblyImage);
        if (img && pc.CStr(pc.Ptr(img + L_.imageName)) == "Assembly-CSharp") image = img;
    }
    if (!image) {
        status_ = "Assembly-CSharp ainda não carregado";
        return false;
    }

    // Class cache (MonoInternalHashTable: size at +0x18, bucket array at +0x20).
    int buckets = pc.I32(image + L_.imageClassCache + 0x18, 0);
    uintptr_t table = pc.Ptr(image + L_.imageClassCache + 0x20);
    // Wanted classes (global namespace) and a field each must have: a nested class elsewhere
    // could share the name, the right one is the class with that field.
    struct Wanted { const char* name; const char* field; bool required; uintptr_t klass; };
    Wanted wanted[] = { { "GameManager", "_instance", true, 0 }, { "PlayerData", "health", true, 0 },
                        { "HeroController", "cState", true, 0 }, { "HeroControllerStates", "dead", true, 0 },
                        { "SceneManager", "mapZone", false, 0 } };
    std::unordered_map<uintptr_t, std::unordered_map<std::string, FieldInfo>> fieldsOf;
    int seen = 0;
    if (table && buckets > 0 && buckets < 0x100000) {
        for (int b = 0; b < buckets; ++b) {
            int chain = 0;
            for (uintptr_t k = pc.Ptr(table + (uintptr_t)b * 8); k && chain < 1000; k = pc.Ptr(k + L_.classDefNextCache), ++chain) {
                ++seen;
                std::string name = pc.CStr(pc.Ptr(k + L_.className));
                for (Wanted& w : wanted) {
                    if (w.klass || name != w.name || !pc.CStr(pc.Ptr(k + L_.classNamespace)).empty()) continue;
                    auto fields = FieldsOf(pc, k);
                    if (fields.count(w.field)) {
                        w.klass = k;
                        fieldsOf[k] = std::move(fields);
                    }
                }
            }
        }
    }
    char line[200];
    snprintf(line, sizeof(line), "Assembly-CSharp: %d classes no cache", seen);
    status_ = line;
    for (const Wanted& w : wanted)
        if (!w.klass && w.required) {
            status_ += std::string(" | ") + w.name + " não encontrada";
            return false;
        }

    const auto& gm = fieldsOf[wanted[0].klass];
    const auto& pd = fieldsOf[wanted[1].klass];
    const auto& hero = fieldsOf[wanted[2].klass];
    const auto& st = fieldsOf[wanted[3].klass];
    const auto& sm = fieldsOf[wanted[4].klass];   // empty if SceneManager wasn't found
    auto inst = [](const std::unordered_map<std::string, FieldInfo>& f, const char* name) {
        auto it = f.find(name);
        return it != f.end() && !it->second.isStatic ? it->second.offset : -1;
    };
    auto gmInst = gm.find("_instance");
    offInstance_ = gmInst != gm.end() && gmInst->second.isStatic ? gmInst->second.offset : -1;
    gmPlayerData_ = inst(gm, "playerData");
    gmGameState_ = inst(gm, "gameState");
    gmSceneName_ = inst(gm, "sceneName");
    gmHero_ = inst(gm, "<hero_ctrl>k__BackingField");
    gmSceneManager_ = inst(gm, "<sm>k__BackingField");
    gmProfileId_ = inst(gm, "profileID");
    gmSessionStart_ = inst(gm, "sessionStartTime");
    gmSessionTimer_ = inst(gm, "sessionPlayTimer");
    pdHealth_ = inst(pd, "health");
    pdMaxHealth_ = inst(pd, "maxHealth");
    pdPermadeath_ = inst(pd, "permadeathMode");
    pdMapZone_ = inst(pd, "mapZone");
    pdRespawnScene_ = inst(pd, "respawnScene");
    pdVersion_ = inst(pd, "version");
    pdPlayTime_ = inst(pd, "playTime");
    heroCachedPtr_ = inst(hero, "m_CachedPtr");
    heroStates_ = inst(hero, "cState");
    stDead_ = inst(st, "dead");
    stHazardDeath_ = inst(st, "hazardDeath");
    smMapZone_ = inst(sm, "mapZone");
    bossFlags_.clear();
    for (const hk::BossArena& b : hk::BossArenas())
        if (b.flag) bossFlags_[b.flag] = inst(pd, b.flag);

    struct { const char* name; int off; } required[] = {
        { "GameManager._instance", offInstance_ }, { "GameManager.playerData", gmPlayerData_ },
        { "GameManager.gameState", gmGameState_ }, { "GameManager.sceneName", gmSceneName_ },
        { "GameManager.hero_ctrl", gmHero_ }, { "GameManager.profileID", gmProfileId_ },
        { "PlayerData.health", pdHealth_ }, { "PlayerData.permadeathMode", pdPermadeath_ },
        { "HeroController.cState", heroStates_ }, { "HeroControllerStates.dead", stDead_ },
    };
    for (const auto& r : required)
        if (r.off < 0) {
            status_ += std::string(" | campo ") + r.name + " não encontrado";
            return false;
        }

    gmStatic_ = StaticData(pc, wanted[0].klass);
    if (!gmStatic_) {
        status_ += " | GameManager ainda não inicializado";
        return false;
    }
    status_ += " | campos OK";
    resolved_ = true;
    return true;
}

bool HollowKnightModule::Read(HANDLE proc, GameSnapshot& s)
{
    if (!resolved_ && !Resolve(proc)) return false;

    auto i32 = [&](uintptr_t obj, int off, int fallback = -1) {
        int32_t v = fallback;
        return obj && off >= 0 && ReadT(proc, obj + off, v) ? v : fallback;
    };
    auto f32 = [&](uintptr_t obj, int off) {
        float v = 0;
        return obj && off >= 0 && ReadT(proc, obj + off, v) && v >= 0 && v < 1e8f ? v : 0.0f;
    };
    auto flag = [&](uintptr_t obj, int off) {
        uint8_t v = 0;
        return obj && off >= 0 && ReadT(proc, obj + off, v) && v != 0;
    };
    auto str = [&](uintptr_t obj, int off, uintptr_t& lastPtr, std::string& cached) {
        uintptr_t p = obj && off >= 0 ? ReadPtr(proc, obj + off) : 0;
        if (p != lastPtr) {
            lastPtr = p;
            cached = ReadMonoString(proc, p);
        }
    };

    uintptr_t gm = ReadPtr(proc, gmStatic_ + offInstance_);
    s.gameDataOk = gm != 0;
    uintptr_t pd = gm ? ReadPtr(proc, gm + gmPlayerData_) : 0;
    uintptr_t hero = gm ? ReadPtr(proc, gm + gmHero_) : 0;
    gameState_ = i32(gm, gmGameState_);
    profile_ = i32(gm, gmProfileId_);
    str(gm, gmSceneName_, sceneStr_, scene_);
    str(pd, pdRespawnScene_, respawnStr_, respawn_);
    str(pd, pdVersion_, versionStr_, version_);
    health_ = i32(pd, pdHealth_);
    maxHealth_ = i32(pd, pdMaxHealth_);
    permadeath_ = i32(pd, pdPermadeath_, 0);

    // A destroyed Unity object keeps its managed shell; m_CachedPtr (the native object) is 0 then.
    uintptr_t native = 0;
    heroOk_ = hero && (heroCachedPtr_ < 0 || (ReadT(proc, hero + heroCachedPtr_, native) && native));
    uintptr_t states = heroOk_ ? ReadPtr(proc, hero + heroStates_) : 0;
    dead_ = flag(states, stDead_);
    hazardDeath_ = flag(states, stHazardDeath_);

    uintptr_t sm = gm && gmSceneManager_ >= 0 ? ReadPtr(proc, gm + gmSceneManager_) : 0;
    zone_ = sm && smMapZone_ >= 0 ? i32(sm, smMapZone_) : i32(pd, pdMapZone_);

    bool menu = gameState_ == kStateInactive || gameState_ == kStateMainMenu || IsMenuScene(scene_);
    s.charLoaded = pd && profile_ >= 1 && profile_ <= 4 && !menu;
    s.inWorld = s.charLoaded && heroOk_;
    if (s.charLoaded) {
        s.saveSlot = profile_ - 1;
        s.name = L"Slot " + std::to_wstring(profile_) + (permadeath_ != 0 ? L" (Steel Soul)" : L"");
        float secs = gmSessionStart_ >= 0 ? f32(gm, gmSessionStart_) + f32(gm, gmSessionTimer_) : f32(pd, pdPlayTime_);
        s.playTimeMs = (uint32_t)std::min(secs * 1000.0f, 4.0e9f);
        s.hp = health_;
        s.lastBonfire = hk::BenchAreaId(respawn_);
        s.areaName = hk::MapZoneName(zone_);
        if (s.areaName.empty()) s.areaName = scene_;
    }

    // Cause: the boss of this arena while it's still undefeated; otherwise "Sonho" in dreams.
    cause_.clear();
    if (const hk::BossArena* b = hk::FindBossArena(scene_)) {
        auto it = b->flag ? bossFlags_.find(b->flag) : bossFlags_.end();
        int off = it != bossFlags_.end() ? it->second : -1;
        if (!b->flag || (off >= 0 && !flag(pd, off))) cause_ = std::string("Chefe: ") + b->boss;
    }
    if (cause_.empty() && hk::IsDreamZone(zone_)) cause_ = "Sonho";
    s.cause = cause_;

    hk::Sample sample;
    sample.inGame = s.inWorld;
    sample.playing = gameState_ > kStateLoading;
    sample.save = pd;
    sample.health = health_;
    sample.dead = dead_;
    detector_.Update(sample);
    s.deaths = detector_.count();
    return true;
}

std::string HollowKnightModule::LiveDiagnostics() const
{
    std::string t = monoLog_;
    char b[512];
    snprintf(b, sizeof(b), "Classes: %s (tentativas: %d)\n", status_.c_str(), resolveAttempts_);
    t += b;
    if (!resolved_) return t;
    const char* state = gameState_ >= 0 && gameState_ < (int)std::size(kGameStates) ? kGameStates[gameState_] : "?";
    snprintf(b, sizeof(b),
             "HK | cena: %s | zona: %d (%s) | estado: %s | slot: %d | Steel Soul: %d\n"
             "HK | vida: %d/%d | dead: %d | hazardDeath: %d | herói: %s | banco: %s (%s)\n"
             "HK | mortes vistas nesta sessão: %d%s | causa agora: %s\n",
             scene_.c_str(), zone_, hk::MapZoneName(zone_).c_str(), state, profile_, permadeath_, health_, maxHealth_,
             dead_, hazardDeath_, heroOk_ ? "sim" : "não", respawn_.c_str(),
             hk::BenchAreaName(hk::BenchAreaId(respawn_)).c_str(), detector_.count(),
             detector_.dying() ? " (morrendo)" : "", cause_.empty() ? "-" : cause_.c_str());
    t += b;
    return t;
}

std::unique_ptr<GameModule> CreateHollowKnight() { return std::make_unique<HollowKnightModule>(); }

} // namespace

// No death counter in the game: GameInfo::countsItself makes the app keep the total.
const GameInfo kHollowKnightInfo = {
    "hk", L"hollow_knight.exe", "Hollow Knight", "HK", CreateHollowKnight,
    hk::BenchAreaName, hk::AllAreaNames, hk::CauseSuggestions, "Último banco (área)", false, false, false, true,
};
