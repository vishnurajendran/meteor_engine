//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once

#ifndef METEOR_ENGINE_IMGUISUBWINDOW_H
#define METEOR_ENGINE_IMGUISUBWINDOW_H
#include "imgui.h"
#include "core/meteor_core_minimal.h"

class MImGuiSubWindow : public MObject{
protected:
    SString title;
    ImVec2 minSize = ImVec2(0, 0);
    ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);
public:
    MImGuiSubWindow(int x, int y);
    SString getTitle() const;
    MImGuiSubWindow(const SString& title, int x, int y);
    ~MImGuiSubWindow();
    void setWindowConstraints(float minx, float miny, float maxx, float maxy);
    void draw();
    virtual void onGui()=0;
};


#endif //METEOR_ENGINE_IMGUISUBWINDOW_H
