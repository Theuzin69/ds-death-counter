#include "games/eldenring_save.h"
#include "games/eldenring_data.h"
#include <bcrypt.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace {

constexpr size_t kSlotSize = 0x280000;
constexpr size_t kUserData10Size = 0x60000;
constexpr size_t kEventFlagsSize = 0x1BF99F;
constexpr size_t kPreFlagsBlock = 0x1D;     // 3 bytes, deaths, character type, ..., last grace, ...
constexpr size_t kSummaryOffset = 0x1964;   // in USER_DATA010, counting its MD5
constexpr size_t kSummarySize = 0x24C;

struct View {
    const uint8_t* p;
    size_t n;
    bool has(size_t at, size_t len) const { return at <= n && len <= n - at; }
    uint32_t u32(size_t at) const { uint32_t v = 0; if (has(at, 4)) memcpy(&v, p + at, 4); return v; }
    int32_t i32(size_t at) const { return (int32_t)u32(at); }
    uint64_t u64(size_t at) const { uint64_t v = 0; if (has(at, 8)) memcpy(&v, p + at, 8); return v; }
};

bool Md5(const uint8_t* data, size_t len, uint8_t out[16])
{
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = false;
    if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, nullptr, 0))) {
        if (BCRYPT_SUCCESS(BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0))) {
            ok = BCRYPT_SUCCESS(BCryptHashData(h, (PUCHAR)data, (ULONG)len, 0)) &&
                 BCRYPT_SUCCESS(BCryptFinishHash(h, out, 16, 0));
            BCryptDestroyHash(h);
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    return ok;
}

std::wstring ReadName(const View& v, size_t at)
{
    std::wstring name;
    for (int i = 0; i < 16 && v.has(at + i * 2, 2); ++i) {
        wchar_t c = (wchar_t)(v.p[at + i * 2] | v.p[at + i * 2 + 1] << 8);
        if (!c) break;
        if (c < 0x20 || c == 0xFFFF) return {};
        name += c;
    }
    return name;
}

// From the block that holds the death counter, walks the rest of the slot (event flags, the five
// world state blocks, position, NetMan...) and checks that it ends on the account's Steam id.
// Any misplaced start makes the walk miss it, so a pass means the counter really is at `g`.
bool ChainOk(const View& s, size_t g, uint32_t version, uint64_t steamId)
{
    size_t p = g + kPreFlagsBlock + kEventFlagsSize + 1;
    for (int i = 0; i < 5; ++i) {                 // FieldArea, WorldArea, WorldGeomMan x2, RendMan
        if (!s.has(p, 4)) return false;
        uint32_t size = s.u32(p);
        if (size > 0x100000) return false;
        p += 4 + size;
    }
    if (!s.has(p, 0x3D)) return false;
    uint32_t coordMap = s.u32(p + 12);
    p += 0x3D;                                     // PlayerCoordinates
    p += 10 + (version >= 65 ? 4 : 0) + (version >= 66 ? 1 : 0);   // GameMan values
    p += 0x20004 + 0xC + 0xC + 0x10;              // NetMan, weather, time of day, base version
    if (!s.has(p, 8)) return false;
    if (steamId) return s.u64(p) == steamId;
    return s.u64(p) == 0 && coordMap == s.u32(4);
}

// Full sequential parse up to the tutorial data (layout of current saves; older ones differ only
// in the number of gaitem entries). Gives NG+ and the bloodstain; must land exactly on `tutorial`.
bool ParseDetail(const View& s, size_t tutorial, ErSaveSlot& out)
{
    size_t p = 0x20;
    int entries = out.version <= 81 ? 5118 : 5120;
    for (int i = 0; i < entries; ++i) {
        if (!s.has(p, 8)) return false;
        uint32_t top = s.u32(p) >> 28;
        p += top == 8 ? 21 : top == 9 ? 16 : 8;
    }
    p += 0x1B0 + 0xD0 + 0x58 + 0x1C + 0x58 + 0x58 + 0x9010 + 0x74 + 0x8C + 0x18;
    if (!s.has(p, 4) || s.u32(p) > 0x10000) return false;
    p += 4 + (size_t)s.u32(p) * 8;                 // acquired projectiles
    p += 0x9C + 0xC + 0x12F + 0x6010 + 0x100;
    if (!s.has(p, 4) || s.u32(p) > 0x10000) return false;
    p += 4 + (size_t)s.u32(p) * 4;                 // unlocked regions
    p += 0x28 + 1;                                 // Torrent, one byte
    size_t blood = p;
    p += 0x44;
    int ng = s.i32(p);
    p += 8;
    if (!s.has(p, 8) || s.u32(p + 4) > 0x10000) return false;
    p += 8 + s.u32(p + 4);                         // menu data
    p += 0x34 + 0x1B588;                           // trophy data, item history
    if (p != tutorial) return false;

    out.detailOk = true;
    out.ngPlus = (ng >= 0 && ng < 100) ? ng : -1;
    out.bloodstainMap = s.u32(blood + 0x38);
    out.bloodstainActive = s.i32(blood + 0x30) == 0 && ErMapIdValid(out.bloodstainMap);
    memcpy(out.bloodstain, s.p + blood, sizeof(out.bloodstain));
    out.bloodstainAt = (uint32_t)blood;
    return true;
}

void ParseSlot(const View& s, uint64_t steamId, ErSaveSlot& out)
{
    out.version = s.u32(0);
    out.mapId = s.u32(4);
    out.playTimeMs = s.u64(8);
    if (!out.version) { out.error = "vazio"; return; }

    // The counter is right after the tutorial data, whose header is "uint16 174, uint16 1, uint32 size".
    size_t found = 0, tutorial = 0;
    int hits = 0;
    size_t end = std::min(s.n, (size_t)0x200000);
    for (size_t t = 0x8000; t + 8 < end; ++t) {
        const uint8_t* q = (const uint8_t*)memchr(s.p + t, 0xAE, end - t);
        if (!q) break;
        t = (size_t)(q - s.p);
        if (q[1] != 0 || q[2] != 1 || q[3] != 0) continue;
        uint32_t size = s.u32(t + 4);
        if (size < 8 || size > 0x10000) continue;
        size_t g = t + 8 + size;
        if (ChainOk(s, g, out.version, steamId)) { found = g; tutorial = t; ++hits; }
    }
    if (hits != 1) {
        out.error = hits ? "contador de mortes ambíguo" : "contador de mortes não localizado (formato desconhecido?)";
        return;
    }
    out.deaths = s.u32(found + 3);
    out.deathsAt = (uint32_t)(found + 3);
    out.lastGrace = s.u32(found + 0x10);
    if (out.deaths > 10000000) { out.error = "contador de mortes inválido"; return; }
    out.valid = true;
    ParseDetail(s, tutorial, out);
}

} // namespace

std::string ErUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n > 0 ? n : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

bool ErParseSave(const std::vector<uint8_t>& file, ErSaveData& out)
{
    out = ErSaveData{};
    View f{ file.data(), file.size() };
    if (!f.has(0, 0x40) || memcmp(file.data(), "BND4", 4) != 0) { out.error = "não é um arquivo BND4"; return false; }
    uint32_t count = f.u32(0x0C);
    if (count < 11 || count > 64 || !f.has(0x40, count * 0x20)) { out.error = "cabeçalho inesperado"; return false; }
    auto entrySize = [&](int i) { return f.u64(0x40 + i * 0x20 + 8); };
    auto entryOff = [&](int i) { return (size_t)f.u32(0x40 + i * 0x20 + 0x10); };

    // USER_DATA010: Steam id and the load screen summary of each slot.
    size_t ud = entryOff(10);
    if (entrySize(10) < kUserData10Size + 0x10 || !f.has(ud, kUserData10Size + 0x10)) { out.error = "USER_DATA010 ausente"; return false; }
    uint8_t md5[16];
    if (!Md5(file.data() + ud + 0x10, kUserData10Size, md5) || memcmp(md5, file.data() + ud, 16) != 0) {
        out.error = "checksum do USER_DATA010 não confere (arquivo sendo gravado?)";
        return false;
    }
    out.steamId = f.u64(ud + 0x14);
    size_t sum = ud + kSummaryOffset;
    for (int i = 0; i < 10; ++i) {
        uint8_t used = file[sum + i];
        if (used > 1) { out.error = "resumo dos personagens inesperado"; return false; }
        ErSaveSlot& sl = out.slots[i];
        sl.used = used == 1;
        size_t prof = sum + 10 + i * kSummarySize;
        if (sl.used) {
            sl.name = ReadName(f, prof);
            sl.level = (int)f.u32(prof + 0x22);
        }
    }

    for (int i = 0; i < 10; ++i) {
        ErSaveSlot& sl = out.slots[i];
        size_t off = entryOff(i);
        if (entrySize(i) < kSlotSize + 0x10 || !f.has(off, kSlotSize + 0x10)) { sl.error = "slot ausente"; continue; }
        memcpy(sl.md5, file.data() + off, 16);
        if (!sl.used) continue;
        if (!Md5(file.data() + off + 0x10, kSlotSize, md5) || memcmp(md5, sl.md5, 16) != 0) {
            sl.error = "checksum não confere (arquivo sendo gravado?)";
            continue;
        }
        ParseSlot(View{ file.data() + off + 0x10, kSlotSize }, out.steamId, sl);
    }
    out.ok = true;
    return true;
}

// ---------------------------------------------------------------------------------------------

void ErSaveWatcher::Reset()
{
    bool coop = allowCoop_;
    *this = ErSaveWatcher{};
    allowCoop_ = coop;
}

void ErSaveWatcher::FindNewestFile()
{
    wchar_t appdata[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"APPDATA", appdata, MAX_PATH);
    if (!n || n >= MAX_PATH) return;
    std::wstring root = std::wstring(appdata) + L"\\EldenRing\\";
    std::wstring best;
    FILETIME bestTime{};
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((root + L"*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == L'.') continue;
        for (const wchar_t* name : { L"ER0000.sl2", L"ER0000.co2" }) {
            if (!allowCoop_ && wcscmp(name, L"ER0000.co2") == 0) continue;
            std::wstring file = root + fd.cFileName + L"\\" + name;
            WIN32_FILE_ATTRIBUTE_DATA fa;
            if (!GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &fa)) continue;
            if (best.empty() || CompareFileTime(&fa.ftLastWriteTime, &bestTime) > 0) {
                best = file;
                bestTime = fa.ftLastWriteTime;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (!best.empty() && best != path_) {
        bool coop = allowCoop_;
        *this = ErSaveWatcher{};
        allowCoop_ = coop;
        path_ = best;
    }
}

bool ErSaveWatcher::ReadFile(std::vector<uint8_t>& buf)
{
    // Shared read access, so the game can keep writing; the handle is closed right away.
    HANDLE f = CreateFileW(path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(f, &size) && size.QuadPart > 0 && size.QuadPart < 64ll * 1024 * 1024;
    if (ok) {
        buf.resize((size_t)size.QuadPart);
        DWORD got = 0;
        ok = ::ReadFile(f, buf.data(), (DWORD)buf.size(), &got, nullptr) && got == buf.size();
    }
    CloseHandle(f);
    return ok;
}

void ErSaveWatcher::Poll()
{
    ULONGLONG now = GetTickCount64();
    if (now < nextPoll_) return;
    nextPoll_ = now + 500;
    if (path_.empty() || now >= nextScan_) {
        FindNewestFile();
        nextScan_ = now + 10000;
    }
    if (path_.empty()) { lastError_ = "save não encontrado em %APPDATA%\\EldenRing"; return; }

    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (!GetFileAttributesExW(path_.c_str(), GetFileExInfoStandard, &fa)) { path_.clear(); return; }
    uint64_t size = (uint64_t)fa.nFileSizeHigh << 32 | fa.nFileSizeLow;
    if (CompareFileTime(&fa.ftLastWriteTime, &seenWrite_) != 0 || size != seenSize_) {
        seenWrite_ = fa.ftLastWriteTime;
        seenSize_ = size;
        seenSince_ = now;
    }
    bool parsedCurrent = parsedOnce_ && CompareFileTime(&seenWrite_, &lastWrite_) == 0 && seenSize_ == lastSize_;
    if (parsedCurrent && !retryAt_) return;
    if (retryAt_ && now < retryAt_) return;
    // Let the game finish writing before reading (the first read happens right away).
    if (parsedOnce_ && now - seenSince_ < 700) return;

    std::vector<uint8_t> buf;
    ErSaveData d;
    bool torn = !ReadFile(buf);
    if (!torn) {
        ErParseSave(buf, d);
        torn = !d.ok && d.error.find("checksum") != std::string::npos;
        for (const auto& sl : d.slots)
            if (sl.used && sl.error.find("checksum") != std::string::npos) torn = true;
    }
    if (torn && ++retries_ <= 5) {   // caught the game mid-write: try again shortly
        retryAt_ = now + 1000;
        lastError_ = "leitura incompleta, tentando de novo";
        return;
    }
    retries_ = 0;
    retryAt_ = 0;
    lastWrite_ = seenWrite_;
    lastSize_ = seenSize_;
    lastRead_ = now;
    ++reads_;
    if (!d.ok) { lastError_ = d.error.empty() ? "não foi possível ler o arquivo" : d.error; return; }
    lastError_.clear();
    // A slot that still fails its checksum after the retries keeps its last good read.
    for (int i = 0; i < 10; ++i) {
        ErSaveSlot& n = d.slots[i];
        if (n.used && !n.valid && n.error.find("checksum") != std::string::npos && data_.ok && data_.slots[i].valid) {
            n = data_.slots[i];
            lastError_ = "slot " + std::to_string(i + 1) + " com checksum inválido; mantendo a leitura anterior";
        }
    }

    // The slot the game rewrote is the one being played (several changed: the one whose play time grew most).
    if (parsedOnce_ && data_.ok) {
        int best = -1;
        int64_t bestGain = INT64_MIN;
        for (int i = 0; i < 10; ++i) {
            const ErSaveSlot& n = d.slots[i];
            const ErSaveSlot& o = data_.slots[i];
            if (!n.used || memcmp(n.md5, o.md5, 16) == 0) continue;
            int64_t gain = (int64_t)n.playTimeMs - (int64_t)o.playTimeMs;
            if (gain > bestGain) { bestGain = gain; best = i; }
            // The bloodstain moves when the character dies.
            d.slots[i].bloodstainChanged = n.detailOk && o.detailOk && memcmp(n.bloodstain, o.bloodstain, sizeof(n.bloodstain)) != 0;
        }
        if (best >= 0) activeSlot_ = best;
    }
    if (activeSlot_ < 0) {
        int used = 0, last = -1;
        for (int i = 0; i < 10; ++i)
            if (d.slots[i].used) { ++used; last = i; }
        if (used == 1) activeSlot_ = last;
    }
    if (activeSlot_ >= 0 && !d.slots[activeSlot_].used) activeSlot_ = -1;
    data_ = std::move(d);
    parsedOnce_ = true;
}

int ErSaveWatcher::MatchSlot(const std::wstring& name, int level, uint32_t playTimeMs) const
{
    if (!data_.ok || name.empty()) return -1;
    int best = -1;
    int64_t bestScore = INT64_MAX;
    for (int i = 0; i < 10; ++i) {
        const ErSaveSlot& sl = data_.slots[i];
        if (!sl.used || sl.name != name) continue;
        // Same name in several slots: prefer the same level, then the closest saved play time.
        int64_t diff = (int64_t)playTimeMs - (int64_t)sl.playTimeMs;
        int64_t score = (sl.level == level ? 0 : (int64_t)1 << 40) + (diff >= -60000 ? (diff < 0 ? -diff : diff) : (int64_t)1 << 39);
        if (score < bestScore) { bestScore = score; best = i; }
    }
    return best;
}

std::string ErSaveWatcher::StatusText() const
{
    std::string t;
    char b[256];
    t += "Save: " + (path_.empty() ? std::string("(não encontrado)") : ErUtf8(path_)) + "\n";
    snprintf(b, sizeof(b), "Leituras: %d", reads_);
    t += b;
    if (lastRead_) {
        snprintf(b, sizeof(b), " (última há %llu s)", (unsigned long long)((GetTickCount64() - lastRead_) / 1000));
        t += b;
    }
    t += "\n";
    if (!lastError_.empty()) t += "Aviso: " + lastError_ + "\n";
    if (!data_.ok) return t;
    snprintf(b, sizeof(b), "Steam id: %llu | slot em uso: %s\n", (unsigned long long)data_.steamId,
             activeSlot_ >= 0 ? std::to_string(activeSlot_ + 1).c_str() : "? (aguardando o jogo salvar)");
    t += b;
    for (int i = 0; i < 10; ++i) {
        const ErSaveSlot& sl = data_.slots[i];
        if (!sl.used) continue;
        std::string name = ErUtf8(sl.name);
        if (sl.valid)
            snprintf(b, sizeof(b), "  slot %d: %s | nível %d | mortes %u | %s | NG+ %d | v%u\n", i + 1, name.c_str(), sl.level,
                     sl.deaths, ErMapIdText(sl.mapId).c_str(), sl.ngPlus, sl.version);
        else
            snprintf(b, sizeof(b), "  slot %d: %s | %s | v%u\n", i + 1, name.c_str(), sl.error.c_str(), sl.version);
        t += b;
    }
    return t;
}
