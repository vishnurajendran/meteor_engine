//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "core/engine/subsystem/subsystem_interface.h"
#include "core/engine/subsystem/subsystem_registry.h"   // so users can reach the manager
#include "core/object/object.h"
#include "core/utils/delegate.h"

class MScene;
/**
 * @brief Manages Scene loads and unloads.
 *
 * An engine subsystem, registered under its own type by each application
 * (MEditorSceneManager in the editor, MSceneManager in the player):
 *
 *   auto* scenes = MEngineSubsystemRegistry::getSubsystem<MSceneManager>();
 *
 * The application drives update / fixedUpdate / lateUpdate itself, in its
 * frame order, so canTick() stays false.
*/

SCRIPT_BIND_CLASS()
class MSceneManager : public MObject, public IEngineSubSystem {
    DEFINE_OBJECT_SUBCLASS(MSceneManager)
public:
    MSceneManager() = default;
    ~MSceneManager() override;

    // -- IEngineSubSystem ----------------------------------------------------
    // init() starts with an empty scene. cleanup() closes the active scene;
    // applications close it themselves before the registry cleans up, while
    // every other subsystem (physics, audio, scripting) is still alive.
    void init() override { loadEmptyScene(); }
    void cleanup() override { closeActiveScene(); }

    SCRIPT_BIND_FUNC()
    virtual bool loadEmptyScene();

    SCRIPT_BIND_FUNC()
    virtual bool loadScene(const SString& path);

    virtual bool closeActiveScene();
    virtual void update(float deltaTime);
    virtual void fixedUpdate(float fixedDeltaTime);
    virtual void lateUpdate(float deltaTime);
    virtual void postPhysicsStep(float fixedDeltaTime);
    virtual void interpolatePhysics(float alpha);

    // Fired after a scene finishes loading, with the new active scene.
    TMulticastDelegate<MScene*> onSceneLoaded;

    SCRIPT_BIND_FUNC()
    virtual MScene* getActiveScene() { return activeScene; }

    SCRIPT_BIND_FUNC()
    virtual SString getActiveScenePath() { return currentScenePath; }

public:
    // Script entry point (MSceneManager.getSceneManagerInstance() in Lua).
    // C++ code uses MEngineSubsystemRegistry::getSubsystem<MSceneManager>().
    SCRIPT_BIND_FUNC_STATIC()
    static MSceneManager* getSceneManagerInstance();

protected:
    MScene* activeScene = nullptr;
    SString currentScenePath;
};

#endif //SCENEMANAGER_H
