//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#define METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#include "editor/meteorite_minimal.h"

class MEditorHierarchyWindow : public MImGuiSubWindow{
public:
    MEditorHierarchyWindow();
    MEditorHierarchyWindow(int x, int y);
    void onGui() override;
};


#endif //METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
