#pragma once
#ifndef SCENE_IO_H
#define SCENE_IO_H

#include "core/utils/sstring.h"

class MSceneIO
{
public:
    // Save to the path the scene was last opened from / saved to.
    static bool saveCurrentScene();

    // Destroy the current scene, load from file, remember path.
    static bool loadScene(const SString& filePath);

    // Clear the current scene and start fresh (no file).
    static bool newScene();

    // The path the current scene was last loaded from or saved to.
    // Empty if the scene has never been saved.
    [[nodiscard]] static SString getCurrentPath() { return s_currentPath; }
    static void setCurrentPath(const SString& path) { s_currentPath = path; }
private:
    static SString s_currentPath;
};

#endif