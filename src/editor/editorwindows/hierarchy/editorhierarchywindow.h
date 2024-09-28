//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#define METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#include "editor/meteorite_minimal.h"

class MEditorHierarchyWindow : public MImGuiSubWindow{
private:
    sf::Texture sceneTex;
    sf::Texture entityTex;
public:
    MEditorHierarchyWindow();
    MEditorHierarchyWindow(int x, int y);
    void onGui() override;
    void drawRecursiveSceneTree(MSpatialEntity* spatial, int depth);
    void drawLeaf(MSpatialEntity* spatial);
};


#endif //METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
