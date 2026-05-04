#pragma once
#ifndef METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#define METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H

#include "ImGuizmo.h"
#include "editor/meteorite_minimal.h"

class MCameraEntity;

class MEditorSceneViewWindow : public MImGuiSubWindow
{
public:
    MEditorSceneViewWindow();
    explicit MEditorSceneViewWindow(int x, int y);

    void onGui(float deltaTime) override;
    void handleInput(float dt) override;

private:
    // ── Render target ─────────────────────────────────────────────────────────
    sf::RenderTexture  renderTexture;
    sf::ContextSettings settings;
    void               updateRenderTarget();

    // ── Viewport state ────────────────────────────────────────────────────────
    ImVec2  viewportMin     = {};
    ImVec2  viewportSize    = {};
    bool    viewportHovered = false;

    // ── Overlay panels ────────────────────────────────────────────────────────
    void drawOverlayToolbar();
    void drawCameraSpeedOverlay();
    void drawViewportInfoOverlay();

    // ── Gizmo handles ─────────────────────────────────────────────────────────
    void drawTransformHandles();

    ImGuizmo::OPERATION transformOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      transformMode      = ImGuizmo::LOCAL;
    bool                gizmosEnabled      = true;   // toggled by the eye button

    // ── Click-to-select ───────────────────────────────────────────────────────
    void trySelectEntity(MCameraEntity* camera);

    bool screenPointToRay(MCameraEntity*   camera,
                          const ImVec2&    screenPx,
                          SVector3&        outOrigin,
                          SVector3&        outDir) const;

    MSpatialEntity* pickEntity(MCameraEntity*  camera,
                               const SVector3& rayOrigin,
                               const SVector3& rayDir,
                               float           pickRadiusPx = 12.0f) const;

    // ── Camera inputs ─────────────────────────────────────────────────────────
    bool handleCameraMouseInputs(MCameraEntity* camera, float dt);
    void handleCameraKeyboardInputs(MCameraEntity* camera, float dt);
    void focusOnSelected(MCameraEntity* camera);

    float cameraYaw         =  0.0f;
    float cameraPitch       =  0.0f;
    float cameraMoveSpeed   = 10.0f;
    float cameraSensitivity =  0.15f;

    // FPS tracking (0.5s smoothing window)
    float fpsAccum   = 0.0f;
    int   fpsFrames  = 0;
    float displayFps = 0.0f;

    // ── Icons ─────────────────────────────────────────────────────────────────
    sf::Texture translateIcon;
    sf::Texture rotateIcon;
    sf::Texture scaleIcon;
    sf::Texture localSpaceIcon;
    sf::Texture worldSpaceIcon;
    sf::Texture gizmoOnIcon;    // eye-open  icon
    sf::Texture gizmoOffIcon;   // eye-closed icon

    // ── Text helpers ──────────────────────────────────────────────────────────
    SString getCurrentTransformGizmoText() const;
    SString getCurrentTransformModeText()  const;
};

#endif // METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H