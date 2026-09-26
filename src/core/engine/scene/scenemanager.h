//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "core/object/object.h"
#include "functional"

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

    SString registerOnLoadCallback(std::function<void(MScene*)> callback);
    void deregisterOnLoadCallback(SString callbackId);

    SCRIPT_BIND_FUNC()
    virtual MScene* getActiveScene() { return activeScene; }

    SCRIPT_BIND_FUNC()
    virtual SString getActiveScenePath() { return currentScenePath; }

public:
    static void registerSceneManager(MSceneManager* sceneManagerInstance);

    SCRIPT_BIND_FUNC_STATIC()
    static MSceneManager* getSceneManagerInstance();

private:
    std::unordered_map<SString, std::function<void(MScene*)>> sceneLoadCallbackListeners;
    void informSceneLoadCallbackListeners(MScene* scene);

protected:
    MScene* activeScene;
    SString currentScenePath;

private:
    static MSceneManager* sceneManagerInstance;
};

#endif //SCENEMANAGER_H
