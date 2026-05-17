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
    IRenderPipelineManagerSubsystem* pipelineManager;
    std::atomic<bool> splashShowing = true;

public:
    // Unified selection - can be MSpatialEntity* or MAsset*.
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

private:
    void addSubsystems();
};

#endif