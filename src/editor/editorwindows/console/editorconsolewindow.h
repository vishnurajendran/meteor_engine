//
// Created by Vishnu Rajendran on 2024-09-24.
//

#pragma once
#ifndef METEOR_ENGINE_EDITORCONSOLEWINDOW_H
#define METEOR_ENGINE_EDITORCONSOLEWINDOW_H

#include "editor/meteorite_minimal.h"

class MEditorConsoleWindow : public MImGuiSubWindow {
private:
    std::vector<SString> logs;
    void onLogRecieved(SString log);
    int subscriptionId;
public:
    MEditorConsoleWindow();
    ~MEditorConsoleWindow();
    MEditorConsoleWindow(int x, int y);
    void onGui() override;
};


#endif //METEOR_ENGINE_EDITORCONSOLEWINDOW_H