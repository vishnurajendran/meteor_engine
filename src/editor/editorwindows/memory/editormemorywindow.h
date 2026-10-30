//
// editormemorywindow.h
//
// One table: the engine's own trackers (object pool, C++ heap, Jolt), then
// the MObject types with live objects, biggest first.
//

#pragma once
#ifndef METEOR_ENGINE_EDITORMEMORYWINDOW_H
#define METEOR_ENGINE_EDITORMEMORYWINDOW_H

#include <string>
#include <vector>

#include "editor/window/imgui/imguisubwindow.h"

class MEditorMemoryWindow : public MImGuiSubWindow
{
    DEFINE_OBJECT_SUBCLASS(MEditorMemoryWindow)

public:
    MEditorMemoryWindow();
    explicit MEditorMemoryWindow(int x, int y);

    void onGui(float deltaTime) override;

private:
    struct SRow
    {
        std::string name;
        size_t      count  = 0;     // 0 = not applicable
        size_t      bytes  = 0;
        bool        header = false; // summary row (not a type)
    };

    void refresh();

    static constexpr float REFRESH_INTERVAL = 0.5f; // seconds

    std::vector<SRow> rows;
    float timeSinceRefresh = REFRESH_INTERVAL; // refresh on first draw
};

#endif // METEOR_ENGINE_EDITORMEMORYWINDOW_H
