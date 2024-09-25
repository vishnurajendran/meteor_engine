//
// Created by Vishnu Rajendran on 2024-09-24.
//
#pragma once

#ifndef METEOR_ENGINE_IMGUISUBWINDOW_H
#define METEOR_ENGINE_IMGUISUBWINDOW_H
#include "core/meteor_core_minimal.h"

class MImGuiSubWindow : public MObject{
protected:
    SString title;
public:
    MImGuiSubWindow(int x, int y);
    SString getTitle() const;
    MImGuiSubWindow(const SString& title, int x, int y);
    ~MImGuiSubWindow();
    void draw();
    virtual void onGui()=0;
};


#endif //METEOR_ENGINE_IMGUISUBWINDOW_H
