//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorconsolewindow.h"
#include "editor/window/imgui/imguisubwindow.h"

// ─── Style constants ──────────────────────────────────────────────────────────
static constexpr ImU32 ROW_SELECTED_BG  = IM_COL32(60,  90,  160, 80);
static constexpr ImU32 ROW_HOVER_BG     = IM_COL32(255, 255, 255, 12);
static constexpr float BORDER_STRIP_W   = 3.0f;   // coloured left-border width

// ─── Construction ─────────────────────────────────────────────────────────────

MEditorConsoleWindow::MEditorConsoleWindow() : MEditorConsoleWindow(700, 300) {}

MEditorConsoleWindow::MEditorConsoleWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Console";
    // BUG FIX: subscription was in the default constructor, so it ran before
    // the delegated constructor finished initialising the base class.
    subscriptionId = MLogger::subscribe(
        std::bind(&MEditorConsoleWindow::onLogReceived, this, std::placeholders::_1));
}

MEditorConsoleWindow::~MEditorConsoleWindow() {
    MLogger::unsubscribe(subscriptionId);
}

// ─── Log ingestion ────────────────────────────────────────────────────────────

void MEditorConsoleWindow::onLogReceived(SString raw) {
    // Expected format: "TAG: message" where TAG is 3 characters
    SLogEntry entry;
    entry.tag  = raw.substring(0, 3);
    entry.text = raw.length() > 5 ? raw.substring(5, raw.length()) : SString("");

    if      (entry.tag == "WRN") entry.level = ELogLevel::Warning;
    else if (entry.tag == "ERR") entry.level = ELogLevel::Error;
    else                          entry.level = ELogLevel::Log;

    ++levelCounts[static_cast<int>(entry.level)];
    entries.push_back(std::move(entry));
}

// ─── onGui ────────────────────────────────────────────────────────────────────

void MEditorConsoleWindow::onGui() {
    drawToolbar();
    ImGui::Separator();
    drawLogList();
}

// ─── Toolbar ──────────────────────────────────────────────────────────────────

void MEditorConsoleWindow::drawToolbar() {
    // ── Clear ──
    if (ImGui::Button("Clear")) {
        entries.clear();
        levelCounts = {};
        selectedIndex = -1;
    }

    ImGui::SameLine(0, 10);

    // ── Auto-scroll toggle ──
    ImGui::PushStyleColor(ImGuiCol_Button,
        autoScroll ? ImVec4(0.2f, 0.5f, 0.9f, 0.6f) : ImVec4(0, 0, 0, 0));
    if (ImGui::Button(autoScroll ? "  Scroll \xe2\x86\x93  " : "  Scroll \xe2\x86\x91  "))
        autoScroll = !autoScroll;
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(autoScroll ? "Auto-scroll on" : "Auto-scroll off");

    ImGui::SameLine(0, 10);

    // ── Copy selected ──
    const bool hasSelection = (selectedIndex >= 0 &&
                               selectedIndex < (int)entries.size());
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Copy")) {
        const auto& e = entries[selectedIndex];
        SString full  = STR("[") + e.tag + STR("] ") + e.text;
        ImGui::SetClipboardText(full.c_str());
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip(hasSelection ? "Copy to clipboard" : "Select a row first");

    ImGui::SameLine(0, 16);

    // ── Level filter buttons ── [All N] [LOG N] [WRN N] [ERR N]
    const int total = (int)entries.size();

    struct FilterBtn { const char* label; int idx; ImVec4 col; };
    const FilterBtn btns[] = {
        { "All",  -1, ImVec4(0.75f, 0.75f, 0.75f, 1.0f) },
        { "LOG",   0, levelColor(ELogLevel::Log)          },
        { "WRN",   1, levelColor(ELogLevel::Warning)      },
        { "ERR",   2, levelColor(ELogLevel::Error)        },
    };

    for (const auto& b : btns) {
        const bool active = (activeFilter == b.idx);
        ImGui::PushStyleColor(ImGuiCol_Button,
            active ? ImVec4(b.col.x, b.col.y, b.col.z, 0.35f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? b.col : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border,
            active ? ImVec4(b.col.x, b.col.y, b.col.z, 0.6f) : ImVec4(0, 0, 0, 0));

        const int count = (b.idx < 0) ? total : levelCounts[b.idx];
        const SString label = STR(b.label) + STR(" ") + SString::fromInt(count);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, active ? 1.0f : 0.0f);
        if (ImGui::Button(label.c_str()))
            activeFilter = (activeFilter == b.idx) ? -1 : b.idx;  // toggle off if re-clicked
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 4);
    }

    ImGui::SameLine(0, 12);

    // ── Search ──
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::InputTextWithHint("##search", "\xf0\x9f\x94\x8d  Filter...", searchBuf, sizeof(searchBuf));
    ImGui::PopStyleColor();
}

// ─── Log list ─────────────────────────────────────────────────────────────────

void MEditorConsoleWindow::drawLogList() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Transparent child bg so the window bg shows through between rows
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
    ImGui::BeginChild("##log_scroll", avail, false, ImGuiWindowFlags_HorizontalScrollbar);

    ImDrawList* dl      = ImGui::GetWindowDrawList();
    const float lineH   = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
    const float winX    = ImGui::GetWindowPos().x;
    const float winW    = ImGui::GetWindowSize().x;

    for (int i = 0; i < (int)entries.size(); ++i) {
        const SLogEntry& e = entries[i];
        if (!passesFilter(e)) continue;

        const bool isSelected = (selectedIndex == i);
        ImVec2     rowMin     = ImGui::GetCursorScreenPos();
        ImVec2     rowMax     = { winX + winW, rowMin.y + lineH };

        // ── Row background ──────────────────────────────────────────────
        if (isSelected)
            dl->AddRectFilled(rowMin, rowMax, ROW_SELECTED_BG);
        else if (ImGui::IsMouseHoveringRect(rowMin, rowMax))
            dl->AddRectFilled(rowMin, rowMax, ROW_HOVER_BG);

        // ── Coloured left-border strip ──────────────────────────────────
        dl->AddRectFilled(rowMin,
                          { rowMin.x + BORDER_STRIP_W, rowMax.y },
                          levelBgColor(e.level));

        // ── Invisible full-row selectable ───────────────────────────────
        // BUG FIX: original used &log (pointer into vector) as ID — dangling
        // after any push_back. We use the stable integer index instead.
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos({ rowMin.x + BORDER_STRIP_W + 4.0f, rowMin.y + 2.0f });
        if (ImGui::Selectable("##row", isSelected,
                              ImGuiSelectableFlags_SpanAllColumns,
                              { winW - BORDER_STRIP_W - 4.0f, lineH - 2.0f }))
        {
            selectedIndex = isSelected ? -1 : i;   // click again to deselect
        }
        ImGui::PopID();

        // ── Tag badge ───────────────────────────────────────────────────
        ImGui::SameLine(BORDER_STRIP_W + 8.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, levelColor(e.level));
        ImGui::TextUnformatted(levelTag(e.level));
        ImGui::PopStyleColor();

        // ── Message ─────────────────────────────────────────────────────
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Text,
            isSelected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                       : ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();
    }

    // ── Auto-scroll ─────────────────────────────────────────────────────────
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ─── Filter ───────────────────────────────────────────────────────────────────

bool MEditorConsoleWindow::passesFilter(const SLogEntry& e) const {
    if (activeFilter >= 0 && static_cast<int>(e.level) != activeFilter)
        return false;

    if (searchBuf[0] != '\0') {
        // Case-insensitive substring match
        SString haystack = e.text.toLower();
        SString needle   = SString(searchBuf).toLower();
        if (!haystack.contains(needle))
            return false;
    }
    return true;
}

// ─── Static colour helpers ────────────────────────────────────────────────────

ImVec4 MEditorConsoleWindow::levelColor(ELogLevel level) {
    switch (level) {
        case ELogLevel::Log:     return ImVec4(0.45f, 0.78f, 1.00f, 1.0f);  // sky blue
        case ELogLevel::Warning: return ImVec4(1.00f, 0.80f, 0.20f, 1.0f);  // amber
        case ELogLevel::Error:   return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);  // red
    }
    return ImVec4(1, 1, 1, 1);
}

ImU32 MEditorConsoleWindow::levelBgColor(ELogLevel level) {
    switch (level) {
        case ELogLevel::Log:     return IM_COL32(60,  140, 220,  160);
        case ELogLevel::Warning: return IM_COL32(220, 160,  30,  180);
        case ELogLevel::Error:   return IM_COL32(210,  50,  50,  200);
    }
    return IM_COL32(255, 255, 255, 80);
}

const char* MEditorConsoleWindow::levelTag(ELogLevel level) {
    switch (level) {
        case ELogLevel::Log:     return "[LOG]";
        case ELogLevel::Warning: return "[WRN]";
        case ELogLevel::Error:   return "[ERR]";
    }
    return "[???]";
}