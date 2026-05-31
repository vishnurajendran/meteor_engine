//
// Created by ssj5v on 23-05-2026.
//

#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H
#include "core/engine/subsystem/subsystem_interface.h"
#include "editor/editor_constants.h"
#include "editorapplication.h"


class MEditorApplication;
class MEditorSimulationManagerSubsystem : public IEngineSubSystem {
public:
    MEditorSimulationManagerSubsystem() = default;
    ~MEditorSimulationManagerSubsystem() override = default;

    void init() override;
    void cleanup() override;
private:
    void onSimulationStateChanged(EEditorSimulationState state);
    void onSimulate();
    void onSimulationStopped();
    void onSimulationPaused();

    void createSceneSnapshot();
    void loadSnapShotSceneForSimulation();
    void restoreFromSceneSnapshot();

private:
    MEditorApplication* appInst;
    MEditorSceneManager* sceneManagerInst;
    SString callbackId = "";

    SString activeScenePath = "";
    SString sceneSnapshotPath = "";
};

#endif //SIMULATION_MANAGER_H
