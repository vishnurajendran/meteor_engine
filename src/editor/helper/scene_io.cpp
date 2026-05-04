#include "scene_io.h"
#include "core/engine/entities/spatial/spatial.h"
#include "editor/editorscenemanager/editorscenemanager.h"

SString MSceneIO::s_currentPath = "";

bool MSceneIO::saveCurrentScene()
{
    auto* editorSceneManager = dynamic_cast<MEditorSceneManager*>(
        MSceneManager::getSceneManagerInstance());
    if (!editorSceneManager) return false;

    // Pass s_currentPath so the manager has a path even on the first save
    // after opening a file (it will prefer its own currentScenePath if set).
    return editorSceneManager->saveCurrentScene(s_currentPath);
}

bool MSceneIO::loadScene(const SString& filePath)
{
    if (filePath.empty())
    {
        MERROR("MSceneIO::loadScene — empty path");
        return false;
    }

    auto* editorSceneManager = dynamic_cast<MEditorSceneManager*>(
        MSceneManager::getSceneManagerInstance());
    if (!editorSceneManager)
    {
        MERROR("MSceneIO::loadScene — not running in editor context");
        return false;
    }

    const bool ok = editorSceneManager->loadScene(filePath);
    if (ok)
        s_currentPath = filePath;

    return ok;
}

bool MSceneIO::newScene()
{
    MSceneManager::getSceneManagerInstance()->loadEmptyScene();
    s_currentPath = "";
    MLOG("MSceneIO: new scene.");
    return true;
}