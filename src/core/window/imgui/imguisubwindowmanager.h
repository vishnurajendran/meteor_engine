//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_IMGUISUBWINDOWMANAGER_H
#define METEOR_ENGINE_IMGUISUBWINDOWMANAGER_H

#include "vector"
#include "core/meteor_core_minimal.h"

class MImGuiSubWindow;

class MImGuiSubWindowManager {
private:
    static std::vector<MImGuiSubWindow*> windows;
public:
    static void add(MImGuiSubWindow* subWindow);
    static void remove(MImGuiSubWindow* subWindow);
    static std::vector<MImGuiSubWindow*> getSubWindows();
};


#endif //METEOR_ENGINE_IMGUISUBWINDOWMANAGER_H
