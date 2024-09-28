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
    static bool loadEmptyScene();
    static bool loadScene(const SString& path);
    static bool closeActiveScene();
    static void update(float deltaTime);
    static MScene* getActiveScene() { return activeScene; }
private:
    static MScene* activeScene;
};

#endif //SCENEMANAGER_H
