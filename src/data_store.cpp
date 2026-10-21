#include "data_store.h"
#include "util.h"
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {

json ToJson(const Settings& s)
{
    return json{
        { "firstRunDone", s.firstRunDone },
        { "gameExePath", s.gameExePath },
        { "startWithWindows", s.startWithWindows },
        { "closeToTray", s.closeToTray },
        { "hotkeyEnabled", s.hotkeyEnabled },
        { "overlayEnabled", s.overlayEnabled },
        { "overlayOnlyWhenFocused", s.overlayOnlyWhenFocused },
        { "overlayHideInMenu", s.overlayHideInMenu },
        { "overlayShowName", s.overlayShowName },
        { "overlayScaleWithGame", s.overlayScaleWithGame },
        { "overlayAnchor", s.overlayAnchor },
        { "overlayOffsetX", s.overlayOffsetX },
        { "overlayOffsetY", s.overlayOffsetY },
        { "overlayFontSize", s.overlayFontSize },
        { "overlayFont", s.overlayFont },
        { "overlayBold", s.overlayBold },
        { "overlayLabel", s.overlayLabel },
        { "overlayOutline", s.overlayOutline },
        { "colorNormal", s.colorNormal },
        { "colorEdited", s.colorEdited },
        { "colorOutline", s.colorOutline },
    };
}

void FromJson(const json& j, Settings& s)
{
    Settings d;
    s.firstRunDone = j.value("firstRunDone", d.firstRunDone);
    s.gameExePath = j.value("gameExePath", d.gameExePath);
    s.startWithWindows = j.value("startWithWindows", d.startWithWindows);
    s.closeToTray = j.value("closeToTray", d.closeToTray);
    s.hotkeyEnabled = j.value("hotkeyEnabled", d.hotkeyEnabled);
    s.overlayEnabled = j.value("overlayEnabled", d.overlayEnabled);
    s.overlayOnlyWhenFocused = j.value("overlayOnlyWhenFocused", d.overlayOnlyWhenFocused);
    s.overlayHideInMenu = j.value("overlayHideInMenu", d.overlayHideInMenu);
    s.overlayShowName = j.value("overlayShowName", d.overlayShowName);
    s.overlayScaleWithGame = j.value("overlayScaleWithGame", d.overlayScaleWithGame);
    s.overlayAnchor = j.value("overlayAnchor", d.overlayAnchor);
    s.overlayOffsetX = j.value("overlayOffsetX", d.overlayOffsetX);
    s.overlayOffsetY = j.value("overlayOffsetY", d.overlayOffsetY);
    s.overlayFontSize = j.value("overlayFontSize", d.overlayFontSize);
    s.overlayFont = j.value("overlayFont", d.overlayFont);
    s.overlayBold = j.value("overlayBold", d.overlayBold);
    s.overlayLabel = j.value("overlayLabel", d.overlayLabel);
    s.overlayOutline = j.value("overlayOutline", d.overlayOutline);
    s.colorNormal = j.value("colorNormal", d.colorNormal);
    s.colorEdited = j.value("colorEdited", d.colorEdited);
    s.colorOutline = j.value("colorOutline", d.colorOutline);
}

json ToJson(const DeathRecord& r)
{
    json j{
        { "id", r.id }, { "gameCount", r.gameCount }, { "time", r.time }, { "playTimeMs", r.playTimeMs },
        { "mapArea", r.mapArea }, { "mapBlock", r.mapBlock }, { "area", r.area }, { "bonfire", r.bonfire },
        { "cause", r.cause }, { "causeEdited", r.causeEdited }, { "untracked", r.untracked },
    };
    if (r.hasPos) j["pos"] = { r.x, r.y, r.z };
    return j;
}

DeathRecord RecordFromJson(const json& j)
{
    DeathRecord r;
    r.id = j.value("id", (int64_t)0);
    r.gameCount = j.value("gameCount", 0);
    r.time = j.value("time", std::string());
    r.playTimeMs = j.value("playTimeMs", 0u);
    r.mapArea = j.value("mapArea", -1);
    r.mapBlock = j.value("mapBlock", -1);
    r.area = j.value("area", std::string());
    r.bonfire = j.value("bonfire", 0);
    r.cause = j.value("cause", std::string());
    r.causeEdited = j.value("causeEdited", false);
    r.untracked = j.value("untracked", false);
    if (j.contains("pos") && j["pos"].is_array() && j["pos"].size() == 3) {
        r.hasPos = true;
        r.x = j["pos"][0].get<float>();
        r.y = j["pos"][1].get<float>();
        r.z = j["pos"][2].get<float>();
    }
    return r;
}

json ToJson(const Profile& p)
{
    json deaths = json::array();
    for (const auto& r : p.deaths) deaths.push_back(ToJson(r));
    return json{
        { "game", p.game }, { "key", p.key }, { "baseKey", p.baseKey }, { "name", p.name }, { "slot", p.slot },
        { "gameDeaths", p.gameDeaths }, { "baseDeaths", p.baseDeaths }, { "offset", p.offset },
        { "edited", p.edited }, { "playTimeMs", p.playTimeMs }, { "soulLevel", p.soulLevel },
        { "ngPlus", p.ngPlus }, { "firstSeen", p.firstSeen }, { "lastSeen", p.lastSeen },
        { "deaths", deaths },
    };
}

Profile ProfileFromJson(const json& j)
{
    Profile p;
    p.game = j.value("game", std::string("dsr"));   // 1.0.0 files only had Dark Souls Remastered
    p.key = j.value("key", std::string());
    p.baseKey = j.value("baseKey", p.key);
    p.name = j.value("name", std::string());
    p.slot = j.value("slot", -1);
    p.gameDeaths = j.value("gameDeaths", 0);
    p.baseDeaths = j.value("baseDeaths", 0);
    p.offset = j.value("offset", 0);
    p.edited = j.value("edited", false);
    p.playTimeMs = j.value("playTimeMs", 0u);
    p.soulLevel = j.value("soulLevel", 0);
    p.ngPlus = j.value("ngPlus", 0);
    p.firstSeen = j.value("firstSeen", std::string());
    p.lastSeen = j.value("lastSeen", std::string());
    if (j.contains("deaths") && j["deaths"].is_array())
        for (const auto& d : j["deaths"]) p.deaths.push_back(RecordFromJson(d));
    return p;
}

} // namespace

std::wstring DataStore::FilePath() const
{
    return GetAppDataDir() + L"\\data.json";
}

bool DataStore::Load()
{
    std::string text;
    if (!ReadWholeFile(FilePath(), text)) return false;   // first run
    try {
        json j = json::parse(text);
        if (j.contains("settings")) FromJson(j["settings"], settings);
        profiles.clear();
        if (j.contains("profiles") && j["profiles"].is_array())
            for (const auto& p : j["profiles"]) profiles.push_back(ProfileFromJson(p));
        for (const auto& p : profiles)
            for (const auto& r : p.deaths) lastId_ = std::max(lastId_, r.id);
        return true;
    } catch (...) {
        // Keep the unreadable file instead of overwriting it.
        std::wstring backup = FilePath() + L".corrompido-" + Utf8ToWide(std::to_string(GetTickCount64()));
        MoveFileExW(FilePath().c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING);
        return false;
    }
}

bool DataStore::Save()
{
    json profs = json::array();
    for (const auto& p : profiles) profs.push_back(ToJson(p));
    json j{ { "format", 1 }, { "settings", ToJson(settings) }, { "profiles", profs } };
    bool ok = WriteFileAtomic(FilePath(), j.dump(1, '\t', false, json::error_handler_t::replace));
    if (ok) dirty_ = false;
    return ok;
}

Profile* DataStore::Find(const std::string& key)
{
    for (auto& p : profiles)
        if (p.key == key) return &p;
    return nullptr;
}

int64_t DataStore::NextRecordId()
{
    return ++lastId_;
}
