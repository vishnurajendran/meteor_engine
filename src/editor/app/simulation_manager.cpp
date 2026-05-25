//
// Created by ssj5v on 23-05-2026.
//

#include "simulation_manager.h"

#include "editor/editorscenemanager/editorscenemanager.h"

void MEditorSimulationManagerSubsystem::init()
{
    appInst = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
    sceneManagerInst = dynamic_cast<MEditorSceneManager*>(MSceneManager::getSceneManagerInstance());

    if (!appInst || !sceneManagerInst)
    {
        MLOG("SceneManager or AppInst is NULL");
        return;
    }

    callbackId = appInst->registerToSimulationStateChangedCallback([this](const EEditorSimulationState& state)
                                                                   { onSimulationStateChanged(state); });

    MLOG("Simulation manager initialized");
}

void MEditorSimulationManagerSubsystem::cleanup()
{
    if (!appInst)
        return;
    if (callbackId.empty())
        return;
    appInst->unregisterFromSimulationStateChangedCallback(callbackId);
}

void MEditorSimulationManagerSubsystem::onSimulationStateChanged(EEditorSimulationState state)
{
    switch (state)
    {
    case EEditorSimulationState::Stopped:
        onSimulationStopped();
        break;
    case EEditorSimulationState::Simulating:
        onSimulate();
        break;
    case EEditorSimulationState::SimulationPaused:
        onSimulationPaused();
        break;
    }
}

void MEditorSimulationManagerSubsystem::createSceneSnapshot()
{
    if (!sceneManagerInst)
        return;

    auto activeScene = sceneManagerInst->getActiveScene();
    if (!activeScene)
        return;

    // we need to store this, so that we can restore this after play mode.
    activeScenePath = sceneManagerInst->internal_getCurrentScenePath();
    if (FileIO::directoryExists(SEditorPaths::DIR_TEMP))
        FileIO::createDirectory(SEditorPaths::DIR_TEMP);

    sceneSnapshotPath = SString::format("{0}/{1}", SEditorPaths::DIR_TEMP, FileIO::getFileName(activeScenePath));
    if (sceneManagerInst->saveCurrentScene(sceneSnapshotPath))
    {
        MLOG("MEditorSimulationManagerSubsystem::Created scene snapshot");
    }
}
void MEditorSimulationManagerSubsystem::loadSnapShotSceneForSimulation()
{
    sceneManagerInst->closeActiveScene();
    if (!sceneManagerInst->loadScene(sceneSnapshotPath))
    {
        MERROR("MEditorSimulationManagerSubsystem::Failed loading scene snapshot");
    }
}

void MEditorSimulationManagerSubsystem::restoreFromSceneSnapshot()
{
    MLOG("MEditorSimulationManagerSubsystem::Restoring Snapshot");
    sceneManagerInst->closeActiveScene();
    if (sceneManagerInst->loadScene(sceneSnapshotPath))
    {
        sceneManagerInst->internal_OverrideCurrentScenePath(activeScenePath);
        // delete the snapshot file after load. we don't need it any more.
        FileIO::deleteFile(sceneSnapshotPath);
        activeScenePath = "";
        sceneSnapshotPath = "";
    }

    // no need to enable the scene camera, it is enabled by default after scene load.
}

void MEditorSimulationManagerSubsystem::onSimulate()
{
    createSceneSnapshot();
    loadSnapShotSceneForSimulation();

    // disable scene Camera, so that the system uses the first available camera in the scene.
    if (const auto* editorSceneCamera = appInst->getSceneCamera())
    {
        appInst->getSceneCamera()->setEnabled(false);
    }
    MLOG("MEditorSimulationManagerSubsystem::Prepped for simulation");
}

void MEditorSimulationManagerSubsystem::onSimulationStopped()
{
    restoreFromSceneSnapshot();
}

void MEditorSimulationManagerSubsystem::onSimulationPaused()
{
    // do nothing during simulation pause.
}
