//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H
#include "core/meteor_core_minimal.h"
class MScene;
/**
 * @brief Manages Scene loads and unloads.
*/
class MSceneManager : public MObject {
public:
    MSceneManager() = default;
    ~MSceneManager() override;
    virtual bool loadEmptyScene();
    virtual bool loadScene(const SString& path);
    virtual bool closeActiveScene();
    virtual void update(float deltaTime);
    virtual MScene* getActiveScene() { return activeScene; }
    static void registerSceneManager(MSceneManager* sceneManagerInstance);
    static MSceneManager* getSceneManagerInstance();
private:
    static MScene* activeScene;
    static MSceneManager* sceneManagerInstance;
};

#endif //SCENEMANAGER_H
