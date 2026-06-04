// // Created by ssj5v on 31-05-2026.
//

#include "profiler_stats_displayer.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include <string>
#include <vector>
#include <algorithm>

// --- Overlay style constants -------------------------------------------------
static constexpr ImU32  OVL_BG_PROF      = IM_COL32(22,  22,  22,  210);
static constexpr ImU32  OVL_BORDER_PROF  = IM_COL32(70,  70,  70,  200);
static constexpr ImU32  OVL_DIVIDER_PROF = IM_COL32(80,  80,  80,  200);
static constexpr float  OVL_ROUNDING_P   = 5.0f;
static constexpr float  OVL_PAD_P        = 6.0f;
static constexpr float  GRAPH_RIGHT_PAD  = 12.0f; // <-- Extra breathing room on the right side

static constexpr ImU32 GRAPH_COLORS[] = {
    IM_COL32(100, 180, 255, 255),   // blue
    IM_COL32(100, 220, 100, 255),   // green
    IM_COL32(255, 200,  80, 255),   // yellow
    IM_COL32(255, 100, 100, 255),   // red
    IM_COL32(200, 130, 255, 255),   // purple
    IM_COL32(255, 160, 100, 255),   // orange
};
static constexpr int GRAPH_COLOR_COUNT = sizeof(GRAPH_COLORS) / sizeof(GRAPH_COLORS[0]);

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

void SProfilerHistory::push(float value)
{
    samples[head] = value;
    head = (head + 1) % PROFILER_HISTORY_SIZE;
    if (count < PROFILER_HISTORY_SIZE)
        ++count;

    minVal =  FLT_MAX;
    maxVal = -FLT_MAX;
    for (int i = 0; i < count; ++i)
    {
        float s = samples[i];
        if (s < minVal) minVal = s;
        if (s > maxVal) maxVal = s;
    }
}

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
    const float panelW  = 420.0f;
    const float graphH  = 38.0f;

    // Group and sort keys by category dynamically using the '.' separator
    std::map<std::string, std::vector<std::pair<SString, float>>> groupedTimes;
    for (const auto& [keyName, timeMs] : times)
    {
        std::string keyStr = keyName.c_str();
        size_t dotPos = keyStr.find('.');

        std::string category = "General";
        if (dotPos != std::string::npos)
        {
            category = keyStr.substr(0, dotPos);
        }
        groupedTimes[category].push_back({ keyName, timeMs });
    }

    float posX = viewportMin.x + margin;
    float posY = viewportMin.y + margin;
    float activePanelH = std::clamp(lastPanelHeight, 50.0f, viewportSize.y - margin * 2.0f);

    beginProfilerPanel("##profiler_graphs", { posX, posY }, { panelW, activePanelH });
    {
        // -- Main Title Header ---------------------------------------------
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
        ImGui::TextUnformatted("Profiler Stats");
        ImGui::PopStyleColor();

        // Unit tracker aligned with the right-padding margin
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("(ms)").x - GRAPH_RIGHT_PAD);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::TextUnformatted("(ms)");
        ImGui::PopStyleColor();

        ImGui::Dummy({0.0f, 2.0f});
        ImGui::GetWindowDrawList()->AddLine(
            ImGui::GetCursorScreenPos(),
            {ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x - GRAPH_RIGHT_PAD, ImGui::GetCursorScreenPos().y},
            OVL_DIVIDER_PROF);
        ImGui::Dummy({0.0f, 4.0f});

        // -- Process Categories -------------------------------------------
        for (const auto& [category, items] : groupedTimes)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
            bool categoryOpen = ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleVar();

            if (categoryOpen)
            {
                ImGui::Indent(12.0f);
                ImGui::Dummy({0.0f, 2.0f});

                int colorIdx = 0;
                for (const auto& [keyName, timeMs] : items)
                {
                    ImU32 lineColor = GRAPH_COLORS[colorIdx % GRAPH_COLOR_COUNT];
                    ++colorIdx;

                    std::string keyStr = keyName.c_str();
                    size_t dotPos = keyStr.find('.');
                    std::string label = (dotPos != std::string::npos) ? keyStr.substr(dotPos + 1) : keyStr;

                    // Display Sub-label
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(lineColor));
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::PopStyleColor();

                    // Format and cleanly align the metric string accounting for right padding
                    char timeBuf[32];
                    std::snprintf(timeBuf, sizeof(timeBuf), "%.3f", timeMs);
                    float textW = ImGui::CalcTextSize(timeBuf).x;

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - textW - GRAPH_RIGHT_PAD);

                    ImVec4 timeColor = ImVec4(0.40f, 0.85f, 0.40f, 1.0f); // Healthy Green
                    if      (timeMs > 16.67f) timeColor = ImVec4(0.95f, 0.30f, 0.25f, 1.0f); // Alert Red
                    else if (timeMs >  8.33f) timeColor = ImVec4(0.95f, 0.75f, 0.20f, 1.0f); // Warning Yellow

                    ImGui::PushStyleColor(ImGuiCol_Text, timeColor);
                    ImGui::TextUnformatted(timeBuf);
                    ImGui::PopStyleColor();

                    // Draw sparkline with custom right padding applied
                    float targetGraphW = ImGui::GetContentRegionAvail().x - GRAPH_RIGHT_PAD;
                    auto it = histories.find(keyName);
                    if (it != histories.end())
                        drawLineGraph(it->second, targetGraphW, graphH, lineColor);
                    else
                        ImGui::Dummy({targetGraphW, graphH});

                    ImGui::Dummy({0.0f, 4.0f});
                }
                ImGui::Unindent(12.0f);
            }
        }

        lastPanelHeight = ImGui::GetCursorPosY() + OVL_PAD_P * 2.0f;
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

    dl->AddRectFilled(pos, {pos.x + graphWidth, pos.y + graphHeight}, IM_COL32(14, 14, 14, 200), 3.0f);
    dl->AddRect(pos, {pos.x + graphWidth, pos.y + graphHeight}, IM_COL32(45, 45, 45, 120), 3.0f);

    float rangeMin = history.minVal;
    float rangeMax = history.maxVal;
    float span     = rangeMax - rangeMin;
    if (span < 0.01f)
    {
        rangeMin -= 0.5f;
        rangeMax += 0.5f;
        span = rangeMax - rangeMin;
    }

    rangeMin -= span * 0.05f;
    rangeMax += span * 0.05f;
    span = rangeMax - rangeMin;

    const int n = history.count;
    const float dx = graphWidth / static_cast<float>(n - 1);
    int oldest = (history.head - n + PROFILER_HISTORY_SIZE) % PROFILER_HISTORY_SIZE;

    ImVec2 prev;
    for (int i = 0; i < n; ++i)
    {
        int idx = (oldest + i) % PROFILER_HISTORY_SIZE;
        float t = std::clamp((history.samples[idx] - rangeMin) / span, 0.0f, 1.0f);

        ImVec2 pt = { pos.x + i * dx, pos.y + graphHeight - t * graphHeight };
        if (i > 0)
            dl->AddLine(prev, pt, lineColor, 1.25f);

        prev = pt;
    }

    if (n >= 2)
    {
        ImU32 fillColor = (lineColor & 0x00FFFFFF) | 0x15000000;
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

    char minBuf[16], maxBuf[16];
    std::snprintf(minBuf, sizeof(minBuf), "%.1f", history.minVal);
    std::snprintf(maxBuf, sizeof(maxBuf), "%.1f", history.maxVal);

    dl->AddText({pos.x + 4.0f, pos.y + graphHeight - ImGui::GetTextLineHeight()}, IM_COL32(140, 140, 140, 100), minBuf);
    dl->AddText({pos.x + 4.0f, pos.y + 1.0f}, IM_COL32(140, 140, 140, 100), maxBuf);

    ImGui::Dummy({graphWidth, graphHeight});
}