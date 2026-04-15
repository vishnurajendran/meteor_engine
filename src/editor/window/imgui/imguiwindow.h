//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_IMGUIWINDOW_H
#define METEOR_ENGINE_IMGUIWINDOW_H

#include "imgui.h"
#include "core/window/simple/windowing.h"

class MImGuiWindow : public MWindow {
private:
    std::function<void(void)> graphicsFunction;

private:
    void drawGUI(float deltaTime);

public:
    bool initialiseWindow(const SString& inTitle, SVector2 inSize, int inFps) override;

    void clear() override;
    void update(float deltaTime) override;
    void close() override;
    void drawImGuiSubWindows(float deltaTime);
    void showDockSpace();
    void drawMenuBar();
    void drawControls();

    void setGraphicsCall(std::function<void(void)> func)
    {
        graphicsFunction = func;
    };

    //Todo:: move to asset manager
    void loadFontFile(const SString& pathToFile, float pointSize);
};


#endif //METEOR_ENGINE_IMGUIWINDOW_H
