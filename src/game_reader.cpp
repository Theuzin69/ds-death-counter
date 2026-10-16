#include "game_reader.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

#pragma comment(lib, "version.lib")

namespace {

const wchar_t* kGameExe = L"DarkSoulsRemastered.exe";

// Module sizes of the 1.01.x builds (from DSR-Gadget). Anything else is treated
// as 1.03 / 1.03.1, the build Steam distributes today.
bool IsOldLayout(uint32_t moduleSize)
{
    return moduleSize == 0x4869400 || moduleSize == 0x496BE00 || moduleSize == 0x37CB400;
}

struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<bool>    fixed;
};

Pattern ParsePattern(const char* text)
{
    Pattern p;
    const char* c = text;
    while (*c) {
        while (*c == ' ') ++c;
        if (!*c) break;
        if (*c == '?') {
            p.bytes.push_back(0);
            p.fixed.push_back(false);
            while (*c == '?') ++c;
        } else {
            p.bytes.push_back((uint8_t)strtoul(c, nullptr, 16));
            p.fixed.push_back(true);
            c += 2;
        }
    }
    return p;
}

// Returns every match offset of `pat` inside `buf`.
std::vector<size_t> FindAll(const std::vector<uint8_t>& buf, const char* patText)
{
    Pattern pat = ParsePattern(patText);
    std::vector<size_t> hits;
    // Search for the longest run of fixed bytes, then verify the whole pattern.
    size_t bestStart = 0, bestLen = 0;
    for (size_t i = 0; i < pat.bytes.size();) {
        if (!pat.fixed[i]) { ++i; continue; }
        size_t j = i;
        while (j < pat.bytes.size() && pat.fixed[j]) ++j;
        if (j - i > bestLen) { bestStart = i; bestLen = j - i; }
        i = j;
    }
    if (bestLen == 0 || buf.size() < pat.bytes.size()) return hits;

    auto runBegin = pat.bytes.begin() + bestStart;
    std::boyer_moore_horspool_searcher searcher(runBegin, runBegin + bestLen);
    auto it = buf.begin();
    while (true) {
        auto found = std::search(it, buf.end(), searcher);
        if (found == buf.end()) break;
        size_t runPos = (size_t)(found - buf.begin());
        if (runPos >= bestStart) {
            size_t start = runPos - bestStart;
            if (start + pat.bytes.size() <= buf.size()) {
                bool ok = true;
                for (size_t k = 0; k < pat.bytes.size() && ok; ++k)
                    if (pat.fixed[k] && buf[start + k] != pat.bytes[k]) ok = false;
                if (ok) hits.push_back(start);
            }
        }
        it = found + 1;
    }
    return hits;
}

int32_t ReadI32(const std::vector<uint8_t>& buf, size_t at)
{
    int32_t v;
    memcpy(&v, buf.data() + at, 4);
    return v;
}

DWORD FindGamePid()
{
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (_wcsicmp(pe.szExeFile, kGameExe) == 0) { pid = pe.th32ProcessID; break; }
    }
    CloseHandle(snap);
    return pid;
}

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

} // namespace

std::wstring GameReader::ProcessPath() const
{
    if (!proc_) return {};
    wchar_t buf[MAX_PATH * 2];
    DWORD n = (DWORD)std::size(buf);
    if (!QueryFullProcessImageNameW(proc_, 0, buf, &n)) return {};
    return std::wstring(buf, n);
}

std::string GameReader::VersionName() const
{
    if (!proc_) return "-";
    std::string fileVer;
    std::wstring path = ProcessPath();
    DWORD dummy = 0, sz = GetFileVersionInfoSizeW(path.c_str(), &dummy);
    if (sz) {
        std::vector<uint8_t> data(sz);
        VS_FIXEDFILEINFO* ffi = nullptr;
        UINT len = 0;
        if (GetFileVersionInfoW(path.c_str(), 0, sz, data.data()) &&
            VerQueryValueW(data.data(), L"\\", (void**)&ffi, &len) && ffi) {
            char b[64];
            snprintf(b, sizeof(b), "%u.%u.%u.%u", HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
                     HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
            fileVer = b;
        }
    }
    const char* label = "1.03.x";
    switch (size_) {
        case 0x4869400: label = "1.01"; break;
        case 0x496BE00: label = "1.01.1"; break;
        case 0x37CB400: label = "1.01.2"; break;
        case 0x3817800: label = "1.03"; break;
    }
    return std::string(label) + (fileVer.empty() ? "" : " (exe " + fileVer + ")");
}

bool GameReader::ProcessAlive() const
{
    return proc_ && WaitForSingleObject(proc_, 0) == WAIT_TIMEOUT;
}

void GameReader::Detach()
{
    if (proc_) CloseHandle(proc_);
    proc_ = nullptr;
    pid_ = 0;
    base_ = 0;
    size_ = 0;
    gameMan_ = gameDataMan_ = worldChrMan_ = 0;
    scanAttempts_ = 0;
    state_ = State::NotRunning;
}

bool GameReader::OpenGame(DWORD pid)
{
    proc_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    if (!proc_) {
        state_ = (GetLastError() == ERROR_ACCESS_DENIED) ? State::AccessDenied : State::NotRunning;
        return false;
    }
    pid_ = pid;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (snap == INVALID_HANDLE_VALUE) { Detach(); return false; }   // game still starting, retry later
    MODULEENTRY32W me{ sizeof(me) };
    for (BOOL ok = Module32FirstW(snap, &me); ok; ok = Module32NextW(snap, &me)) {
        if (_wcsicmp(me.szModule, kGameExe) == 0) {
            base_ = (uintptr_t)me.modBaseAddr;
            size_ = me.modBaseSize;
            break;
        }
    }
    CloseHandle(snap);
    if (!base_) { Detach(); return false; }
    return true;
}

bool GameReader::ScanPatterns()
{
    std::vector<uint8_t> img(size_, 0);
    // Copy the committed, readable parts of the exe image.
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
    return AnalyzeImage(img);
}

bool GameReader::AnalyzeImage(const std::vector<uint8_t>& img)
{
    scanLog_.clear();
    char line[256];
    // Resolves "mov reg, [rip+rel32]" style references to the address of a global.
    auto resolve = [&](const char* name, std::initializer_list<const char*> patterns, int relOff, int instrLen) -> uintptr_t {
        for (const char* p : patterns) {
            auto hits = FindAll(img, p);
            if (hits.size() == 1) {
                uintptr_t g = base_ + hits[0] + instrLen + ReadI32(img, hits[0] + relOff);
                snprintf(line, sizeof(line), "%-12s -> exe+%llX\n", name, (unsigned long long)(g - base_));
                scanLog_ += line;
                return g;
            }
        }
        snprintf(line, sizeof(line), "%-12s -> NÃO ENCONTRADO\n", name);
        scanLog_ += line;
        return 0;
    };

    gameMan_ = resolve("GameMan", { "48 8B 05 ? ? ? ? C6 40 18 00" }, 3, 7);
    gameDataMan_ = resolve("GameDataMan", { "48 8B 05 ? ? ? ? 48 8B 50 10 48 89 54 24 60",
                                            "48 8B 05 ? ? ? ? 48 85 C0 ? ? F3 0F 58 80 AC 00 00 00" }, 3, 7);
    worldChrMan_ = resolve("WorldChrMan", { "48 8B 0D ? ? ? ? 0F 28 F1 48 85 C9 74 ? 48 89 7C",
                                            "48 8B 05 ? ? ? ? 48 8B 48 68 48 85 C9 0F 84 ? ? ? ? 48 39 5E 10 0F 84 ? ? ? ? 48" }, 3, 7);

    // Version dependent offsets.
    newLayout_ = !IsOldLayout(size_);
    offChrMapData_ = newLayout_ ? 0x68 : 0x48;
    offHp_ = newLayout_ ? 0x3E8 : 0x3D8;

    // GameMan grew by 0x10 bytes in later patches. Measure the shift from the code
    // that stores the player's "stable position" (offset 0xB90 in 1.01).
    int shift = (size_ == 0x4869400 || size_ == 0x496BE00) ? 0 : 0x10;
    auto posHits = FindAll(img, "48 8B 05 ? ? ? ? 66 0F 7F 80 ? ? ? ? 0F 28 02 66 0F 7F 80 ? ? ? ? C6 80");
    if (posHits.size() == 1 && base_ + posHits[0] + 7 + ReadI32(img, posHits[0] + 3) == gameMan_) {
        int d = ReadI32(img, posHits[0] + 11) - 0xB90;
        if (d == 0 || d == 0x10) shift = d;
    }
    offSaveSlot_ = 0xA90 + shift;
    offLastBonfire_ = 0xB24 + shift;
    offStableMap_ = 0xBC0 + shift;
    // The map id of that stable position has its own setter; read the offset from it directly.
    auto mapHits = FindAll(img, "8B 01 48 8B 0D ? ? ? ? 89 81 ? ? ? ? C3");
    for (size_t h : mapHits) {
        if (base_ + h + 9 + ReadI32(img, h + 5) == gameMan_) {
            int d = ReadI32(img, h + 11);
            if (d > 0xA00 && d < 0xE00) offStableMap_ = d;
            break;
        }
    }
    snprintf(line, sizeof(line), "layout %s | slot +%X | fogueira +%X | mapa +%X | chrMap +%X | hp +%X\n",
             newLayout_ ? "1.03+" : "1.01", offSaveSlot_, offLastBonfire_, offStableMap_, offChrMapData_, offHp_);
    scanLog_ += line;

    return gameMan_ && gameDataMan_ && worldChrMan_;
}

void GameReader::Update()
{
    ULONGLONG now = GetTickCount64();
    if (proc_ && !ProcessAlive()) Detach();
    if (state_ == State::Ready) return;
    if (now < nextTry_) return;
    nextTry_ = now + 2000;

    if (!proc_) {
        DWORD pid = FindGamePid();
        if (!pid) { state_ = State::NotRunning; return; }
        if (!OpenGame(pid)) return;
    }
    state_ = State::Scanning;
    if (ScanPatterns()) {
        state_ = State::Ready;
    } else {
        // Code may not be fully unpacked yet right after launch; try again shortly.
        ++scanAttempts_;
        nextTry_ = now + (scanAttempts_ < 10 ? 3000 : 30000);
    }
}

uintptr_t GameReader::ReadPtr(uintptr_t addr) const
{
    uintptr_t v = 0;
    if (!ReadT(addr, v) || !ValidPtr(v)) return 0;
    return v;
}

bool GameReader::Read(GameSnapshot& s)
{
    s = GameSnapshot{};
    if (state_ != State::Ready) return false;

    if (uintptr_t gdm = ReadPtr(gameDataMan_)) {
        s.gameDataOk = true;
        ReadT(gdm + 0x98, s.deaths);
        ReadT(gdm + 0xA4, s.playTimeMs);
        ReadT(gdm + 0x78, s.ngPlus);
        uintptr_t pgd = ReadPtr(gdm + 0x10);
        if (pgd) {
            wchar_t nameBuf[17] = {};
            SIZE_T n = 0;
            if (ReadProcessMemory(proc_, (LPCVOID)(pgd + 0xA8), nameBuf, 32, &n)) {
                std::wstring name;
                bool ok = true;
                for (int i = 0; i < 16 && nameBuf[i]; ++i) {
                    if (nameBuf[i] < 0x20 || nameBuf[i] == 0xFFFF) { ok = false; break; }
                    name += nameBuf[i];
                }
                if (ok) s.name = name;
            }
            ReadT(pgd + 0x90, s.soulLevel);
        }
        // The name is only used to tell characters apart; counting works without it.
        s.charLoaded = pgd && s.playTimeMs > 0 && s.deaths >= 0 && s.deaths < 10000000;
    }

    if (uintptr_t gm = ReadPtr(gameMan_)) {
        ReadT(gm + offSaveSlot_, s.saveSlot);
        if (s.saveSlot < 0 || s.saveSlot > 9) s.saveSlot = -1;
        ReadT(gm + offLastBonfire_, s.lastBonfire);
        ReadT(gm + offStableMap_, s.mapRaw);
        // FromSoftware map ids are packed as AA BB CC DD (mAA_BB_CC_DD), area in the high byte.
        int a = (s.mapRaw >> 24) & 0xFF, b = (s.mapRaw >> 16) & 0xFF;
        if (a >= 10 && a <= 18 && b <= 3) { s.mapArea = a; s.mapBlock = b; }
        else {
            a = s.mapRaw & 0xFF; b = (s.mapRaw >> 8) & 0xFF;
            if (a >= 10 && a <= 18 && b <= 3) { s.mapArea = a; s.mapBlock = b; }
        }
    }

    if (uintptr_t wcm = ReadPtr(worldChrMan_)) {
        if (uintptr_t pins = ReadPtr(wcm + 0x68)) {
            s.inWorld = true;
            ReadT(pins + offHp_, s.hp);
            if (s.hp < 0 || s.hp > 100000) s.hp = -1;
            if (uintptr_t chrMap = ReadPtr(pins + offChrMapData_)) {
                if (uintptr_t posData = ReadPtr(chrMap + 0x28)) {
                    float p[3];
                    SIZE_T n = 0;
                    if (ReadProcessMemory(proc_, (LPCVOID)(posData + 0x10), p, sizeof(p), &n) &&
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

std::string GameReader::DiagnosticsText() const
{
    char b[512];
    std::string t;
    const char* st = "jogo não encontrado";
    switch (state_) {
        case State::AccessDenied: st = "acesso negado (o jogo está rodando como administrador?)"; break;
        case State::Scanning:     st = "analisando o executável..."; break;
        case State::Ready:        st = "conectado"; break;
        default: break;
    }
    snprintf(b, sizeof(b), "Estado: %s\nPID: %lu\nBase: %llX  Tamanho: %X\nVersão: %s\nTentativas de análise: %d\n",
             st, pid_, (unsigned long long)base_, size_, VersionName().c_str(), scanAttempts_);
    t += b;
    t += scanLog_;
    return t;
}
