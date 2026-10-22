#define IMGUI_DEFINE_MATH_OPERATORS
#include "app.h"
#include "ds1_data.h"
#include "games/games.h"
#include "platform.h"
#include "util.h"
#include "version.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <map>
#include <numeric>

namespace {

const ImVec4 kAccent(1.00f, 0.62f, 0.25f, 1.0f);
const ImVec4 kGreen(0.45f, 0.85f, 0.45f, 1.0f);
const ImVec4 kYellow(0.95f, 0.80f, 0.35f, 1.0f);
const ImVec4 kRed(0.95f, 0.40f, 0.35f, 1.0f);

ImVec4 Argb(uint32_t c)
{
    return ImVec4(((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, ((c >> 24) & 255) / 255.0f);
}

uint32_t ToArgb(const ImVec4& v)
{
    auto b = [](float f) { return (uint32_t)std::lround(std::clamp(f, 0.0f, 1.0f) * 255.0f); };
    return b(v.w) << 24 | b(v.x) << 16 | b(v.y) << 8 | b(v.z);
}

std::string Lower(std::string s)
{
    for (auto& ch : s) ch = (char)tolower((unsigned char)ch);
    return s;
}

bool Contains(const std::string& haystack, const std::string& lowerNeedle)
{
    return lowerNeedle.empty() || Lower(haystack).find(lowerNeedle) != std::string::npos;
}

void StatusDot(const ImVec4& col)
{
    float h = ImGui::GetTextLineHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + h * 0.35f, p.y + h * 0.5f), h * 0.28f, ImGui::GetColorU32(col));
    ImGui::Dummy(ImVec2(h * 0.8f, h));
    ImGui::SameLine();
}

void Badge(const char* text, const ImVec4& bg)
{
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImVec2 pad(6, 2);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p + ts + pad * 2, ImGui::GetColorU32(bg), 4.0f);
    dl->AddText(p + pad, IM_COL32(20, 14, 10, 255), text);
    ImGui::Dummy(ts + pad * 2);
}

std::string SlotText(int slot)
{
    return slot >= 0 ? std::to_string(slot + 1) : std::string("?");
}

std::string CsvField(const std::string& s)
{
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += '"';
        out += c;
    }
    return out + "\"";
}

} // namespace

void App::DrawUI()
{
    Profile* active = tracker_.active();
    std::string activeKey = active ? active->key : std::string();
    if (activeKey != lastActiveKey_) {
        if (!activeKey.empty()) selectedKey_ = activeKey;   // follow the character being played
        lastActiveKey_ = activeKey;
    }

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    UiHeader();

    bool settingsOpen = false;
    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Personagens")) { UiProfilesTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Configurações")) { settingsOpen = true; UiSettingsTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Diagnóstico")) { UiDiagnosticsTab(); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Sobre")) { UiAboutTab(); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    if (!settingsOpen) overlayPreview_ = false;

    UiPopups();
    UiWelcome();
    ImGui::End();
}

void App::UiHeader()
{
    const Settings& s = store_.settings;
    Profile* active = tracker_.active();
    ImVec2 start = ImGui::GetCursorPos();

    ImGui::PushFont(fontTitle_);
    ImGui::TextColored(kAccent, "DS DEATH COUNTER");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    if (reader_.state() == GameReader::State::NotRunning) {
        std::string tags;
        for (const GameInfo* g : AllGames()) tags += (tags.empty() ? "" : "  ·  ") + std::string(g->shortName);
        ImGui::TextDisabled("%s", tags.c_str());
    } else {
        ImGui::TextDisabled("%s", reader_.game().name);
    }

    switch (reader_.state()) {
        case GameReader::State::NotRunning:
            StatusDot(ImVec4(0.55f, 0.52f, 0.48f, 1));
            ImGui::TextDisabled("Aguardando o jogo abrir...");
            ImGui::SameLine();
            if (ImGui::SmallButton("Iniciar Dark Souls")) LaunchGame(Utf8ToWide(s.gameExePath));
            break;
        case GameReader::State::AccessDenied:
            StatusDot(kRed);
            ImGui::TextColored(kRed, "O jogo está rodando como administrador: abra o DS Death Counter como administrador também.");
            break;
        case GameReader::State::Unsupported:
            StatusDot(kRed);
            ImGui::TextColored(kRed, "Versão do jogo não suportada (%s): o contador lê só as versões de 64 bits.",
                               reader_.UnsupportedReason().c_str());
            break;
        case GameReader::State::Scanning:
            StatusDot(kYellow);
            ImGui::TextColored(kYellow, "%s encontrado, preparando a leitura...", reader_.game().name);
            break;
        case GameReader::State::Ready:
            StatusDot(kGreen);
            if (active)
                ImGui::Text("Contando as mortes de %s (slot %s)", active->name.c_str(), SlotText(active->slot).c_str());
            else
                ImGui::Text("Conectado ao jogo (versão %s). Carregue um personagem.", reader_.VersionName().c_str());
            break;
    }
    if (GetTickCount64() < statusMessageUntil_) {
        ImGui::SameLine();
        ImGui::TextColored(kAccent, "   %s", statusMessage_.c_str());
    }
    ImVec2 end = ImGui::GetCursorPos();

    // Current character's count on the right.
    if (active) {
        std::string num = std::to_string(active->Shown());
        ImGui::PushFont(fontMid_);
        float w = ImGui::CalcTextSize(num.c_str()).x;
        ImGui::PopFont();
        float labelW = ImGui::CalcTextSize("mortes").x;
        float x = ImGui::GetWindowContentRegionMax().x - w - labelW - ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPos(ImVec2(x, start.y));
        ImGui::PushFont(fontMid_);
        ImVec4 col = Argb(active->edited ? s.colorEdited : s.colorNormal);
        col.w = 1.0f;
        ImGui::TextColored(col, "%s", num.c_str());
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::SetCursorPosY(start.y + fontMid_->FontSize - ImGui::GetTextLineHeight());
        ImGui::TextDisabled("mortes");
        ImGui::SetCursorPos(end);
    }
    ImGui::Spacing();
}

// ---------------------------------------------------------------------------------------------
// Characters

void App::UiProfilesTab()
{
    if (store_.profiles.empty()) {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x * 0.8f);
        ImGui::TextUnformatted("Nenhum personagem ainda.");
        ImGui::TextDisabled("Abra o jogo (%s) e carregue um save: o personagem aparece aqui sozinho "
                            "e cada save ganha a sua própria contagem.", SupportedGamesText().c_str());
        ImGui::PopTextWrapPos();
        return;
    }

    Profile* active = tracker_.active();
    std::vector<int> order(store_.profiles.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const Profile& pa = store_.profiles[a];
        const Profile& pb = store_.profiles[b];
        bool aa = active && pa.key == active->key, ab = active && pb.key == active->key;
        if (aa != ab) return aa;
        return pa.lastSeen > pb.lastSeen;
    });
    if (!store_.Find(selectedKey_)) selectedKey_ = store_.profiles[order[0]].key;

    const Settings& s = store_.settings;
    ImGui::BeginChild("##list", ImVec2(280 * dpiScale_, 0), ImGuiChildFlags_Borders);
    ImGui::TextDisabled("SAVES / PERSONAGENS");
    for (int idx : order) {
        Profile& p = store_.profiles[idx];
        ImGui::PushID(p.key.c_str());
        float line = ImGui::GetTextLineHeight();
        float pad = 8.0f * dpiScale_;
        float rowH = line * 2 + pad * 1.5f;
        if (ImGui::Selectable("##row", p.key == selectedKey_, 0, ImVec2(0, rowH))) selectedKey_ = p.key;
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        std::string count = std::to_string(p.Shown());
        ImVec2 cs = fontTitle_->CalcTextSizeA(fontTitle_->FontSize, FLT_MAX, 0.0f, count.c_str());
        ImVec4 cc = Argb(p.edited ? s.colorEdited : s.colorNormal);
        cc.w = 1.0f;
        dl->AddText(fontTitle_, fontTitle_->FontSize, ImVec2(b.x - cs.x - pad, a.y + (rowH - cs.y) * 0.5f),
                    ImGui::GetColorU32(cc), count.c_str());

        float textMaxX = b.x - cs.x - pad * 2;
        ImVec4 clip(a.x, a.y, textMaxX, b.y);
        std::string name = p.name.empty() ? "(sem nome)" : p.name;
        dl->AddText(fontBold_, fontBold_->FontSize, ImVec2(a.x + pad, a.y + pad * 0.5f),
                    ImGui::GetColorU32(ImGuiCol_Text), name.c_str(), nullptr, 0.0f, &clip);
        const GameInfo& pg = GameById(p.game);
        std::string subText = std::string(pg.shortName) + (p.slot >= 0 ? "  ·  Slot " + SlotText(p.slot) : std::string());
        if (pg.hasLevel) subText += "  ·  Nível " + std::to_string(p.soulLevel);
        if (p.ngPlus > 0) subText += "  ·  NG+" + std::to_string(p.ngPlus);
        const char* sub = subText.c_str();
        bool isActive = active && active->key == p.key;
        float subX = a.x + pad;
        if (isActive) {
            dl->AddCircleFilled(ImVec2(subX + line * 0.3f, a.y + pad * 0.5f + line * 1.5f), line * 0.22f, ImGui::GetColorU32(kGreen));
            subX += line * 0.8f;
        }
        dl->AddText(fontUi_, fontUi_->FontSize, ImVec2(subX, a.y + pad * 0.5f + line),
                    ImGui::GetColorU32(ImGuiCol_TextDisabled), sub, nullptr, 0.0f, &clip);
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##detail", ImVec2(0, 0), ImGuiChildFlags_None);
    if (Profile* sel = store_.Find(selectedKey_)) UiProfileDetail(*sel);
    ImGui::EndChild();
}

void App::UiProfileDetail(Profile& p)
{
    const Settings& s = store_.settings;
    Profile* active = tracker_.active();
    bool isActive = active && active->key == p.key;

    ImGui::PushFont(fontTitle_);
    ImGui::TextUnformatted(p.name.empty() ? "(sem nome)" : p.name.c_str());
    ImGui::PopFont();
    if (isActive) {
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(kGreen, "\xE2\x97\x8F no jogo agora");
    }
    std::string cycle = p.ngPlus > 0 ? "NG+" + std::to_string(p.ngPlus) : std::string("NG");
    std::string level = GameById(p.game).hasLevel ? "  ·  Nível " + std::to_string(p.soulLevel) : std::string();
    std::string slot = p.slot >= 0 ? "  ·  Slot " + SlotText(p.slot) : std::string();
    ImGui::TextDisabled("%s%s%s  ·  %s  ·  Tempo de jogo %s", GameById(p.game).name, slot.c_str(),
                        level.c_str(), cycle.c_str(), FormatPlayTime(p.playTimeMs).c_str());
    ImGui::TextDisabled("Acompanhado desde %s  ·  visto por último em %s", p.firstSeen.c_str(), p.lastSeen.c_str());

    ImVec4 col = Argb(p.edited ? s.colorEdited : s.colorNormal);
    col.w = 1.0f;
    ImGui::PushFont(fontBig_);
    ImGui::TextColored(col, "%d", p.Shown());
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, fontBig_->FontSize * 0.15f));
    ImGui::PushFont(fontMid_);
    ImGui::TextUnformatted("mortes");
    ImGui::PopFont();
    if (p.edited) {
        Badge("CONTAGEM EDITADA", Argb(s.colorEdited | 0xFF000000));
        ImGui::TextDisabled("Jogo: %d  ·  ajuste manual: %+d", p.gameDeaths, p.offset);
    } else {
        ImGui::TextDisabled(GameById(p.game).countsItself ? "Contada pelo app (o jogo não tem contador de mortes)"
                                                          : "Contagem lida do próprio jogo");
    }
    ImGui::EndGroup();

    if (ImGui::Button("Editar contagem...")) {
        openEditCount_ = true;
        editCountValue_ = p.Shown();
    }
    if (p.edited) {
        ImGui::SameLine();
        if (ImGui::Button("Voltar à contagem do jogo")) {
            p.offset = 0;
            p.edited = false;
            store_.Save();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Exportar CSV...")) ExportCsv(p);
    ImGui::SameLine();
    if (ImGui::Button("Excluir personagem...")) openDeleteProfile_ = true;

    if (p.baseDeaths > 0) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextDisabled("%d morte(s) aconteceram antes do contador conhecer este personagem: entram no total, mas sem detalhes na lista.",
                            p.baseDeaths);
        ImGui::PopTextWrapPos();
    }

    ImGui::Spacing();
    if (ImGui::BeginTabBar("detailTabs")) {
        char label[64];
        snprintf(label, sizeof(label), "Mortes (%d)###deaths", (int)p.deaths.size());
        if (ImGui::BeginTabItem(label)) { UiDeathTable(p); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Por área")) { UiStatsTable(p, false); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Por causa")) { UiStatsTable(p, true); ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
}

void App::UiDeathTable(Profile& p)
{
    ImGui::SetNextItemWidth(300 * dpiScale_);
    ImGui::InputTextWithHint("##filter", "Filtrar por área, ponto de retorno ou causa...", &deathFilter_);
    ImGui::SameLine();
    ImGui::TextDisabled("Clique na causa para editar  ·  botão direito na linha para mais opções");

    const GameInfo& game = GameById(p.game);
    std::string needle = Lower(deathFilter_);
    std::vector<int> rows;
    for (int i = (int)p.deaths.size() - 1; i >= 0; --i) {
        const DeathRecord& r = p.deaths[i];
        if (Contains(r.area, needle) || Contains(r.cause, needle) || Contains(game.bonfireName(r.bonfire), needle))
            rows.push_back(i);
    }
    if (p.deaths.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("Nenhuma morte registrada ainda. Que a chama te proteja.");
        return;
    }

    int64_t deleteId = 0;
    ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterH |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("deaths", 6, flags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 56 * dpiScale_);
        ImGui::TableSetupColumn("Data / hora", ImGuiTableColumnFlags_WidthFixed, 150 * dpiScale_);
        ImGui::TableSetupColumn("Área", ImGuiTableColumnFlags_WidthStretch, 2.2f);
        ImGui::TableSetupColumn(game.checkpointLabel, ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("Causa", ImGuiTableColumnFlags_WidthStretch, 1.6f);
        ImGui::TableSetupColumn("Tempo de jogo", ImGuiTableColumnFlags_WidthFixed, 100 * dpiScale_);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin((int)rows.size());
        while (clipper.Step()) {
            for (int ri = clipper.DisplayStart; ri < clipper.DisplayEnd; ++ri) {
                DeathRecord& r = p.deaths[rows[ri]];
                ImGui::PushID((int)r.id);
                ImGui::TableNextRow();
                if (r.untracked) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

                ImGui::TableSetColumnIndex(0);
                std::string num = r.gameCount > 0 ? std::to_string(r.gameCount) : "-";
                ImGui::Selectable(num.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                if (ImGui::BeginPopupContextItem("rowMenu")) {
                    if (ImGui::MenuItem("Editar causa")) {
                        editRecordId_ = r.id; editingArea_ = false; editBuffer_ = r.cause; openEditRecord_ = true;
                    }
                    if (ImGui::MenuItem("Editar área")) {
                        editRecordId_ = r.id; editingArea_ = true; editBuffer_ = r.area; openEditRecord_ = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Excluir este registro")) deleteId = r.id;
                    ImGui::EndPopup();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(r.time.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(r.area.c_str());
                if (r.hasPos)
                    ImGui::SetItemTooltip("Mapa m%02d_%02d  ·  posição %.1f, %.1f, %.1f", r.mapArea, r.mapBlock, r.x, r.y, r.z);

                ImGui::TableSetColumnIndex(3);
                std::string bonfire = game.bonfireName(r.bonfire);
                if (bonfire.empty() && r.bonfire > 0) bonfire = "#" + std::to_string(r.bonfire);
                ImGui::TextUnformatted(bonfire.empty() ? "-" : bonfire.c_str());

                ImGui::TableSetColumnIndex(4);
                bool emptyCause = r.cause.empty();
                if (emptyCause) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                std::string causeLabel = (emptyCause ? std::string("definir...") : r.cause) + "##cause";
                if (ImGui::Selectable(causeLabel.c_str())) {
                    editRecordId_ = r.id; editingArea_ = false; editBuffer_ = r.cause; openEditRecord_ = true;
                }
                if (emptyCause) ImGui::PopStyleColor();
                if (!r.untracked && !r.cause.empty() && !r.causeEdited)
                    ImGui::SetItemTooltip("Detectado automaticamente (clique para corrigir)");

                ImGui::TableSetColumnIndex(5);
                ImGui::TextUnformatted(r.untracked ? "-" : FormatPlayTime(r.playTimeMs).c_str());

                if (r.untracked) ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    if (deleteId) {
        p.deaths.erase(std::remove_if(p.deaths.begin(), p.deaths.end(), [&](const DeathRecord& r) { return r.id == deleteId; }),
                       p.deaths.end());
        store_.Save();
    }
}

void App::UiStatsTable(Profile& p, bool byCause)
{
    std::map<std::string, int> counts;
    int total = 0;
    for (const auto& r : p.deaths) {
        if (r.untracked) continue;
        std::string key = byCause ? (r.cause.empty() ? "(causa não definida)" : r.cause)
                                  : (r.area.empty() ? "Desconhecida" : r.area);
        ++counts[key];
        ++total;
    }
    if (total == 0) {
        ImGui::Spacing();
        ImGui::TextDisabled("Ainda não há mortes registradas com detalhes.");
        return;
    }
    std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    if (ImGui::BeginTable("stats", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
                                      ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(byCause ? "Causa" : "Área", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Mortes", ImGuiTableColumnFlags_WidthFixed, 70 * dpiScale_);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableHeadersRow();
        for (const auto& [name, n] : sorted) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", n);
            ImGui::TableSetColumnIndex(2);
            char pct[16];
            snprintf(pct, sizeof(pct), "%.0f%%", 100.0f * n / total);
            ImGui::ProgressBar((float)n / total, ImVec2(-FLT_MIN, 0), pct);
        }
        ImGui::EndTable();
    }
}

// ---------------------------------------------------------------------------------------------
// Popups

void App::UiPopups()
{
    const float wrap = 460 * dpiScale_;

    if (openEditCount_) { ImGui::OpenPopup("Editar contagem"); openEditCount_ = false; }
    ImGui::SetNextWindowSize(ImVec2(wrap + 40 * dpiScale_, 0));
    if (ImGui::BeginPopupModal("Editar contagem", nullptr, ImGuiWindowFlags_NoResize)) {
        Profile* p = store_.Find(selectedKey_);
        if (!p) {
            ImGui::CloseCurrentPopup();
        } else {
            ImGui::Text("Personagem: %s", p->name.c_str());
            ImGui::Text("Contagem lida do jogo: %d", p->gameDeaths);
            ImGui::SetNextItemWidth(180 * dpiScale_);
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::InputInt("Nova contagem", &editCountValue_);
            editCountValue_ = std::max(0, editCountValue_);
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextDisabled("O jogo continua sendo lido normalmente: as próximas mortes somam a partir do número "
                                "que você definir. Enquanto a contagem estiver editada, o app mostra o aviso e o "
                                "número do overlay usa a cor de \"contagem editada\".");
            ImGui::PopTextWrapPos();
            if (ImGui::Button("Salvar", ImVec2(120 * dpiScale_, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                p->offset = editCountValue_ - p->gameDeaths;
                p->edited = p->offset != 0;
                store_.Save();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120 * dpiScale_, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openDeleteProfile_) { ImGui::OpenPopup("Excluir personagem"); openDeleteProfile_ = false; }
    ImGui::SetNextWindowSize(ImVec2(wrap + 40 * dpiScale_, 0));
    if (ImGui::BeginPopupModal("Excluir personagem", nullptr, ImGuiWindowFlags_NoResize)) {
        Profile* p = store_.Find(selectedKey_);
        if (!p) {
            ImGui::CloseCurrentPopup();
        } else {
            ImGui::PushTextWrapPos(0.0f);
            ImGui::Text("Excluir \"%s\" e todo o histórico de mortes dele?", p->name.c_str());
            ImGui::TextDisabled("Isto só apaga os dados deste app (o save do jogo não é tocado). Se você carregar esse "
                                "personagem de novo, ele volta para a lista, sem o histórico.");
            ImGui::PopTextWrapPos();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.12f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.20f, 0.15f, 1));
            bool del = ImGui::Button("Excluir", ImVec2(120 * dpiScale_, 0));
            ImGui::PopStyleColor(2);
            if (del) {
                std::string key = p->key;
                store_.profiles.erase(std::remove_if(store_.profiles.begin(), store_.profiles.end(),
                                                     [&](const Profile& q) { return q.key == key; }),
                                      store_.profiles.end());
                selectedKey_.clear();
                store_.Save();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120 * dpiScale_, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (openEditRecord_) { ImGui::OpenPopup("Editar registro"); openEditRecord_ = false; }
    ImGui::SetNextWindowSize(ImVec2(wrap + 40 * dpiScale_, 0));
    if (ImGui::BeginPopupModal("Editar registro", nullptr, ImGuiWindowFlags_NoResize)) {
        Profile* p = store_.Find(selectedKey_);
        DeathRecord* r = nullptr;
        if (p)
            for (auto& d : p->deaths)
                if (d.id == editRecordId_) r = &d;
        if (!r) {
            ImGui::CloseCurrentPopup();
        } else {
            if (r->gameCount > 0) ImGui::Text(editingArea_ ? "Área da morte #%d" : "Causa da morte #%d", r->gameCount);
            else ImGui::TextUnformatted(editingArea_ ? "Área da morte" : "Causa da morte");
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth(-FLT_MIN);
            bool enter = ImGui::InputTextWithHint("##edit", editingArea_ ? "Onde foi?" : "O que te matou?", &editBuffer_,
                                                  ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::TextDisabled("Sugestões:");
            ImGui::BeginChild("##sugg", ImVec2(0, 220 * dpiScale_), ImGuiChildFlags_Borders);
            const GameInfo& game = GameById(p->game);
            const auto& list = editingArea_ ? game.allMapNames() : game.causeSuggestions();
            bool exact = std::find(list.begin(), list.end(), editBuffer_) != list.end();
            std::string needle = exact ? std::string() : Lower(editBuffer_);
            for (const auto& sug : list) {
                if (!Contains(sug, needle)) continue;
                if (ImGui::Selectable(sug.c_str(), sug == editBuffer_)) editBuffer_ = sug;
            }
            ImGui::EndChild();
            if (ImGui::Button("Salvar", ImVec2(120 * dpiScale_, 0)) || enter) {
                if (editingArea_) {
                    r->area = editBuffer_;
                } else {
                    r->cause = editBuffer_;
                    r->causeEdited = true;
                }
                store_.Save();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancelar", ImVec2(120 * dpiScale_, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void App::UiWelcome()
{
    Settings& s = store_.settings;
    if (s.firstRunDone) return;
    static bool triedDetect = false;
    if (!triedDetect) {
        triedDetect = true;
        if (s.gameExePath.empty()) s.gameExePath = WideToUtf8(FindSteamGameExe());
    }
    if (!ImGui::IsPopupOpen("Bem-vindo")) ImGui::OpenPopup("Bem-vindo");
    ImGui::SetNextWindowSize(ImVec2(640 * dpiScale_, 0));
    if (ImGui::BeginPopupModal("Bem-vindo", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::PushFont(fontTitle_);
        ImGui::TextColored(kAccent, "Bem-vindo ao DS Death Counter");
        ImGui::PopFont();
        ImGui::Text("Ele conta suas mortes no %s (uma contagem para cada save/personagem) "
                    "e mostra o número por cima do jogo. Só lê a memória do jogo: não modifica nada.", SupportedGamesText().c_str());

        ImGui::SeparatorText("1. Vincular ao jogo");
        if (!s.gameExePath.empty()) {
            ImGui::TextColored(kGreen, "Jogo encontrado:");
            ImGui::TextDisabled("%s", s.gameExePath.c_str());
        } else {
            ImGui::TextColored(kYellow, "Não encontrei a instalação automaticamente.");
        }
        if (ImGui::Button("Procurar DarkSoulsRemastered.exe...")) {
            std::wstring f = BrowseForExe(hwnd_, Utf8ToWide(s.gameExePath));
            if (!f.empty()) s.gameExePath = WideToUtf8(f);
        }
        ImGui::TextDisabled("O caminho só é usado pelo botão \"Iniciar Dark Souls\": o contador reconhece o jogo "
                            "sozinho sempre que ele estiver aberto.");

        ImGui::SeparatorText("2. Sempre ligado");
        ImGui::Checkbox("Iniciar com o Windows, minimizado na bandeja (recomendado)", &s.startWithWindows);
        ImGui::TextDisabled("Assim, toda vez que você abrir um dos jogos as mortes já estarão sendo contadas, "
                            "sem precisar lembrar de abrir este app.");

        ImGui::SeparatorText("3. Overlay");
        ImGui::TextUnformatted("O número aparece num canto da tela do jogo. Use o jogo em modo Janela ou Janela sem "
                               "bordas, que é onde ele sempre funciona (em tela cheia ele pode não aparecer). "
                               "Ctrl+Shift+D mostra/oculta o overlay a qualquer momento.");
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (ImGui::Button("Começar", ImVec2(160 * dpiScale_, 0))) {
            s.firstRunDone = true;
            SetAutostart(s.startWithWindows);
            store_.Save();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------------------------------
// Settings / diagnostics / about

void App::UiSettingsTab()
{
    Settings& s = store_.settings;
    bool changed = false;
    ImGui::BeginChild("##settings", ImVec2(0, 0));
    ImGui::PushItemWidth(320 * dpiScale_);
    ImGui::PushTextWrapPos(760 * dpiScale_);

    ImGui::SeparatorText("Vínculo com o jogo");
    ImGui::TextDisabled("O contador encontra o jogo (%s) sozinho sempre que ele estiver aberto.", SupportedGamesText().c_str());
    std::string path = s.gameExePath.empty() ? std::string("(não definido)") : s.gameExePath;
    ImGui::InputText("Executável do jogo", &path, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Procurar...")) {
        std::wstring f = BrowseForExe(hwnd_, Utf8ToWide(s.gameExePath));
        if (!f.empty()) { s.gameExePath = WideToUtf8(f); changed = true; }
    }
    ImGui::SameLine();
    if (ImGui::Button("Detectar na Steam")) {
        std::wstring f = FindSteamGameExe();
        if (!f.empty()) { s.gameExePath = WideToUtf8(f); changed = true; }
        statusMessage_ = f.empty() ? "Instalação da Steam não encontrada" : "Jogo encontrado na Steam";
        statusMessageUntil_ = GetTickCount64() + 4000;
    }
    ImGui::SameLine();
    if (ImGui::Button("Iniciar Dark Souls")) LaunchGame(Utf8ToWide(s.gameExePath));

    if (ImGui::Checkbox("Iniciar com o Windows (minimizado na bandeja)", &s.startWithWindows)) {
        SetAutostart(s.startWithWindows);
        changed = true;
    }
    changed |= ImGui::Checkbox("Fechar a janela mantém o contador rodando na bandeja", &s.closeToTray);
    if (ImGui::Checkbox("Atalho Ctrl+Shift+D mostra/oculta o overlay", &s.hotkeyEnabled)) {
        RegisterOverlayHotkey();
        changed = true;
    }

    ImGui::SeparatorText("Overlay");
    changed |= ImGui::Checkbox("Mostrar a contagem por cima do jogo", &s.overlayEnabled);
    changed |= ImGui::Checkbox("Só quando o jogo estiver em primeiro plano", &s.overlayOnlyWhenFocused);
    changed |= ImGui::Checkbox("Esconder no menu principal do jogo", &s.overlayHideInMenu);
    changed |= ImGui::Checkbox("Mostrar o nome do personagem", &s.overlayShowName);
    changed |= ImGui::InputText("Texto antes do número", &s.overlayLabel);

    static const char* fonts[] = { "Georgia", "Times New Roman", "Palatino Linotype", "Book Antiqua", "Garamond",
                                   "Segoe UI", "Trebuchet MS", "Arial Black", "Impact", "Consolas" };
    if (ImGui::BeginCombo("Fonte", s.overlayFont.c_str())) {
        for (const char* f : fonts)
            if (ImGui::Selectable(f, s.overlayFont == f)) { s.overlayFont = f; changed = true; }
        ImGui::EndCombo();
    }
    changed |= ImGui::InputText("Outra fonte instalada", &s.overlayFont);
    changed |= ImGui::Checkbox("Negrito", &s.overlayBold);
    changed |= ImGui::SliderFloat("Tamanho", &s.overlayFontSize, 12.0f, 140.0f, "%.0f px");
    changed |= ImGui::Checkbox("Ajustar à resolução do jogo (tamanhos pensados para 1080p)", &s.overlayScaleWithGame);
    static const char* anchors[] = { "Canto superior esquerdo", "Canto superior direito", "Canto inferior esquerdo",
                                     "Canto inferior direito" };
    changed |= ImGui::Combo("Posição", &s.overlayAnchor, anchors, IM_ARRAYSIZE(anchors));
    changed |= ImGui::SliderInt("Distância horizontal", &s.overlayOffsetX, 0, 1200, "%d px");
    changed |= ImGui::SliderInt("Distância vertical", &s.overlayOffsetY, 0, 1000, "%d px");
    changed |= ImGui::SliderFloat("Contorno", &s.overlayOutline, 0.0f, 10.0f, "%.1f px");

    ImGuiColorEditFlags cf = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf;
    ImVec4 cn = Argb(s.colorNormal), ce = Argb(s.colorEdited), co = Argb(s.colorOutline);
    if (ImGui::ColorEdit4("Cor normal", &cn.x, cf)) { s.colorNormal = ToArgb(cn); changed = true; }
    if (ImGui::ColorEdit4("Cor quando a contagem foi editada", &ce.x, cf)) { s.colorEdited = ToArgb(ce); changed = true; }
    if (ImGui::ColorEdit4("Cor do contorno", &co.x, cf)) { s.colorOutline = ToArgb(co); changed = true; }

    ImGui::Checkbox("Pré-visualizar o overlay agora", &overlayPreview_);
    ImGui::SameLine();
    ImGui::Checkbox("como contagem editada", &previewAsEdited_);
    ImGui::TextDisabled("Com o jogo aberto, a prévia aparece sobre a janela do jogo; sem ele, sobre este monitor.");

    ImGui::SeparatorText("Janela do jogo");
    ImGui::TextDisabled("O overlay é uma janela própria desenhada por cima do jogo. Ele aparece com o jogo em modo "
                        "Janela ou Janela sem bordas; em Tela cheia ele pode não aparecer ou piscar, dependendo do Windows e do driver.");
    ImGui::BeginDisabled(!gameHwnd_);
    if (ImGui::Button("Deixar a janela do jogo sem bordas (tela cheia em janela)")) {
        bool ok = MakeWindowBorderless(gameHwnd_);
        statusMessage_ = ok ? "Janela do jogo ajustada" : "Não foi possível ajustar a janela do jogo";
        statusMessageUntil_ = GetTickCount64() + 4000;
    }
    ImGui::EndDisabled();
    ImGui::TextDisabled("Coloque o jogo em modo Janela, na resolução do seu monitor, e use o botão acima. "
                        "Só mexe na moldura da janela; nada dentro do jogo é alterado.");

    ImGui::SeparatorText("OBS (lives)");
    ImGui::TextDisabled("A Captura de jogo e a Captura de janela do OBS não mostram o overlay. Para pôr a contagem na "
                        "live, o app grava o texto num arquivo, e uma fonte de texto do OBS lê esse arquivo sozinha.");
    changed |= ImGui::Checkbox("Gravar a contagem num arquivo de texto para o OBS", &s.obsTextEnabled);
    changed |= ImGui::InputText("Texto no OBS", &s.obsTextFormat);
    ImGui::TextDisabled("{mortes} vira o número de mortes e {nome}, o nome do personagem.");
    std::wstring obsPath = ObsTextPath();
    std::string obsPathUtf8 = WideToUtf8(obsPath);
    ImGui::SetNextItemWidth(560 * dpiScale_);
    ImGui::InputText("Arquivo", &obsPathUtf8, ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Escolher arquivo...")) {
        std::wstring f = BrowseSaveTxt(hwnd_, obsPath);
        if (f.size() > 4 && _wcsicmp(f.c_str() + f.size() - 4, L".txt") == 0) {
            s.obsTextPath = WideToUtf8(f);
            changed = true;
        } else if (!f.empty()) {
            statusMessage_ = "Escolha um arquivo .txt";
            statusMessageUntil_ = GetTickCount64() + 4000;
        }
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(s.obsTextPath.empty());
    if (ImGui::Button("Usar o padrão")) { s.obsTextPath.clear(); changed = true; }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Abrir pasta")) OpenFolder(obsPath.substr(0, obsPath.find_last_of(L'\\')));
    ImGui::SameLine();
    if (ImGui::Button("Copiar caminho")) ImGui::SetClipboardText(obsPathUtf8.c_str());
    if (!s.obsTextEnabled)
        ImGui::TextDisabled("Prévia: %s", ObsText().c_str());
    else if (obsWriteFailed_)
        ImGui::TextColored(kRed, "Não foi possível gravar o arquivo. Confira se a pasta existe ou escolha outro arquivo.");
    else
        ImGui::TextColored(kGreen, "Gravando: %s", ObsText().c_str());
    ImGui::TextUnformatted("Como usar no OBS:");
    ImGui::TextDisabled("1. Ligue a opção acima.\n"
                        "2. No OBS, em Fontes, clique em + e escolha Texto (GDI+).\n"
                        "3. Marque \"Carregar do arquivo\" e escolha o arquivo acima (ou cole o caminho copiado).\n"
                        "4. Fonte, cor e contorno se ajustam no próprio OBS. O texto se atualiza sozinho a cada morte.");

    ImGui::SeparatorText("Dados");
    if (ImGui::Button("Abrir pasta de dados")) OpenFolder(GetAppDataDir());
    ImGui::SameLine();
    ImGui::TextDisabled("%s", WideToUtf8(store_.FilePath()).c_str());

    ImGui::PopTextWrapPos();
    ImGui::PopItemWidth();
    ImGui::EndChild();
    if (changed) store_.MarkDirty();
}

void App::UiDiagnosticsTab()
{
    std::string t = reader_.DiagnosticsText();
    char b[512];
    if (tracker_.hasSnapshot()) {
        const GameSnapshot& s = tracker_.snapshot();
        snprintf(b, sizeof(b),
                 "\nPersonagem carregado: %s | No mundo: %s\nNome: %s | Slot: %d | Mortes (jogo): %d | Tempo: %s | NG+: %d | Nível: %d\n",
                 s.charLoaded ? "sim" : "não", s.inWorld ? "sim" : "não", WideToUtf8(s.name).c_str(), s.saveSlot,
                 s.deaths, FormatPlayTime(s.playTimeMs).c_str(), s.ngPlus, s.soulLevel);
        t += b;
        std::string map = !s.areaName.empty() ? s.areaName : s.mapArea >= 0 ? Ds1MapName(s.mapArea, s.mapBlock) : "?";
        snprintf(b, sizeof(b), "Mapa: 0x%08X -> m%02d_%02d (%s)\nÚltima fogueira: %d (%s)\nHP: %d | Posição: %s%.2f, %.2f, %.2f\n",
                 s.mapRaw, s.mapArea, s.mapBlock, map.c_str(), s.lastBonfire, GameById(s.game).bonfireName(s.lastBonfire).c_str(),
                 s.hp, s.hasPos ? "" : "(indisponível) ", s.pos.x, s.pos.y, s.pos.z);
        t += b;
        t += "Barras de chefe:";
        for (int i = 0; i < GameSnapshot::kBossSlots; ++i) {
            int id = s.bossNameId[i];
            std::string name = id > 0 ? Ds1BossBarName(id) : std::string();
            snprintf(b, sizeof(b), " [%d] %s%s%s", i, id > 0 ? std::to_string(id).c_str() : "-",
                     name.empty() ? "" : " ", name.c_str());
            t += b;
        }
        t += "\n";
    }
    Profile* p = tracker_.active();
    t += "\nPerfil ativo: " + (p ? p->key : std::string("(nenhum)")) + "\n";
    t += "Arquivo de dados: " + WideToUtf8(store_.FilePath()) + "\n";
    t += std::string("DS Death Counter ") + DSDC_VERSION_STRING + "\n";

    ImGui::TextDisabled("Informações técnicas: úteis se algo não estiver funcionando.");
    ImGui::SameLine();
    if (ImGui::SmallButton("Copiar")) ImGui::SetClipboardText(t.c_str());
    ImGui::InputTextMultiline("##diag", &t, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
}

void App::UiAboutTab()
{
    ImGui::BeginChild("##about", ImVec2(0, 0));
    ImGui::PushTextWrapPos(780 * dpiScale_);
    ImGui::PushFont(fontTitle_);
    ImGui::Text("DS Death Counter %s", DSDC_VERSION_STRING);
    ImGui::PopFont();
    ImGui::TextDisabled("Contador de mortes com overlay para %s. Código aberto (licença MIT).", SupportedGamesText().c_str());
    if (ImGui::Button("Página do projeto no GitHub")) OpenFolder(Utf8ToWide(DSDC_REPO_URL));
    ImGui::SameLine();
    if (ImGui::Button("Baixar versões novas")) OpenFolder(Utf8ToWide(std::string(DSDC_REPO_URL) + "/releases"));

    ImGui::SeparatorText("Feito com IA");
    ImGui::TextUnformatted("Ideia, requisitos e testes no jogo: MatheusMuzzi. Código, pesquisa dos endereços de memória "
                           "e documentação: Claude (Claude Opus 5.5, da Anthropic), usando o Claude Code. A página do "
                           "projeto explica em detalhes como o app foi feito.");

    ImGui::SeparatorText("Como funciona");
    ImGui::BulletText("O app lê o contador de mortes que o próprio jogo guarda para cada personagem. Por isso a "
                      "contagem nunca se perde: fechou o jogo, a contagem para; abriu de novo, continua de onde parou.");
    ImGui::BulletText("Cada save/personagem tem a sua contagem e a sua lista de mortes.");
    ImGui::BulletText("Quando você morre, o app anota a hora, o mapa em que você estava e a última fogueira usada. "
                      "Se a barra de vida de um chefe estava na tela, a causa vira esse chefe; se percebeu uma queda "
                      "grande, vira \"Queda\". As outras causas (inimigos comuns, armadilhas...) você define na lista.");
    ImGui::BulletText("Editar a contagem só altera o número mostrado por este app; o contador do jogo não é tocado.");

    ImGui::SeparatorText("Por que não arrisca banimento");
    ImGui::BulletText("Só LÊ a memória do jogo, a mesma técnica dos autosplitters do LiveSplit e do DSDeaths, "
                      "usados há anos pela comunidade. Não escreve nada, não injeta DLL e não altera arquivos ou saves.");
    ImGui::BulletText("O overlay é uma janela separada por cima do jogo; nada é desenhado dentro dele.");
    ImGui::BulletText("Os banimentos do Dark Souls vêm de dados de personagem impossíveis enviados aos servidores, "
                      "coisa que este app não toca.");

    ImGui::SeparatorText("Dados");
    ImGui::TextDisabled("%s", WideToUtf8(store_.FilePath()).c_str());
    if (ImGui::Button("Abrir pasta de dados")) OpenFolder(GetAppDataDir());
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
}

void App::ExportCsv(const Profile& p)
{
    std::string base = p.name.empty() ? "personagem" : p.name;
    for (auto& ch : base)
        if (strchr("\\/:*?\"<>|", ch)) ch = '_';
    std::wstring file = BrowseSaveCsv(hwnd_, Utf8ToWide("mortes_" + base + ".csv"));
    if (file.empty()) return;

    std::string out = "\xEF\xBB\xBF" "numero;data_hora;area;ultima_fogueira;causa;tempo_de_jogo;x;y;z;registrada\r\n";
    for (const auto& r : p.deaths) {
        char pos[96] = ";;";
        if (r.hasPos) snprintf(pos, sizeof(pos), "%.2f;%.2f;%.2f", r.x, r.y, r.z);
        out += (r.gameCount > 0 ? std::to_string(r.gameCount) : std::string()) + ";" + CsvField(r.time) + ";" +
               CsvField(r.area) + ";" + CsvField(GameById(p.game).bonfireName(r.bonfire)) + ";" + CsvField(r.cause) + ";" +
               (r.untracked ? std::string() : FormatPlayTime(r.playTimeMs)) + ";" + pos + ";" +
               (r.untracked ? "nao" : "sim") + "\r\n";
    }
    bool ok = WriteFileAtomic(file, out);
    statusMessage_ = ok ? "CSV exportado" : "Não foi possível salvar o CSV";
    statusMessageUntil_ = GetTickCount64() + 4000;
}
