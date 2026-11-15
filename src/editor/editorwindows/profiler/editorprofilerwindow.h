//
// editorprofilerwindow.h
//
// Profiler stats (MSimpleProfiler) as their own editor window: each key's
// time this frame plus a graph of recent frames, grouped by category.
// Opened from Window/Debug/Profiler. History is only recorded while the
// window is drawn.
//

#pragma once
#ifndef METEOR_ENGINE_EDITORPROFILERWINDOW_H
#define METEOR_ENGINE_EDITORPROFILERWINDOW_H

#include "editor/profiling/profiler_stats_displayer.h"
#include "editor/window/imgui/imguisubwindow.h"

class MEditorProfilerWindow : public MImGuiSubWindow
{
    DEFINE_OBJECT_SUBCLASS(MEditorProfilerWindow)

public:
    MEditorProfilerWindow();
    explicit MEditorProfilerWindow(int x, int y);

    void onGui(float deltaTime) override;

private:
    MProfilerStatsDisplayer displayer;
};

#endif // METEOR_ENGINE_EDITORPROFILERWINDOW_H
