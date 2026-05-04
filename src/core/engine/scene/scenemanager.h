//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "functional"
#include "core/meteor_core_minimal.h"

class MScene;
/**
 * @brief Manages Scene loads and unloads.
*/
class MSceneManager : public MObject {
public:
    MSceneManager() = default;
    ~MSceneManager() override;
    virtual void init() { loadEmptyScene(); }
    virtual bool loadEmptyScene();
    virtual bool loadScene(const SString& path);
    virtual bool closeActiveScene();
    virtual void update(float deltaTime);

    SString registerOnLoadCallback(std::function<void(MScene*)> callback);
    void deregisterOnLoadCallback(SString callbackId);


    virtual MScene* getActiveScene() { return activeScene; }
    virtual SString getActiveScenePath() { return currentScenePath; }

public:
    static void registerSceneManager(MSceneManager* sceneManagerInstance);
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
