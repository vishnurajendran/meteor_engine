#include "editorprofilerwindow.h"

#include "editor/editorwindows/registry/editor_subwindow_registry.h"

EDITOR_REGISTER_SUBWINDOW("Profiler", "Window/Debug/Profiler", MEditorProfilerWindow)

MEditorProfilerWindow::MEditorProfilerWindow() : MEditorProfilerWindow(420, 500) {}

MEditorProfilerWindow::MEditorProfilerWindow(const int x, const int y) : MImGuiSubWindow(x, y)
{
    title = "Profiler";
}

void MEditorProfilerWindow::onGui(float /*deltaTime*/)
{
    displayer.draw();
}
