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
    void drawGUI();
    void draw();
public:
    MImGuiWindow(const SString& title);
    MImGuiWindow(const SString& title, int sizeX, int sizeY, int fps);
    void clear() override;
    void update() override;
    void close() override;
    void drawImGuiSubWindows();
    void showDockSpace();
    void drawMenuBar();
    void drawControls();

    //Todo:: move to asset manager
    void loadFontFile(const SString& pathToFile, float pointSize);
};


#endif //METEOR_ENGINE_IMGUIWINDOW_H
