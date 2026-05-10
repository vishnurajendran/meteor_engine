//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORINSPECTORWINDOW_H
#define METEOR_ENGINE_EDITORINSPECTORWINDOW_H
#include "editor/meteorite_minimal.h"
#include "editor/window/imgui/imguisubwindow.h"

class MEditorInspectorWindow : public MImGuiSubWindow {
    DEFINE_OBJECT_SUBCLASS(MEditorInspectorWindow)
public:
    MEditorInspectorWindow();
    MEditorInspectorWindow(int x, int y);
    void onGui(float deltaTime) override;
};


#endif //METEOR_ENGINE_EDITORINSPECTORWINDOW_H
