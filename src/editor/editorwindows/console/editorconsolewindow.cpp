//
// Created by Vishnu Rajendran on 2024-09-24.
//

#include "editorconsolewindow.h"
#include "editor/window/imgui/imguisubwindow.h"
#include <cstring>
#include <cstdio>
#include <ctime>

// -- Style constants -------------------------------------------------------
static constexpr ImU32 ROW_SELECTED_BG  = IM_COL32(60,  90,  160, 80);
static constexpr ImU32 ROW_HOVER_BG     = IM_COL32(255, 255, 255, 12);
static constexpr ImU32 ROW_ALT_BG       = IM_COL32(255, 255, 255, 4);
static constexpr float BORDER_STRIP_W   = 3.0f;
static constexpr float DETAIL_PANE_H    = 130.0f;

// -- Construction ----------------------------------------------------------

MEditorConsoleWindow::MEditorConsoleWindow() : MEditorConsoleWindow(700, 300) {}

MEditorConsoleWindow::MEditorConsoleWindow(int x, int y) : MImGuiSubWindow(x, y) {
    title = "Console";
    subscriptionId = MLogger::subscribe(
        std::bind(&MEditorConsoleWindow::onLogReceived, this, std::placeholders::_1));
}

MEditorConsoleWindow::~MEditorConsoleWindow() {
    MLogger::unsubscribe(subscriptionId);
}

// -- Utility ---------------------------------------------------------------

SString MEditorConsoleWindow::shortenPath(const SString& fullPath) {
    const char* raw   = fullPath.c_str();
    const char* slash = strrchr(raw, '/');
    if (!slash) slash = strrchr(raw, '\\');
    return SString(slash ? slash + 1 : raw);
}

// -- Log ingestion ---------------------------------------------------------

void MEditorConsoleWindow::onLogReceived(const SLogMessage& msg) {
    SLogEntry entry;
    entry.tag        = msg.tag;
    entry.text       = msg.text;
    entry.fullFile   = SString(msg.location.file);
    entry.shortFile  = shortenPath(entry.fullFile);
    entry.line       = msg.location.line;
    entry.function   = SString(msg.location.function);
    entry.stackTrace = msg.stackTrace;

    if      (entry.tag == "WRN") entry.level = ELogLevel::Warning;
    else if (entry.tag == "ERR") entry.level = ELogLevel::Error;
    else                          entry.level = ELogLevel::Log;

    // Format timestamp as HH:MM:SS.mmm
    auto epoch_ms   = std::chrono::duration_cast<std::chrono::milliseconds>(
                          msg.timestamp.time_since_epoch());
    auto time_t_val = std::chrono::system_clock::to_time_t(msg.timestamp);
    int  ms         = static_cast<int>(epoch_ms.count() % 1000);

    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_buf);
#endif

    char tsBuf[16];
    snprintf(tsBuf, sizeof(tsBuf), "%02d:%02d:%02d.%03d",
             tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, ms);
    entry.timestamp = SString(tsBuf);

    ++levelCounts[static_cast<int>(entry.level)];
    entries.push_back(std::move(entry));
}

// -- onGui -----------------------------------------------------------------

void MEditorConsoleWindow::onGui(float deltaTime) {
    drawToolbar();
    ImGui::Separator();
    drawLogList();

    // Detail pane when an entry is selected
    if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
        ImGui::Separator();
        drawDetailPane();
    }
}

// -- Toolbar ---------------------------------------------------------------

void MEditorConsoleWindow::drawToolbar() {
    // -- Clear --
    if (ImGui::Button("Clear")) {
        entries.clear();
        levelCounts = {};
        selectedIndex = -1;
    }

    ImGui::SameLine(0, 10);

    // -- Auto-scroll toggle --
    ImGui::PushStyleColor(ImGuiCol_Button,
        autoScroll ? ImVec4(0.2f, 0.5f, 0.9f, 0.6f) : ImVec4(0, 0, 0, 0));
    if (ImGui::Button(autoScroll ? "  Scroll \xe2\x86\x93  " : "  Scroll \xe2\x86\x91  "))
        autoScroll = !autoScroll;
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(autoScroll ? "Auto-scroll on" : "Auto-scroll off");

    ImGui::SameLine(0, 10);

    // -- Copy selected --
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

    // -- Level filter buttons -- [All N] [LOG N] [WRN N] [ERR N]
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
            activeFilter = (activeFilter == b.idx) ? -1 : b.idx;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 4);
    }

    ImGui::SameLine(0, 12);

    // -- Search --
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::InputTextWithHint("##search", "\xf0\x9f\x94\x8d  Filter...", searchBuf, sizeof(searchBuf));
    ImGui::PopStyleColor();
}

// -- Log list --------------------------------------------------------------

void MEditorConsoleWindow::drawLogList() {
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Reserve vertical space for the detail pane when a row is selected
    const bool hasDetail = (selectedIndex >= 0 && selectedIndex < (int)entries.size());
    if (hasDetail)
        avail.y -= (DETAIL_PANE_H + 8.0f);

    // Transparent child bg so the window bg shows through between rows
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
    ImGui::BeginChild("##log_scroll", avail, false, ImGuiWindowFlags_HorizontalScrollbar);

    ImDrawList* dl      = ImGui::GetWindowDrawList();
    const float lineH   = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
    const float winX    = ImGui::GetWindowPos().x;
    const float winW    = ImGui::GetWindowSize().x;
    int         visIdx  = 0;   // counter for alternating row tint

    for (int i = 0; i < (int)entries.size(); ++i) {
        const SLogEntry& e = entries[i];
        if (!passesFilter(e)) continue;

        const bool isSelected = (selectedIndex == i);
        ImVec2     rowMin     = ImGui::GetCursorScreenPos();
        ImVec2     rowMax     = { winX + winW, rowMin.y + lineH };

        // -- Row background (alternating tint, selection, hover) ----------
        if (isSelected)
            dl->AddRectFilled(rowMin, rowMax, ROW_SELECTED_BG);
        else if (ImGui::IsMouseHoveringRect(rowMin, rowMax))
            dl->AddRectFilled(rowMin, rowMax, ROW_HOVER_BG);
        else if (visIdx % 2 == 1)
            dl->AddRectFilled(rowMin, rowMax, ROW_ALT_BG);

        // -- Coloured left-border strip -----------------------------------
        dl->AddRectFilled(rowMin,
                          { rowMin.x + BORDER_STRIP_W, rowMax.y },
                          levelBgColor(e.level));

        // -- Invisible full-row selectable --------------------------------
        ImGui::PushID(i);
        ImGui::SetCursorScreenPos({ rowMin.x + BORDER_STRIP_W + 4.0f, rowMin.y + 2.0f });
        if (ImGui::Selectable("##row", isSelected,
                              ImGuiSelectableFlags_SpanAllColumns,
                              { winW - BORDER_STRIP_W - 4.0f, lineH - 2.0f }))
        {
            selectedIndex = isSelected ? -1 : i;
        }
        ImGui::PopID();

        // -- Timestamp (dim) ----------------------------------------------
        ImGui::SameLine(BORDER_STRIP_W + 8.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.40f, 0.40f, 1.0f));
        ImGui::TextUnformatted(e.timestamp.c_str());
        ImGui::PopStyleColor();

        // -- Tag badge ----------------------------------------------------
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Text, levelColor(e.level));
        ImGui::TextUnformatted(levelTag(e.level));
        ImGui::PopStyleColor();

        // -- Message ------------------------------------------------------
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Text,
            isSelected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
                       : ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
        ImGui::TextUnformatted(e.text.c_str());
        ImGui::PopStyleColor();

        // -- Source location (dim, after message) -------------------------
        if (e.line > 0) {
            ImGui::SameLine(0, 12);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
            char locBuf[128];
            snprintf(locBuf, sizeof(locBuf), "%s:%d", e.shortFile.c_str(), e.line);
            ImGui::TextUnformatted(locBuf);
            ImGui::PopStyleColor();
        }

        ++visIdx;
    }

    // -- Auto-scroll ------------------------------------------------------
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// -- Detail pane -----------------------------------------------------------
// Shows full info for the selected entry, including stack trace for errors.

void MEditorConsoleWindow::drawDetailPane() {
    const SLogEntry& e = entries[selectedIndex];

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::BeginChild("##detail_pane", ImVec2(0, DETAIL_PANE_H), true);

    // Header line: [TAG] message
    ImGui::PushStyleColor(ImGuiCol_Text, levelColor(e.level));
    ImGui::TextUnformatted(levelTag(e.level));
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 8);
    ImGui::TextWrapped("%s", e.text.c_str());

    ImGui::Spacing();

    // Source location
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 1.0f));
    if (e.line > 0) {
        ImGui::Text("Location: %s:%d  in  %s()",
                     e.fullFile.c_str(), e.line, e.function.c_str());
    }
    ImGui::Text("Time: %s", e.timestamp.c_str());
    ImGui::PopStyleColor();

    // Stack trace (errors only, when available)
    if (e.stackTrace.length() > 0) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
        ImGui::TextUnformatted("Stack Trace:");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.42f, 0.42f, 1.0f));
        ImGui::BeginChild("##stacktrace", ImVec2(0, 0), false);
        ImGui::TextUnformatted(e.stackTrace.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// -- Filter ----------------------------------------------------------------

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

// -- Static colour helpers -------------------------------------------------

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