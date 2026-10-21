#include "games.h"
#include "ds1_data.h"

extern const GameInfo kDs2Info;
extern const GameInfo kSekiroInfo;

namespace {

const GameInfo kDsrInfo = {
    "dsr", L"DarkSoulsRemastered.exe", "Dark Souls Remastered", "DSR", nullptr,
    Ds1BonfireName, Ds1AllMapNames, Ds1CauseSuggestions, "Última fogueira", true, true,
};

} // namespace

const std::vector<const GameInfo*>& AllGames()
{
    static const std::vector<const GameInfo*> list = {
        &kDsrInfo,
        &kDs2Info,
        &kSekiroInfo,
    };
    return list;
}

const GameInfo& GameById(const std::string& id)
{
    for (const GameInfo* g : AllGames())
        if (id == g->id) return *g;
    return kDsrInfo;
}

std::string SupportedGamesText()
{
    const auto& games = AllGames();
    std::string t;
    for (size_t i = 0; i < games.size(); ++i) {
        if (i > 0) t += (i + 1 == games.size()) ? " e " : ", ";
        t += games[i]->name;
    }
    return t;
}
