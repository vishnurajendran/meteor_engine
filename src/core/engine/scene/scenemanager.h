//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "core/object/object.h"
#include "core/utils/delegate.h"

class MScene;
/**
 * @brief Manages Scene loads and unloads.
*/

SCRIPT_BIND_CLASS()
class MSceneManager : public MObject {
    DEFINE_OBJECT_SUBCLASS(MSceneManager)
public:
    MSceneManager() = default;
    ~MSceneManager() override;
    virtual void init() { loadEmptyScene(); }

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
    static void registerSceneManager(MSceneManager* sceneManagerInstance);

    SCRIPT_BIND_FUNC_STATIC()
    static MSceneManager* getSceneManagerInstance();

private:

protected:
    MScene* activeScene;
    SString currentScenePath;

private:
    static MSceneManager* sceneManagerInstance;
};

#endif //SCENEMANAGER_H
