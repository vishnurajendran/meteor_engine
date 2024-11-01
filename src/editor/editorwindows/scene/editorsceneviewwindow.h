//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#define METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#include "ImGuizmo.h"
#include "editor/meteorite_minimal.h"

class MEditorSceneViewWindow : public MImGuiSubWindow{
private:
    sf::RenderTexture renderTexture;
    sf::ContextSettings settings;
    sf::Vector2u sceneImageCursorPosition;

    ImGuizmo::OPERATION transformOperation=ImGuizmo::TRANSLATE;
    ImGuizmo::MODE transformMode=ImGuizmo::LOCAL;

    sf::Texture localSpaceIcon;
    sf::Texture worldSpaceIcon;

    sf::Texture translateIcon;
    sf::Texture rotateIcon;
    sf::Texture scaleIcon;

public:
    MEditorSceneViewWindow();
    MEditorSceneViewWindow(int x, int y);
    void onGui() override;
private:
    void drawTransformGizmoSelector();
    void drawModeSelector();
    void drawTranformHandles();
    SString getCurrentTransformGizmoText() const;
    SString getCurrentTransformModeText() const;
};


#endif //METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
