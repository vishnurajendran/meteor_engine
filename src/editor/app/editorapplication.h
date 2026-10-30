#pragma once
#include "core/application/application.h"
#include "core/default_settings_paths.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "editor/window/imgui/imguisubwindow.h"
#include "editor/window/imgui/imguiwindow.h"
#include "editor/project/editor_project_manager.h"
#include "core/utils/delegate.h"

#ifndef METEOR_ENGINE_EDITORAPPLICATION_H
#define METEOR_ENGINE_EDITORAPPLICATION_H


class MEditorSimulationManagerSubsystem;
class MEditorAssetManager;
class IAudioEngineSubsystem;
class IPhysicsEngineSubsystem;
class MEditorSceneManager;

enum EEditorSimulationState
{
    Stopped = 0,
    Simulating,
    SimulationPaused,
};

class MEditorApplication : public MApplication {
public:
    MEditorApplication();
    void initialise() override;
    void run() override;
    void cleanup() override;
    bool isRunning() const override;

    void pause(const bool& pause) override {}; // no API support for this func here

    [[nodiscard]] bool isPaused()  const override  { return simulationState == SimulationPaused; };
    [[nodiscard]] bool isPlaying() const override  { return simulationState == Simulating;       };
    [[nodiscard]] bool isSimulating() const override { return simulationState == Simulating;     };
    float getPhysicsStep() const;

    SString getEngineSettingsPath() const override { return SString::format("{0}{1}", DEFAULT_SETTINGS_PATH, "EditorSettings.xml"); }

    void showSplashScreen();
    void loadPrerequisites();

    void startSimulation();
    void stopSimulation();

    // False while scripts are compiling or any script has errors - Play is
    // blocked then. outReason says why, for the Play button's tooltip.
    [[nodiscard]] bool canStartSimulation(SString& outReason) const;
    void pauseSimulation();

    // Fired whenever Play / Pause / Stop changes the simulation state.
    TMulticastDelegate<EEditorSimulationState> onSimulationStateChanged;

private:
    // Time the script refresh may use per frame before yielding, so the
    // editor keeps drawing (and the progress overlay can show) during a
    // long recompile.
    static constexpr double SCRIPT_REFRESH_BUDGET_MS = 8.0;

    void registerSubsystems();
    void notifySimulationStateChange();
    void tickPhysics(float deltaTime);

public:
    static void exit();
    static MCameraEntity* getSceneCamera();
    MEditorProjectManager& getProjectManager() { return projectManager; }
    static MObject* SelectedObject;

private:
    MEditorSceneManager* sceneManagerRef = nullptr;
    MEditorAssetManager* assetManagerRef = nullptr;
    MEditorProjectManager projectManager;
    MEditorSimulationManagerSubsystem* simulationManagerSubsystem = nullptr;

    // sub-system interfaces
    IAudioEngineSubsystem* audioEngineRef = nullptr;
    IPhysicsEngineSubsystem* physicsEngineRef = nullptr;
    IRenderPipelineManagerSubsystem* pipelineManager= nullptr;

    MObjectPtr<MImGuiWindow> window = nullptr;
    std::atomic<bool> splashShowing = true;

    float physicsAccumulator = 0.0f;  // seconds carried over between frames
    EEditorSimulationState simulationState = EEditorSimulationState::Stopped;

    // The editor's own subscriptions to engine events.
    SScopedCallbackDelegateHandle windowResizedSubscription;
    SScopedCallbackDelegateHandle sceneLoadedSubscription;
    SScopedCallbackDelegateHandle instanceReplacedSubscription;
};

#endif