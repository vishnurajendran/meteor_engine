#pragma once
#include "core/application/application.h"
#include "core/default_settings_paths.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "editor/window/imgui/imguisubwindow.h"
#include "editor/window/imgui/imguiwindow.h"

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
    float getPhysicsStep() const;

    SString getEngineSettingsPath() const override { return SString::format("{0}{1}", DEFAULT_SETTINGS_PATH, "EditorSettings.xml"); }

    void showSplashScreen();
    void loadPrerequisites();

    void startSimulation();
    void stopSimulation();
    void pauseSimulation();

    // registers a callback function callback, returns it Id handle.
    SString registerToSimulationStateChangedCallback(std::function<void(const EEditorSimulationState&)> callback);
    void unregisterFromSimulationStateChangedCallback(const SString& callbackId);

private:
    void registerSubsystems();
    void notifySimulationStateChange();
    void tickPhysics(float deltaTime);

public:
    static void exit();
    static MCameraEntity* getSceneCamera();
    static MObject* SelectedObject;

private:
    MEditorSceneManager* sceneManagerRef = nullptr;
    MEditorAssetManager* assetManagerRef = nullptr;
    MEditorSimulationManagerSubsystem* simulationManagerSubsystem = nullptr;

    // sub-system interfaces
    IAudioEngineSubsystem* audioEngineRef = nullptr;
    IPhysicsEngineSubsystem* physicsEngineRef = nullptr;
    IRenderPipelineManagerSubsystem* pipelineManager= nullptr;

    MObjectPtr<MImGuiWindow> window = nullptr;
    std::vector<MObjectPtr<MImGuiSubWindow>> subWindows;
    std::atomic<bool> splashShowing = true;

    float physicsAccumulator = 0.0f;  // seconds carried over between frames
    std::unordered_map<SString, std::function<void(EEditorSimulationState)>> simulationStateChangedCallbacks;
    EEditorSimulationState simulationState = EEditorSimulationState::Stopped;

    bool internal_tickSpatialFixedUpdateFlag   = false;
};

#endif