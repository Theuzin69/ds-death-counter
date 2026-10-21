#include "death_tracker.h"
#include "ds1_data.h"
#include "util.h"
#include <algorithm>

namespace {

std::string AreaLabel(int area, int block)
{
    if (area < 0) return "Desconhecida";
    std::string name = Ds1MapName(area, block);
    if (!name.empty()) return name;
    char b[32];
    snprintf(b, sizeof(b), "Mapa m%02d_%02d", area, block);
    return b;
}

} // namespace

Profile* DeathTracker::active()
{
    return activeKey_.empty() ? nullptr : store_.Find(activeKey_);
}

void DeathTracker::Tick(GameReader& reader)
{
    reader.Update();
    gameState_ = reader.state();
    ULONGLONG now = GetTickCount64();

    if (!reader.IsReady() || !reader.Read(snap_)) {
        // Game closed (or not hooked yet): counting stops here and resumes when it comes back.
        if (!activeKey_.empty()) store_.Save();
        hasSnap_ = false;
        activeKey_.clear();
        candidateKey_.clear();
        inGame_ = false;
        lastInWorld_ = 0;
        history_.clear();
        return;
    }
    hasSnap_ = true;
    const GameSnapshot& s = snap_;

    if (s.inWorld) lastInWorld_ = now;
    // Loading screens (e.g. after dying) briefly remove the player; don't blink the overlay.
    inGame_ = s.inWorld || (lastInWorld_ != 0 && now - lastInWorld_ < 15000);

    // Remember where the player was while alive. HP 0 freezes the history at the moment of death.
    bool alive = s.inWorld && s.hasPos && s.hp != 0;
    if (alive) {
        history_.push_back({ now, s.pos, s.mapArea, s.mapBlock, s.lastBonfire, s.playTimeMs, s.areaName });
        while (!history_.empty() && now - history_.front().t > 10000) history_.pop_front();
    }

    if (!s.charLoaded) return;

    // Dark Souls Remastered keeps the 1.0.0 keys ("slot|name"); other games get a prefix ("ds3|slot|name").
    std::string baseKey = (s.game == "dsr" ? std::string() : s.game + "|") +
                          (s.saveSlot >= 0 ? std::to_string(s.saveSlot) : std::string("?")) + "|" + WideToUtf8(s.name);
    if (baseKey != candidateKey_) {
        candidateKey_ = baseKey;
        candidateSince_ = now;
    }
    bool stable = now - candidateSince_ >= 1500;

    Profile* p = active();
    if (!p || p->baseKey != baseKey) {
        // Only switch characters once the player is actually in the world: on the title
        // screen the slot changes while the previous character's data is still in memory.
        if (stable && s.inWorld) Activate(s, baseKey);
        return;
    }

    if (s.deaths > lastDeaths_) {
        int d = s.deaths - lastDeaths_;
        if (d <= 5) RecordDeaths(*p, d);
        else AddUntracked(*p, d);   // a jump this big is not a live death; just keep the list in sync
        lastDeaths_ = s.deaths;
        p->gameDeaths = s.deaths;
        p->playTimeMs = s.playTimeMs;
        p->lastSeen = NowLocalString();
        ++deathEvents_;
        store_.Save();
    } else if (s.deaths < lastDeaths_) {
        // The game's counter went down (an older save backup was restored): follow the game.
        lastDeaths_ = s.deaths;
        p->gameDeaths = s.deaths;
        store_.MarkDirty();
    }

    if (p->playTimeMs != s.playTimeMs || p->soulLevel != s.soulLevel || p->ngPlus != s.ngPlus) {
        p->playTimeMs = s.playTimeMs;
        p->soulLevel = s.soulLevel;
        p->ngPlus = s.ngPlus;
        p->lastSeen = NowLocalString();
        store_.MarkDirty();
    }
}

void DeathTracker::Activate(const GameSnapshot& s, const std::string& baseKey)
{
    Profile* p = nullptr;
    for (auto& q : store_.profiles)
        if (q.baseKey == baseKey) p = &q;   // newest profile using this slot+name

    // Same slot and name, but a brand-new character (deleted and recreated).
    if (p && s.playTimeMs + 120000 < p->playTimeMs && s.deaths < p->gameDeaths && s.playTimeMs < 30 * 60 * 1000)
        p = nullptr;

    if (!p) {
        Profile np;
        np.game = s.game;
        np.baseKey = baseKey;
        np.key = baseKey;
        for (int gen = 2; store_.Find(np.key); ++gen) np.key = baseKey + "#" + std::to_string(gen);
        np.baseDeaths = s.deaths;
        np.gameDeaths = s.deaths;
        np.firstSeen = NowLocalString();
        store_.profiles.push_back(std::move(np));
        p = &store_.profiles.back();
    } else if (s.deaths > p->gameDeaths) {
        AddUntracked(*p, s.deaths - p->gameDeaths);   // died while the counter was closed
        p->gameDeaths = s.deaths;
    } else if (s.deaths < p->gameDeaths) {
        p->gameDeaths = s.deaths;
    }

    p->name = s.name.empty() ? "Personagem do slot " + (s.saveSlot >= 0 ? std::to_string(s.saveSlot + 1) : std::string("?"))
                             : WideToUtf8(s.name);
    p->slot = s.saveSlot;
    p->playTimeMs = s.playTimeMs;
    p->soulLevel = s.soulLevel;
    p->ngPlus = s.ngPlus;
    p->lastSeen = NowLocalString();
    activeKey_ = p->key;
    lastDeaths_ = s.deaths;
    history_.clear();
    store_.Save();
}

void DeathTracker::AddUntracked(Profile& p, int count)
{
    int n = std::min(count, 200);
    for (int i = 0; i < n; ++i) {
        DeathRecord r;
        r.id = store_.NextRecordId();
        r.gameCount = p.gameDeaths + count - n + i + 1;
        r.time = NowLocalString();
        r.area = "-";
        r.cause = "Não registrada (contador fechado)";
        r.untracked = true;
        p.deaths.push_back(std::move(r));
    }
}

void DeathTracker::RecordDeaths(Profile& p, int count)
{
    Sample last{};
    bool haveSample = !history_.empty();
    if (haveSample) last = history_.back();

    // Fall heuristic: lost more than 10 m of height in the 4 seconds before dying.
    bool fell = false;
    if (haveSample) {
        float peak = last.pos.y;
        for (const auto& h : history_)
            if (last.t - h.t <= 4000) peak = std::max(peak, h.pos.y);
        fell = peak - last.pos.y > 10.0f;
    }

    for (int i = 0; i < count; ++i) {
        DeathRecord r;
        r.id = store_.NextRecordId();
        r.gameCount = lastDeaths_ + i + 1;
        r.time = NowLocalString();
        r.playTimeMs = snap_.playTimeMs;
        r.mapArea = haveSample ? last.mapArea : snap_.mapArea;
        r.mapBlock = haveSample ? last.mapBlock : snap_.mapBlock;
        if (snap_.game == "dsr") {
            r.area = AreaLabel(r.mapArea, r.mapBlock);
        } else {   // the game modules name the area themselves
            const std::string& name = haveSample ? last.areaName : snap_.areaName;
            r.area = name.empty() ? "Desconhecida" : name;
        }
        r.bonfire = haveSample ? last.bonfire : snap_.lastBonfire;
        if (haveSample) {
            r.hasPos = true;
            r.x = last.pos.x; r.y = last.pos.y; r.z = last.pos.z;
        }
        r.cause = fell ? "Queda" : "";
        p.deaths.push_back(std::move(r));
    }
    history_.clear();
}
