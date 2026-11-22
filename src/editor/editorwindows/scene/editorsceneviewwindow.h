#pragma once
#ifndef METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H
#define METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H

#include "ImGuizmo.h"
#include "SFML/Graphics/RenderTexture.hpp"
#include "core/engine/ui/viewport/viewport.h"
#include "core/utils/delegate.h"
#include "core/utils/glmhelper.h"
#include "editor/window/imgui/imguisubwindow.h"

class MSpatialEntity;
class MEditorApplication;
class MCameraEntity;

// Also the game UI's viewport host in the editor: while playing, the Scene
// view shows the game, and input over its image goes to the UI.
class MEditorSceneViewWindow : public MImGuiSubWindow, public IViewportHost
{
public:

private:
    DEFINE_OBJECT_SUBCLASS(MEditorSceneViewWindow)
public:
    MEditorSceneViewWindow();
    explicit MEditorSceneViewWindow(int x, int y);

    ~MEditorSceneViewWindow() override;

    void onGui(float deltaTime) override;
    void handleInput(float dt) override;

    // -- IViewportHost ---------------------------------------------------------
    // Applied during Play. Confined behaves as Free here (the Scene view can't
    // confine the OS cursor). Locked holds the cursor at the image's centre
    // while the Scene view has focus. Escape unlocks and shows it (the cursor
    // goes back to visible / Free, as scripts see it too).
    void applyCursor(bool visible, ECursorState state) override;
    void setCursorShape(const SString& cursorName) override { uiCursorShape = cursorName; }

private:
    void tickDeltaTimeInfo(const float& deltaTime);
    void tickViewportInteraction();

    void drawScene(ImVec2 region);
    void drawEditorSceneView(const float& deltaTime, const ImVec2& region);
    void handleDragDropBehaviour();
    void drawToolbar(bool playMode);
    void drawFpsOverlay();
    void drawSceneInfoOverlay();
    void drawAxisLegend();
    void drawTransformHandles();

    // -- Game UI (Play) ----------------------------------------------------------
    [[nodiscard]] bool isPlayActive() const;     // simulating or paused
    [[nodiscard]] MViewport* uiViewport() const;
    // Window events, mapped into the game image's rectangle, go to the UI while
    // playing and while the image is hovered (mouse) or the window focused (keys).
    void forwardEventToUI(const sf::Event& event);
    // The same events to the input subsystem (SInput's mouse queries) while
    // playing: moves and presses over the game image, and the releases and
    // drags of presses that started there.
    void forwardEventToInput(const sf::Event& event);
    // Locked cursor in effect right now (Play, Locked, focused).
    [[nodiscard]] bool isCursorLockActive() const;
    // Per frame: warps the cursor while locked and tells the input subsystem.
    void updateCursorLock();
    void releaseGameMouse();
    // A window position -> game image pixels.
    [[nodiscard]] SVector2 toGameImagePixels(sf::Vector2i windowPosition) const;
    [[nodiscard]] bool isOverGameImage(sf::Vector2i windowPosition) const;
    void applyUICursor() const;
    void drawUIDebuggerToggle(float iconH);

    void trySelectEntity(MCameraEntity* camera);
    bool screenPointToRay(MCameraEntity* camera, const ImVec2& screenPx, SVector3& outOrigin, SVector3& outDir) const;
    MSpatialEntity* pickEntity(MCameraEntity* camera, const SVector3& rayOrigin, const SVector3& rayDir,
                               float pickRadiusPx = 12.0f) const;
    bool handleCameraMouseInputs(MCameraEntity* camera, float dt);
    void handleCameraKeyboardInputs(MCameraEntity* camera, float dt);
    void focusOnSelected(MCameraEntity* camera);

    SString getCurrentTransformGizmoText() const;
    SString getCurrentTransformModeText() const;

private:
    // -- Render target ---------------------------------------------------------
    sf::RenderTexture renderTexture;
    sf::ContextSettings settings;
    void updateRenderTarget();

    // -- Viewport state --------------------------------------------------------
    ImVec2 viewportMin = {};
    ImVec2 viewportSize = {};
    bool viewportHovered = false;
    bool sceneImageHovered  = false;   // the image item itself, not covered by another window
    bool sceneWindowFocused = false;
    bool uiPointerInside    = false;   // the UI has been told the pointer is over it
    bool gamePointerInside  = false;   // the input subsystem has been told the same
    uint32_t gameMouseButtons = 0;     // bit per button id pressed over the game image, not yet released

    bool          uiCursorVisible     = true;
    ECursorState  uiCursorState       = ECursorState::Free;
    SString       uiCursorShape;
    SScopedCallbackDelegateHandle windowEventSubscription;

    ImGuizmo::OPERATION transformOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE transformMode = ImGuizmo::LOCAL;
    bool gizmosEnabled = true; // toggled by the eye button

    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float cameraMoveSpeed = 10.0f;
    float cameraSensitivity = 0.15f;

    // FPS tracking (0.5s smoothing window)
    float fpsAccum = 0.0f;
    int fpsFrames = 0;
    float displayFps = 0.0f;

    // -- Icons -----------------------------------------------------------------
    sf::Texture translateIcon;
    sf::Texture rotateIcon;
    sf::Texture scaleIcon;
    sf::Texture localSpaceIcon;
    sf::Texture worldSpaceIcon;
    sf::Texture gizmoOnIcon; // eye-open  icon
    sf::Texture gizmoOffIcon; // eye-closed icon


    MEditorApplication* editorAppInst;
};

#endif // METEOR_ENGINE_EDITORSCENEVIEWWINDOW_H