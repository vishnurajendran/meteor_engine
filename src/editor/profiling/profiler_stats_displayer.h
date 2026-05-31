//
// Created by ssj5v on 31-05-2026.
//

#ifndef PROFILER_STATS_DISPLAYER_H
#define PROFILER_STATS_DISPLAYER_H

#include <array>
#include <map>
#include "editor/meteorite_minimal.h"

/// Maximum number of historical samples stored per profiler key.
static constexpr int PROFILER_HISTORY_SIZE = 128;

/// A small ring-buffer that stores the last N float samples for one key.
struct SProfilerHistory
{
    std::array<float, PROFILER_HISTORY_SIZE> samples{};
    int   head  = 0;   // next write position
    int   count = 0;   // how many valid entries (up to PROFILER_HISTORY_SIZE)
    float minVal = 0.0f;
    float maxVal = 0.0f;

    void push(float value);
};

/// Draws the profiler stats ImGui panel with per-key line graphs.
///
/// Usage:
///   In your editor window constructor, create an instance.
///   Each frame, call `draw(viewportMin, viewportSize)` from your GUI code.
class MProfilerStatsDisplayer : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MProfilerStatsDisplayer)

public:
    MProfilerStatsDisplayer()  = default;
    ~MProfilerStatsDisplayer() override = default;

    /// Samples the current profiler times and draws the overlay.
    void draw(const ImVec2& viewportMin, const ImVec2& viewportSize);

private:
    /// Updates internal history ring-buffers from MSimpleProfiler.
    void sampleHistory();

    /// Draws a single line-graph for one key inside the current ImGui context.
    void drawLineGraph(const SProfilerHistory& history,
                       float graphWidth, float graphHeight,
                       ImU32 lineColor) const;

    std::map<SString, SProfilerHistory> histories;
};

#endif // PROFILER_STATS_DISPLAYER_H