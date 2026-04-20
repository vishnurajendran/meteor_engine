#pragma once
#ifndef SCENE_IO_H
#define SCENE_IO_H

#include "core/utils/sstring.h"

class MSceneIO
{
public:
    // Clear all entities and reset subsystems (lights, skybox, etc.)
    static void clearScene();

    // Save to the path the scene was last opened from / saved to.
    static bool saveCurrentScene();

    // Save to an explicit path and remember it as the current path.
    static bool saveScene(const SString& filePath);

    // Destroy the current scene, load from file, remember path.
    static bool loadScene(const SString& filePath);

    // Clear the current scene and start fresh (no file).
    static bool newScene();

    static const SString& getCurrentScenePath() { return s_currentPath; }
    static void setCurrentScenePath(const SString& p) { s_currentPath = p; }

private:
    static SString s_currentPath;
};

#endif