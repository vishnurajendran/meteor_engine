//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once
#ifndef METEOR_ENGINE_WINDOWMANAGER_H
#define METEOR_ENGINE_WINDOWMANAGER_H

#include "core/meteor_core_minimal.h"

class MWindow;

class MWindowManager {
public:
    static MWindow* getSimpleWindow(const SString& title, int x=800, int y=600, int fps=0);
    static MWindow* getImGuiWindow(const SString& title, int x=800, int y=600, int fps=0);
};


#endif //METEOR_ENGINE_WINDOWMANAGER_H
