#pragma once
#include "editor/meteorite.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H

class MEditorAssetManager;
class MCameraEntity;
class MEditorSceneManager;
class MImGuiSubWindow;
class MImGuiWindow;

class MEditorApplication : public MApplication {
private:
    MEditorSceneManager* sceneManagerRef;
    MEditorAssetManager* assetManagerRef;
    MObjectPtr<MImGuiWindow> window;
    std::vector<MObjectPtr<MImGuiSubWindow>> subWindows;
    static MEditorApplication *editorInst;
    MRenderPipelineManager pipelineManager;
    std::atomic<bool> splashShowing = true;

public:
    // Unified selection — can be MSpatialEntity* or MAsset*.
    // For assets, SelectedAssetId is also set so the inspector can resolve
    // the correct instance after a refresh() without a dangling pointer.
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