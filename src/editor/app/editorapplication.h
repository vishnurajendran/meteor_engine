#pragma once
#include "editor/meteorite.h"
#include "editor/window/imgui/imguiwindow.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H


class MEditorAssetManager;
class MEditorSceneManager;
class MEditorApplication : public MApplication {
private:
    MEditorSceneManager* sceneManagerRef;
    MEditorAssetManager* assetManagerRef;
    MObjectPtr<MImGuiWindow> window;
    std::vector<MObjectPtr<MImGuiSubWindow>> subWindows;
    static MEditorApplication *editorInst;
    MRenderPipelineManager pipelineManager;
    std::atomic<bool> splashShowing = true;

    // Checks Ctrl+S / Ctrl+Shift+S each frame during the ImGui pass.
    void processGlobalShortcuts();

    // Save the currently selected asset or scene.
    void saveSelected();

    // Save all dirty assets and the scene if dirty.
    void saveAllDirty();

public:
    // Unified selection — can be MSpatialEntity* or MAsset*.
    static MObject* SelectedObject;

public:
    MEditorApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    bool isRunning() const override;
    SString getEngineSettingsPath() const override { return "EditorSettings.xml"; }

    void showSplashScreen();
    void loadPrerequisites();

    static void exit();
    static MCameraEntity* getSceneCamera();
};

#endif