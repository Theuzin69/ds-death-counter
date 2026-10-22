#include "games/eldenring.h"
#include "games/eldenring_data.h"
#include "game_reader.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwctype>

#pragma comment(lib, "advapi32.lib")   // service status queries (EAC)

namespace {

int32_t I32(const std::vector<uint8_t>& img, size_t at)
{
    int32_t v = 0;
    if (at + 4 <= img.size()) memcpy(&v, img.data() + at, 4);
    return v;
}

bool ValidPtr(uintptr_t p) { return p >= 0x10000 && p < 0x7FFFFFFFFFFFull; }

std::wstring Lower(std::wstring s)
{
    for (auto& c : s) c = (wchar_t)towlower(c);
    return s;
}

// Product version from the VS_FIXEDFILEINFO in the image's resources (mapped with the exe).
bool ImageVersion(const std::vector<uint8_t>& img, uint32_t& ms, uint32_t& ls)
{
    static const uint8_t sig[] = { 0xBD, 0x04, 0xEF, 0xFE, 0x00, 0x00, 0x01, 0x00 };   // signature, struct version 1.0
    auto it = std::search(img.begin(), img.end(), std::begin(sig), std::end(sig));
    if (it == img.end() || img.end() - it < 24) return false;
    size_t at = (size_t)(it - img.begin());
    memcpy(&ms, img.data() + at + 16, 4);
    memcpy(&ls, img.data() + at + 20, 4);
    return true;
}

std::string ErGraceName(int id) { return ErGraceLabel((uint32_t)id); }

std::unique_ptr<GameModule> CreateEldenRing() { return std::make_unique<EldenRingModule>(); }

// Game minor version: 1.02 -> 2, ..., 1.09 -> 9, 2.0 (= 1.10) -> 10, 2.6 (= 1.16) -> 16. 0 = unknown.
int GameMinor(uint32_t ms)
{
    int major = HIWORD(ms), minor = LOWORD(ms);
    if (major == 1) return minor;
    if (major == 2) return minor + 10;
    return 0;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Easy Anti-Cheat

ErEacCheck ErDetectEac(DWORD gamePid)
{
    ErEacCheck r;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        r.active = true;   // can't tell: stay on the safe side
        r.reason = "não foi possível listar os processos";
        return r;
    }
    struct Proc { DWORD pid; std::wstring name; };
    std::vector<Proc> procs;
    DWORD parent = 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        procs.push_back({ pe.th32ProcessID, Lower(pe.szExeFile) });
        if (pe.th32ProcessID == gamePid) parent = pe.th32ParentProcessID;
    }
    CloseHandle(snap);

    for (const auto& p : procs) {
        if (p.name.find(L"easyanticheat") != std::wstring::npos || p.name == L"start_protected_game.exe") {
            r.active = true;
            r.reason = "processo " + ErUtf8(p.name) + " em execução";
            return r;
        }
    }
    for (const auto& p : procs) {
        if (parent && p.pid == parent && p.name == L"start_protected_game.exe") {
            r.active = true;
            r.reason = "o jogo foi aberto pelo start_protected_game.exe (EAC)";
            return r;
        }
    }
    if (SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT)) {
        for (const wchar_t* name : { L"EasyAntiCheat_EOS", L"EasyAntiCheat" }) {
            SC_HANDLE svc = OpenServiceW(scm, name, SERVICE_QUERY_STATUS);
            if (!svc) continue;
            SERVICE_STATUS st{};
            if (QueryServiceStatus(svc, &st) && st.dwCurrentState != SERVICE_STOPPED) {
                r.active = true;
                r.reason = "serviço " + ErUtf8(name) + " ativo";
            }
            CloseServiceHandle(svc);
            if (r.active) break;
        }
        CloseServiceHandle(scm);
    }
    return r;
}

std::string ErVersionName(uint32_t ms, uint32_t ls)
{
    int minor = GameMinor(ms);
    if (!minor) return "?";
    char b[32];
    snprintf(b, sizeof(b), "1.%02d.%d", minor, HIWORD(ls));
    return b;
}

// ---------------------------------------------------------------------------------------------
// Memory route

const GameInfo kEldenRingInfo = {
    "er", L"eldenring.exe", "Elden Ring", "ER", CreateEldenRing,
    ErGraceName, ErAllMapNames, ErCauseSuggestions, "Última graça (área)", true, true, false, ErRegionOf,
};

// Before any handle exists: two checks two seconds apart without a sign of EAC, and never again
// once EAC was seen during this run of the game (GameReader makes a new module per run).
GameModule::Access EldenRingModule::CheckAccess(DWORD pid, std::string& reason)
{
    if (!eacSeen_) {
        ErEacCheck eac = ErDetectEac(pid);
        if (eac.active) {
            eacSeen_ = true;
            eacReason_ = "Easy Anti-Cheat ativo (" + eac.reason + ")";
        } else {
            ++eacClearChecks_;
        }
    }
    save_.SetAllowCoop(!eacSeen_);   // Seamless Co-op saves only exist without EAC
    if (eacSeen_) {
        reason = eacReason_;
        return Access::Never;
    }
    if (eacClearChecks_ < 2) {
        reason = "verificando o Easy Anti-Cheat";
        return Access::Wait;
    }
    reason = "Easy Anti-Cheat não detectado";
    return Access::Open;
}

std::string EldenRingModule::WithoutProcessStatus() const
{
    return save_.ActiveSlot() >= 0 ? "pelo arquivo de save" : "aguardando o jogo salvar para saber qual é o personagem";
}

std::string EldenRingModule::VersionLabel(uint32_t) const
{
    return versionMS_ ? ErVersionName(versionMS_, versionLS_) : std::string();
}

std::string EldenRingModule::LiveDiagnostics() const
{
    return save_.StatusText();
}

bool EldenRingModule::Analyze(const std::vector<uint8_t>& img, uintptr_t base, uint32_t, std::string& scanLog_)
{
    allUnique_ = true;
    char line[256];
    gameDataMan_ = worldChrMan_ = 0;
    uint32_t ms = 0, ls = 0;
    ImageVersion(img, ms, ls);
    versionMS_ = ms;
    versionLS_ = ls;

    // GameDataMan: several independent code sites must agree. The first two are the game's own
    // "add to the death counter" and "advance play time" functions, which also give the offsets.
    struct Sig { const char* name; const char* pattern; int fieldAt; };
    const Sig gdmSigs[] = {
        { "mortes", "48 8B 15 ? ? ? ? 48 85 D2 74 ? 44 8B 82 ? ? ? ? 8B C1 F7 D0 44 3B C0 76 ? C7 82 ? ? ? ? FF FF FF FF C3", 15 },
        { "tempo", "48 8B 0D ? ? ? ? 0F 28 C8 F3 0F 59 0D ? ? ? ? F3 48 0F 2C C1 01 81 ? ? ? ? 48 8B 05 ? ? ? ? 81 B8 ? ? ? ? 18 A0 93 D6", 25 },
        { "FD4Time", "48 8B 05 ? ? ? ? 4C 8B 40 08 4D 85 C0 74 0D 45 0F B6 80 BE 00 00 00 E9 13 00 00 00", -1 },
        { "NG", "48 8B 05 ? ? ? ? 48 8D 4D C0 41 B8 10 00 00 00 48 8B 10 48 83 C2 1C", -1 },
        { "DSDeaths", "48 8B 05 ? ? ? ? 48 85 C0 74 05 48 8B 40 58 C3 C3", -1 },
    };
    bool divergent = false;
    offDeaths_ = 0x94;
    offPlayTime_ = 0xA0;
    for (const auto& sig : gdmSigs) {
        auto hits = FindPattern(img, sig.pattern);
        if (hits.size() != 1) {
            allUnique_ = false;
            snprintf(line, sizeof(line), "GameDataMan (%s) -> %s\n", sig.name, hits.empty() ? "NÃO ENCONTRADO" : "AMBÍGUO");
            scanLog_ += line;
            continue;
        }
        uintptr_t g = base + hits[0] + 7 + I32(img, hits[0] + 3);
        if (gameDataMan_ && g != gameDataMan_) divergent = true;
        if (!gameDataMan_) gameDataMan_ = g;
        int field = sig.fieldAt >= 0 ? I32(img, hits[0] + sig.fieldAt) : 0;
        if (sig.fieldAt >= 0 && field > 0x40 && field < 0x400) (sig.fieldAt == 15 ? offDeaths_ : offPlayTime_) = field;
        if (sig.fieldAt >= 0)
            snprintf(line, sizeof(line), "GameDataMan (%s) -> exe+%llX | +%X\n", sig.name, (unsigned long long)(g - base), field);
        else
            snprintf(line, sizeof(line), "GameDataMan (%s) -> exe+%llX\n", sig.name, (unsigned long long)(g - base));
        scanLog_ += line;
    }
    if (divergent) {
        scanLog_ += "GameDataMan -> DIVERGENTE (assinaturas apontam para endereços diferentes)\n";
        gameDataMan_ = 0;
        allUnique_ = false;
    }

    // WorldChrMan and the player's offset inside it, read from the same instruction sequence.
    auto wcm = FindPattern(img, "48 8B 35 ? ? ? ? 48 85 F6 ? ? BB 01 00 00 00 89 5C 24 20 48 8B B6");
    if (wcm.size() == 1) {
        worldChrMan_ = base + wcm[0] + 7 + I32(img, wcm[0] + 3);
        int off = I32(img, wcm[0] + 0x18);
        if (off > 0x1000 && off < 0x80000) offPlayerIns_ = off;
        snprintf(line, sizeof(line), "WorldChrMan -> exe+%llX | jogador +%X\n", (unsigned long long)(worldChrMan_ - base), offPlayerIns_);
    } else {
        allUnique_ = false;
        snprintf(line, sizeof(line), "WorldChrMan -> %s\n", wcm.empty() ? "NÃO ENCONTRADO" : "AMBÍGUO");
    }
    scanLog_ += line;

    // Position and map id of the player moved between patches (values from SoulSplitter, the
    // 1.02 ones confirmed in that build's code).
    int minor = GameMinor(ms);
    if (minor && minor <= 3)      { offPos_ = 0x6B8; offMap_ = 0x6C8; }
    else if (minor && minor <= 7) { offPos_ = 0x6B0; offMap_ = 0x6C0; }
    else                          { offPos_ = 0x6D4; offMap_ = 0x6D0; }
    snprintf(line, sizeof(line), "versão %s | mortes +%X | tempo +%X | posição +%X | mapa +%X\n",
             ErVersionName(ms, ls).c_str(), offDeaths_, offPlayTime_, offPos_, offMap_);
    scanLog_ += line;
    return gameDataMan_ != 0;
}

bool EldenRingModule::Read(HANDLE proc, GameSnapshot& s)
{
    auto rd = [&](uintptr_t a, void* v, size_t n) {
        SIZE_T got = 0;
        return a && ReadProcessMemory(proc, (LPCVOID)a, v, n, &got) && got == n;
    };
    auto ptr = [&](uintptr_t a) -> uintptr_t {
        uintptr_t v = 0;
        return rd(a, &v, sizeof(v)) && ValidPtr(v) ? v : 0;
    };
    s.game = "er";
    uintptr_t gdm = ptr(gameDataMan_);
    if (!gdm) return true;
    s.gameDataOk = true;
    uint32_t deaths = 0, playTime = 0;
    int ng = 0;
    rd(gdm + offDeaths_, &deaths, 4);
    rd(gdm + offPlayTime_, &playTime, 4);
    rd(gdm + 0x120, &ng, 4);
    s.deaths = (int)std::min<uint32_t>(deaths, 10000000);
    s.playTimeMs = playTime;
    s.ngPlus = (ng >= 0 && ng < 100) ? ng : 0;

    uintptr_t pgd = ptr(gdm + 0x8);
    if (pgd) {
        wchar_t buf[17] = {};
        if (rd(pgd + 0x9C, buf, 32)) {
            std::wstring name;
            bool ok = true;
            for (int i = 0; i < 16 && buf[i]; ++i) {
                if (buf[i] < 0x20 || buf[i] == 0xFFFF) { ok = false; break; }
                name += buf[i];
            }
            if (ok) s.name = name;
        }
        int level = 0;
        if (rd(pgd + 0x68, &level, 4) && level > 0 && level < 1000) s.soulLevel = level;
    }

    // The save slot is not kept in a place we read, so match the character with the save file.
    int slot = save_.MatchSlot(s.name, s.soulLevel, playTime);
    ULONGLONG now = GetTickCount64();
    if (s.name != matchName_) { matchName_ = s.name; matchSince_ = now; }
    bool waitForSave = slot < 0 && now - matchSince_ < 15000;   // a new character is saved right away
    if (slot >= 0) {
        s.saveSlot = slot;
        const ErSaveSlot& sl = save_.data().slots[slot];
        if (sl.valid) s.lastBonfire = (int)sl.lastGrace;
    }
    s.charLoaded = pgd && playTime > 0 && deaths <= 10000000 && !waitForSave;

    if (uintptr_t wcm = ptr(worldChrMan_)) {
        if (uintptr_t pins = ptr(wcm + offPlayerIns_)) {
            s.inWorld = true;
            uint32_t map = 0;
            if (rd(pins + offMap_, &map, 4) && ErMapIdValid(map)) {
                s.mapRaw = map;
                s.mapArea = (int)(map >> 24);
                s.mapBlock = (int)((map >> 16) & 0xFF);
                s.areaName = ErMapName(map);
            }
            float p[3];
            if (rd(pins + offPos_, p, sizeof(p)) && std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]) &&
                std::fabs(p[0]) < 1e5f && std::fabs(p[1]) < 1e5f && std::fabs(p[2]) < 1e5f) {
                // Open world positions are relative to the 256 m tile; make them continuous across tiles.
                unsigned area = map >> 24;
                if (s.mapRaw && (area == 60 || area == 61)) {
                    p[0] += ((map >> 16) & 0xFF) * 256.0f + 128.0f;
                    p[2] += ((map >> 8) & 0xFF) * 256.0f + 128.0f;
                }
                s.pos = { p[0], p[1], p[2] };
                s.hasPos = true;
            }
            if (uintptr_t modules = ptr(pins + 0x190))
                if (uintptr_t data = ptr(modules)) {
                    int hp[2] = { -1, -1 };
                    if (rd(data + 0x138, hp, sizeof(hp)) && hp[0] >= 0 && hp[1] > 0 && hp[0] <= hp[1] && hp[1] < 100000)
                        s.hp = hp[0];
                }
        }
    }
    return true;
}

bool EldenRingModule::ReadWithoutProcess(GameSnapshot& s)
{
    s.game = "er";
    if (!save_.HasData()) return true;
    s.gameDataOk = true;
    int k = save_.ActiveSlot();
    if (k < 0) return true;
    const ErSaveSlot& sl = save_.data().slots[k];
    if (!sl.valid) return true;
    s.charLoaded = true;
    s.inWorld = true;   // unknown without touching the game; the game is running
    s.deaths = (int)sl.deaths;
    s.playTimeMs = (uint32_t)std::min<uint64_t>(sl.playTimeMs, 0xFFFFFFFFull);
    s.name = sl.name;
    s.saveSlot = k;
    s.soulLevel = sl.level;
    s.ngPlus = sl.detailOk && sl.ngPlus >= 0 ? sl.ngPlus : 0;
    s.lastBonfire = (int)sl.lastGrace;
    // After a death the save has the new bloodstain: that is where the character died.
    uint32_t map = sl.bloodstainChanged && sl.bloodstainActive ? sl.bloodstainMap : sl.mapId;
    if (ErMapIdValid(map)) {
        s.mapRaw = map;
        s.mapArea = (int)(map >> 24);
        s.mapBlock = (int)((map >> 16) & 0xFF);
        s.areaName = ErMapName(map);
    }
    return true;
}
