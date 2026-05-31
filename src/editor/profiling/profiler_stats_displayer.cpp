//
// Created by ssj5v on 31-05-2026.
//

#include "profiler_stats_displayer.h"
#include "core/profiling/simple_profiler/simple_profiler.h"

// --- Overlay style constants (match the scene-view overlay look) -------------
static constexpr ImU32  OVL_BG_PROF     = IM_COL32(22,  22,  22,  210);
static constexpr ImU32  OVL_BORDER_PROF = IM_COL32(70,  70,  70,  200);
static constexpr ImU32  OVL_DIVIDER_PROF= IM_COL32(80,  80,  80,  200);
static constexpr float  OVL_ROUNDING_P  = 5.0f;
static constexpr float  OVL_PAD_P       = 6.0f;

// Graph colours per-entry (cycles if more keys than colours)
static constexpr ImU32 GRAPH_COLORS[] = {
    IM_COL32(100, 180, 255, 255),   // blue
    IM_COL32(100, 220, 100, 255),   // green
    IM_COL32(255, 200,  80, 255),   // yellow
    IM_COL32(255, 100, 100, 255),   // red
    IM_COL32(200, 130, 255, 255),   // purple
    IM_COL32(255, 160, 100, 255),   // orange
};
static constexpr int GRAPH_COLOR_COUNT = sizeof(GRAPH_COLORS) / sizeof(GRAPH_COLORS[0]);

// --- Helpers identical to the ones in editorsceneviewwindow.cpp --------------
static void beginProfilerPanel(const char* id, ImVec2 screenPos, ImVec2 size)
{
    ImGui::SetCursorScreenPos(screenPos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(OVL_BG_PROF));
    ImGui::PushStyleColor(ImGuiCol_Border,  ImGui::ColorConvertU32ToFloat4(OVL_BORDER_PROF));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(OVL_PAD_P, OVL_PAD_P));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, OVL_ROUNDING_P);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
}

static void endProfilerPanel()
{
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

// --- SProfilerHistory --------------------------------------------------------

void SProfilerHistory::push(float value)
{
    samples[head] = value;
    head = (head + 1) % PROFILER_HISTORY_SIZE;
    if (count < PROFILER_HISTORY_SIZE)
        ++count;

    // Recompute min/max over the valid range
    minVal =  FLT_MAX;
    maxVal = -FLT_MAX;
    for (int i = 0; i < count; ++i)
    {
        float s = samples[i];
        if (s < minVal) minVal = s;
        if (s > maxVal) maxVal = s;
    }
}

// --- MProfilerStatsDisplayer -------------------------------------------------

void MProfilerStatsDisplayer::sampleHistory()
{
    const auto& times = MSimpleProfiler::getExecutionTimes();
    for (const auto& [key, ms] : times)
        histories[key].push(ms);
}

void MProfilerStatsDisplayer::draw(const ImVec2& viewportMin, const ImVec2& viewportSize)
{
    sampleHistory();

    const auto& times = MSimpleProfiler::getExecutionTimes();
    if (times.empty()) return;

    const float margin  = 10.0f;
    const float panelW  = 450.0f;
    const float graphH  = 40.0f;   // height of each mini line-graph
    const float lineH   = ImGui::GetTextLineHeightWithSpacing();

    // Each entry: label row + graph
    const float entryH  = lineH + graphH + 4.0f;
    const float headerH = lineH + 6.0f;
    const float panelH  = headerH + (times.size() * entryH) + OVL_PAD_P * 3.0f;

    // Position: top-left of viewport
    float posX = viewportMin.x + margin;
    float posY = viewportMin.y + margin;

    beginProfilerPanel("##profiler_graphs", { posX, posY }, { panelW, panelH });
    {
        // -- Header -------------------------------------------------------
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
        ImGui::TextUnformatted("Profiler Stats");
        ImGui::PopStyleColor();

        ImGui::SameLine(panelW - 40.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::TextUnformatted("(ms)");
        ImGui::PopStyleColor();

        ImGui::Dummy({0.0f, 2.0f});
        ImGui::GetWindowDrawList()->AddLine(
            ImGui::GetCursorScreenPos(),
            {ImGui::GetCursorScreenPos().x + panelW - (OVL_PAD_P * 2), ImGui::GetCursorScreenPos().y},
            OVL_DIVIDER_PROF);
        ImGui::Dummy({0.0f, 2.0f});

        // -- Per-key rows -------------------------------------------------
        int colorIdx = 0;
        for (const auto& [keyName, timeMs] : times)
        {
            ImU32 lineColor = GRAPH_COLORS[colorIdx % GRAPH_COLOR_COUNT];
            ++colorIdx;

            // Label + current value
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(lineColor));
            ImGui::TextUnformatted(keyName.c_str());
            ImGui::PopStyleColor();

            char timeBuf[32];
            std::snprintf(timeBuf, sizeof(timeBuf), "%.3f", timeMs);
            float textW = ImGui::CalcTextSize(timeBuf).x;
            ImGui::SameLine(panelW - textW - OVL_PAD_P);

            // Color-code the value text (green / yellow / red)
            ImVec4 timeColor = ImVec4(0.35f, 0.90f, 0.35f, 1.0f);
            if      (timeMs > 16.0f) timeColor = ImVec4(0.95f, 0.30f, 0.25f, 1.0f);
            else if (timeMs >  5.0f) timeColor = ImVec4(0.95f, 0.80f, 0.20f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
            ImGui::TextUnformatted(timeBuf);
            ImGui::PopStyleColor();

            // Line graph
            auto it = histories.find(keyName);
            if (it != histories.end())
                drawLineGraph(it->second, panelW - OVL_PAD_P * 2.0f, graphH, lineColor);
            else
                ImGui::Dummy({panelW - OVL_PAD_P * 2.0f, graphH});

            ImGui::Dummy({0.0f, 2.0f}); // spacing between entries
        }
    }
    endProfilerPanel();
}

void MProfilerStatsDisplayer::drawLineGraph(const SProfilerHistory& history,
                                             float graphWidth, float graphHeight,
                                             ImU32 lineColor) const
{
    if (history.count < 2)
    {
        ImGui::Dummy({graphWidth, graphHeight});
        return;
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Dark background for the graph area
    dl->AddRectFilled(pos,
                      {pos.x + graphWidth, pos.y + graphHeight},
                      IM_COL32(10, 10, 10, 180),
                      3.0f);

    // Subtle border
    dl->AddRect(pos,
                {pos.x + graphWidth, pos.y + graphHeight},
                IM_COL32(50, 50, 50, 150),
                3.0f);

    // Determine value range with a small floor so flat lines don't disappear
    float rangeMin = history.minVal;
    float rangeMax = history.maxVal;
    float span     = rangeMax - rangeMin;
    if (span < 0.01f)
    {
        rangeMin -= 0.5f;
        rangeMax += 0.5f;
        span = rangeMax - rangeMin;
    }

    // Add 10% padding top and bottom
    rangeMin -= span * 0.1f;
    rangeMax += span * 0.1f;
    span = rangeMax - rangeMin;

    // Draw the polyline.  We read from oldest to newest.
    const int n = history.count;
    const float dx = graphWidth / static_cast<float>(n - 1);

    // Oldest sample index
    int oldest = (history.head - n + PROFILER_HISTORY_SIZE) % PROFILER_HISTORY_SIZE;

    ImVec2 prev;
    for (int i = 0; i < n; ++i)
    {
        int idx = (oldest + i) % PROFILER_HISTORY_SIZE;
        float t = (history.samples[idx] - rangeMin) / span;           // 0..1
        t = std::clamp(t, 0.0f, 1.0f);

        ImVec2 pt = {
            pos.x + i * dx,
            pos.y + graphHeight - t * graphHeight   // y = 0 at bottom
        };

        if (i > 0)
            dl->AddLine(prev, pt, lineColor, 1.5f);

        prev = pt;
    }

    // Draw a faint filled area under the line (per-segment quads to handle non-convex shapes)
    if (n >= 2)
    {
        ImU32 fillColor = (lineColor & 0x00FFFFFF) | 0x20000000;  // same hue, low alpha
        float bottomY = pos.y + graphHeight;

        for (int i = 0; i < n - 1; ++i)
        {
            int idx0 = (oldest + i)     % PROFILER_HISTORY_SIZE;
            int idx1 = (oldest + i + 1) % PROFILER_HISTORY_SIZE;

            float t0 = std::clamp((history.samples[idx0] - rangeMin) / span, 0.0f, 1.0f);
            float t1 = std::clamp((history.samples[idx1] - rangeMin) / span, 0.0f, 1.0f);

            ImVec2 tl = { pos.x + i * dx,       pos.y + graphHeight - t0 * graphHeight };
            ImVec2 tr = { pos.x + (i + 1) * dx, pos.y + graphHeight - t1 * graphHeight };
            ImVec2 br = { tr.x, bottomY };
            ImVec2 bl = { tl.x, bottomY };

            dl->AddTriangleFilled(tl, tr, br, fillColor);
            dl->AddTriangleFilled(tl, br, bl, fillColor);
        }
    }

    // Min/max labels
    char minBuf[16], maxBuf[16];
    std::snprintf(minBuf, sizeof(minBuf), "%.1f", history.minVal);
    std::snprintf(maxBuf, sizeof(maxBuf), "%.1f", history.maxVal);

    dl->AddText({pos.x + 3.0f, pos.y + graphHeight - ImGui::GetTextLineHeight() - 1.0f},
                IM_COL32(180, 180, 180, 120), minBuf);
    dl->AddText({pos.x + 3.0f, pos.y + 1.0f},
                IM_COL32(180, 180, 180, 120), maxBuf);

    ImGui::Dummy({graphWidth, graphHeight});
}