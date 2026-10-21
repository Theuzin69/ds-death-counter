#include "games/bloodborne.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace bb {
namespace {

const wchar_t* kProcessNames[] = { L"bbport.exe", L"bb-probe.exe" };   // the port ships both (same file)

constexpr uintptr_t kGuestBase = 0x800000000ull;      // where port v0.5 places eboot address 0
constexpr size_t    kMaxImage = 512u << 20;           // the port's own limit for the linked image
const char kImageMarker[] = "/libexec/ld-elf.so.1";   // first bytes of the eboot image (its PT_INTERP)

// In the save file's copy of PlayerGameData the name comes 0x18 bytes after the level
// (level, blood echoes, ..., name); 0x90 / 0xA8 in 1.09.
constexpr int kNameAfterLevel = 0x18;

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

int32_t ReadI32(const std::vector<uint8_t>& buf, size_t at)
{
    int32_t v;
    memcpy(&v, buf.data() + at, 4);
    return v;
}

DWORD FindPortPid()
{
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    for (BOOL ok = Process32FirstW(snap, &pe); ok && !pid; ok = Process32NextW(snap, &pe))
        for (const wchar_t* name : kProcessNames)
            if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
    CloseHandle(snap);
    return pid;
}

} // namespace

bool AnalyzeImage(const std::vector<uint8_t>& img, Layout& out, std::string& log)
{
    out = Layout{};
    char line[256];
    auto rip = [&](size_t hit, int relOff, int instrEnd) -> uint64_t {
        return (uint64_t)((int64_t)hit + instrEnd + ReadI32(img, hit + relOff));
    };
    auto unique = [&](const char* name, const char* pattern, size_t& hit) {
        auto hits = FindPattern(img, pattern);
        if (hits.size() == 1) { hit = hits[0]; return true; }
        snprintf(line, sizeof(line), "%-14s -> %s (%zu)\n", name, hits.empty() ? "NÃO ENCONTRADO" : "AMBÍGUO", hits.size());
        log += line;
        return false;
    };

    // The Lua bindings AddDeathCount and AddTrueDeathCount, back to back: each increments a
    // GameDataMan field (saturating at -1). The game's scripts call AddDeathCount when the
    // death message appears, so that field is the death counter.
    size_t h = 0;
    uint64_t gdm = 0;
    if (unique("AddDeathCount",
               "48 8D 05 ? ? ? ? 48 83 38 00 74 ? 48 8D 05 ? ? ? ? 48 8B 00 48 85 C0 74 ? 8B 88 ? ? ? ? "
               "83 F9 FF 74 ? FF C1 89 88 ? ? ? ? C3 C7 80 ? ? ? ? FF FF FF FF C3 0F 1F 44 00 00 "
               "48 8D 05 ? ? ? ? 48 83 38 00 74 ? 48 8D 05 ? ? ? ? 48 8B 00 48 85 C0 74 ? 8B 88 ? ? ? ? "
               "83 F9 FF 74 ? FF C1 89 88", h) &&
        rip(h, 16, 20) == rip(h + 64, 16, 20)) {
        gdm = rip(h, 16, 20);
        out.offDeaths = ReadI32(img, h + 30);
        out.offTrueDeaths = ReadI32(img, h + 64 + 30);
    }

    // Play time: frame time * 1000 added to a GameDataMan field, capped at 999:59:59.
    int playTime = 0;
    if (unique("PlayTime", "C5 FA 59 0D ? ? ? ? C4 E1 FA 2C C9 48 8D 05 ? ? ? ? 48 8B 00 03 88 ? ? ? ? 81 F9 18 A0 93 D6", h)) {
        if (!gdm || rip(h, 16, 20) == gdm) {
            gdm = rip(h, 16, 20);
            playTime = ReadI32(img, h + 25);
        }
    }

    // Lua GetLocalPlayerSoulLv: GameDataMan -> PlayerGameData -> level.
    int pgd = 0, level = 0;
    if (unique("SoulLv", "48 8D 05 ? ? ? ? 48 8B 00 48 8B 48 ? 31 C0 48 85 C9 74 ? 8B 81 ? ? ? ? 5D C3", h) &&
        rip(h, 3, 7) == gdm) {
        pgd = img[h + 13];
        level = ReadI32(img, h + 23);
    }

    // Lua GetClearCount: the NG+ cycle (optional).
    if (unique("ClearCount", "48 8D 0D ? ? ? ? 31 C0 48 83 39 00 74 ? 48 8D 05 ? ? ? ? 48 8B 00 8B 40 ? C3", h) &&
        rip(h, 18, 22) == gdm)
        out.offClearCount = img[h + 27];

    bool sane = gdm > 0 && gdm + 8 <= img.size() && out.offDeaths > 0 && out.offDeaths < 0x1000 &&
                playTime > 0 && playTime < 0x1000 && pgd > 0 && level > 0 && level < 0x1000;
    if (!sane) {
        log += "GameDataMan    -> NÃO ENCONTRADO\n";
        return false;
    }
    out.gameDataMan = gdm;
    out.offPlayTime = playTime;
    out.offPlayerData = pgd;
    out.offSoulLevel = level;
    out.offName = level + kNameAfterLevel;
    snprintf(line, sizeof(line), "GameDataMan    -> eboot+%llX\n", (unsigned long long)gdm);
    log += line;
    char ng[16] = "-";
    if (out.offClearCount >= 0) snprintf(ng, sizeof(ng), "+%X", out.offClearCount);
    snprintf(line, sizeof(line), "mortes +%X | AddTrueDeathCount +%X | tempo +%X | NG %s | jogador +%X -> nível +%X, nome +%X\n",
             out.offDeaths, out.offTrueDeaths, out.offPlayTime, ng, out.offPlayerData, out.offSoulLevel, out.offName);
    log += line;
    return true;
}

bool FlatImageFromSelf(const std::vector<uint8_t>& self, std::vector<uint8_t>& img, std::string& err)
{
    auto in = [&](uint64_t off, uint64_t n) { return off <= self.size() && n <= self.size() - off; };
    auto rd = [&](uint64_t off, auto& v) { memcpy(&v, self.data() + off, sizeof(v)); };
    if (!in(0, 32) || memcmp(self.data(), "\x4F\x15\x3D\x1D", 4) != 0) { err = "não é um SELF de PS4"; return false; }
    uint16_t count;
    rd(24, count);
    uint64_t elf = 32 + uint64_t(count) * 32;
    if (!in(elf, 64) || memcmp(&self[elf], "\x7F" "ELF\x02\x01\x01", 7) != 0) { err = "ELF x86-64 esperado"; return false; }
    uint64_t phoff;
    uint16_t phentsize, phnum;
    rd(elf + 32, phoff);
    rd(elf + 54, phentsize);
    rd(elf + 56, phnum);
    if (phentsize != 56 || !in(elf + phoff, uint64_t(phnum) * 56)) { err = "cabeçalhos de programa inválidos"; return false; }

    struct Ph { uint32_t type, flags; uint64_t offset, vaddr, paddr, filesz, memsz, align; };
    std::vector<Ph> ph(phnum);
    for (uint16_t i = 0; i < phnum; ++i) rd(elf + phoff + i * 56, ph[i]);
    uint64_t size = 0;
    for (const Ph& p : ph)
        if (p.type == 1 || p.type == 0x61000010) size = std::max(size, p.vaddr + p.memsz);
    if (size == 0 || size > kMaxImage) { err = "tamanho de imagem inválido"; return false; }
    img.assign(size, 0);

    // Each SELF segment carries the file bytes of one program header.
    for (uint16_t i = 0; i < count; ++i) {
        uint64_t flags, off, fsize, msize;
        rd(32 + i * 32, flags);
        rd(32 + i * 32 + 8, off);
        rd(32 + i * 32 + 16, fsize);
        rd(32 + i * 32 + 24, msize);
        if (!(flags & 0x800)) continue;
        if (flags & 0xA) { err = "segmento criptografado ou comprimido (o port também não aceitaria este arquivo)"; return false; }
        uint64_t idx = (flags >> 20) & 0xFFF;
        if (idx >= ph.size()) { err = "índice de segmento inválido"; return false; }
        const Ph& p = ph[idx];
        if (p.type != 1 && p.type != 0x61000010) continue;
        if (!in(off, fsize) || fsize > p.memsz || p.vaddr + fsize > size) { err = "segmento fora do arquivo"; return false; }
        memcpy(&img[p.vaddr], &self[off], fsize);
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// Live process

void Reader::Detach()
{
    if (proc_) CloseHandle(proc_);
    proc_ = nullptr;
    pid_ = 0;
    ready_ = false;
    base_ = 0;
    size_ = 0;
    scanAttempts_ = 0;
    layout_ = Layout{};
    scanLog_.clear();
    lastPlayTime_ = UINT32_MAX;
    playTimeMovedAt_ = 0;
    lastTrueDeaths_ = -1;
}

bool Reader::Update(GameReader::State& state)
{
    ULONGLONG now = GetTickCount64();
    if (proc_ && WaitForSingleObject(proc_, 0) != WAIT_TIMEOUT) {
        Detach();
        state = GameReader::State::NotRunning;
        return false;
    }
    if (ready_) { state = GameReader::State::Ready; return true; }
    if (now < nextTry_) {
        if (proc_) { state = GameReader::State::Scanning; return true; }
        if (denied_) { state = GameReader::State::AccessDenied; return true; }
        return false;
    }
    nextTry_ = now + 2000;

    if (!proc_) {
        denied_ = false;
        DWORD pid = FindPortPid();
        if (!pid) return false;
        proc_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
        if (!proc_) {
            denied_ = GetLastError() == ERROR_ACCESS_DENIED;
            if (denied_) state = GameReader::State::AccessDenied;
            return denied_;
        }
        pid_ = pid;
    }
    state = GameReader::State::Scanning;
    if (FindImageBase() && Scan()) {
        ready_ = true;
        state = GameReader::State::Ready;
    } else {
        // The port maps and links the game a few seconds after it starts; try again shortly.
        ++scanAttempts_;
        nextTry_ = now + (scanAttempts_ < 10 ? 3000 : 30000);
    }
    return true;
}

bool Reader::FindImageBase()
{
    auto isImage = [&](uintptr_t a) {
        char b[sizeof(kImageMarker)] = {};
        SIZE_T n = 0;
        return ReadProcessMemory(proc_, (LPCVOID)a, b, sizeof(b), &n) && n == sizeof(b) &&
               memcmp(b, kImageMarker, sizeof(b)) == 0;
    };
    base_ = 0;
    if (isImage(kGuestBase)) {
        base_ = kGuestBase;
    } else {
        // Other port versions may put it elsewhere: check the start of every executable allocation.
        const DWORD exec = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        for (uintptr_t a = 0x10000; a < 0x7FFFFFFF0000ull;) {
            MEMORY_BASIC_INFORMATION mbi{};
            if (!VirtualQueryEx(proc_, (LPCVOID)a, &mbi, sizeof(mbi))) break;
            uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            if (mbi.State == MEM_COMMIT && mbi.BaseAddress == mbi.AllocationBase && (mbi.Protect & exec) && isImage(a)) {
                base_ = a;
                break;
            }
            if (next <= a) break;
            a = next;
        }
    }
    if (!base_) return false;

    // The image is the run of committed memory that starts there.
    size_ = 0;
    for (uintptr_t a = base_; a - base_ < kMaxImage;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(proc_, (LPCVOID)a, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT) break;
        a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        size_ = std::min<size_t>(a - base_, kMaxImage);
    }
    return size_ >= 0x100000;
}

bool Reader::Scan()
{
    std::vector<uint8_t> img(size_, 0);
    for (uintptr_t addr = base_; addr < base_ + size_;) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQueryEx(proc_, (LPCVOID)addr, &mbi, sizeof(mbi))) break;
        uintptr_t regionEnd = std::min<uintptr_t>((uintptr_t)mbi.BaseAddress + mbi.RegionSize, base_ + size_);
        bool readable = mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
        if (readable) {
            SIZE_T n = 0;
            ReadProcessMemory(proc_, (LPCVOID)addr, img.data() + (addr - base_), regionEnd - addr, &n);
        }
        addr = regionEnd;
    }
    scanLog_.clear();
    return AnalyzeImage(img, layout_, scanLog_);
}

uintptr_t Reader::ReadPtr(uintptr_t addr) const
{
    uintptr_t v = 0;
    if (!ReadT(addr, v) || !ValidPtr(v)) return 0;
    return v;
}

bool Reader::Read(GameSnapshot& s)
{
    s = GameSnapshot{};
    s.game = "bb";
    if (!ready_) return false;
    ULONGLONG now = GetTickCount64();

    uintptr_t gdm = ReadPtr(base_ + (uintptr_t)layout_.gameDataMan);
    if (!gdm) return true;
    s.gameDataOk = true;
    ReadT(gdm + layout_.offDeaths, s.deaths);
    ReadT(gdm + layout_.offPlayTime, s.playTimeMs);
    ReadT(gdm + layout_.offTrueDeaths, lastTrueDeaths_);
    if (layout_.offClearCount >= 0 && ReadT(gdm + layout_.offClearCount, s.ngPlus) && (s.ngPlus < 0 || s.ngPlus > 999))
        s.ngPlus = 0;

    uintptr_t pgd = ReadPtr(gdm + layout_.offPlayerData);
    if (pgd) {
        wchar_t nameBuf[17] = {};
        SIZE_T n = 0;
        if (ReadProcessMemory(proc_, (LPCVOID)(pgd + layout_.offName), nameBuf, 32, &n)) {
            std::wstring name;
            bool ok = true;
            for (int i = 0; i < 16 && nameBuf[i]; ++i) {
                if (nameBuf[i] < 0x20 || nameBuf[i] == 0xFFFF) { ok = false; break; }
                name += nameBuf[i];
            }
            if (ok) s.name = name;
        }
        if (!ReadT(pgd + layout_.offSoulLevel, s.soulLevel) || s.soulLevel < 0 || s.soulLevel > 999) s.soulLevel = 0;
    }
    // The name is only used to tell characters apart; counting works without it.
    s.charLoaded = pgd && s.playTimeMs > 0 && s.deaths >= 0 && s.deaths < 10000000;

    // No player-entity pointer is read: "in the world" means the play time is running.
    if (s.playTimeMs != lastPlayTime_) {
        if (lastPlayTime_ != UINT32_MAX) playTimeMovedAt_ = now;
        lastPlayTime_ = s.playTimeMs;
    }
    s.inWorld = s.charLoaded && playTimeMovedAt_ != 0 && now - playTimeMovedAt_ < 3000;
    return true;
}

std::wstring Reader::ProcessPath() const
{
    if (!proc_) return {};
    wchar_t buf[MAX_PATH * 2];
    DWORD n = (DWORD)std::size(buf);
    if (!QueryFullProcessImageNameW(proc_, 0, buf, &n)) return {};
    return std::wstring(buf, n);
}

std::string Reader::VersionName() const
{
    std::wstring path = ProcessPath();
    size_t slash = path.find_last_of(L"\\/");
    std::wstring file = slash == std::wstring::npos ? path : path.substr(slash + 1);
    std::string exe;
    for (wchar_t c : file) exe += (c < 0x80) ? (char)c : '?';
    return exe.empty() ? std::string("port de PC") : "port de PC, " + exe;
}

std::string Reader::DiagnosticsText(GameReader::State state) const
{
    char b[512];
    const char* st = "jogo não encontrado";
    switch (state) {
        case GameReader::State::AccessDenied: st = "acesso negado (o jogo está rodando como administrador?)"; break;
        case GameReader::State::Scanning:     st = "analisando a memória do jogo..."; break;
        case GameReader::State::Ready:        st = "conectado"; break;
        default: break;
    }
    snprintf(b, sizeof(b), "Jogo: Bloodborne, %s\nEstado: %s\nPID: %lu\nImagem do jogo: %llX  Tamanho: %zX\nTentativas de análise: %d\n",
             VersionName().c_str(), st, pid_, (unsigned long long)base_, size_, scanAttempts_);
    std::string t = b;
    t += scanLog_;
    if (ready_) {
        snprintf(b, sizeof(b), "AddTrueDeathCount (só informativo): %d\n", lastTrueDeaths_);
        t += b;
    }
    return t;
}

} // namespace bb

namespace {

std::string NoCheckpoint(int) { return {}; }

const std::vector<std::string>& NoAreas()
{
    static const std::vector<std::string> list;
    return list;
}

const std::vector<std::string>& BloodborneCauses()
{
    static const std::vector<std::string> list = {
        "Queda", "Inimigo comum", "Armadilha", "Veneno", "Frenesi",
        "Chefe: Cleric Beast", "Chefe: Father Gascoigne", "Chefe: Vicar Amelia", "Chefe: Blood-starved Beast",
        "Chefe: Witch of Hemwick", "Chefe: Darkbeast Paarl", "Chefe: Shadow of Yharnam", "Chefe: Rom, the Vacuous Spider",
        "Chefe: The One Reborn", "Chefe: Micolash, Host of the Nightmare", "Chefe: Martyr Logarius", "Chefe: Amygdala",
        "Chefe: Celestial Emissary", "Chefe: Ebrietas, Daughter of the Cosmos", "Chefe: Mergo's Wet Nurse",
        "Chefe: Gehrman, the First Hunter", "Chefe: Moon Presence", "Chefe: Ludwig", "Chefe: Laurence, the First Vicar",
        "Chefe: Living Failures", "Chefe: Lady Maria", "Chefe: Orphan of Kos",
    };
    return list;
}

} // namespace

// Attached by bb::Reader (the port maps the PS4 game inside bbport.exe), not by GameReader's scan.
const GameInfo kBloodborneInfo = {
    "bb", L"bbport.exe", "Bloodborne (port de PC)", "BB", nullptr,
    NoCheckpoint, NoAreas, BloodborneCauses, "Última lamparina", true, false, true,
};

