//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#define METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#include "ImGuizmo.h"
#include "editor/meteorite_minimal.h"


class MCameraEntity;
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
    void handleInput(float deltaTime) override;
private:
    void drawTransformGizmoSelector();
    void drawModeSelector();
    void drawTranformHandles();
    SString getCurrentTransformGizmoText() const;
    SString getCurrentTransformModeText() const;

    bool handleCameraMouseInputs(MCameraEntity* camera, float delta_time);
    void handleCameraKeyboardInputs(MCameraEntity* camera, const float& deltaTime);

    float cameraMoveSpeed = 1;
    float cameraYaw = 0;
    float cameraPitch = 0;
    float cameraSensitivity = 0.1f;
};


#endif //METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
