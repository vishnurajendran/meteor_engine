//
// Created by ssj5v on 22-01-2025.
//

#include "editorscenemanager.h"
#include "core/engine/camera/camera.h"

bool MEditorSceneManager::loadEmptyScene()
{
    const auto res =  MSceneManager::loadEmptyScene();
    if (res)
    {
        createEditorSceneCamera();
    }
    return res;
}

bool MEditorSceneManager::loadScene(const SString& path)
{
    const auto res =  MSceneManager::loadScene(path);
    if (res)
    {
        createEditorSceneCamera();
    }
    return res;
}

bool MEditorSceneManager::closeActiveScene()
{
    /*MSpatialEntity::destroy(editorSceneCamera);
    editorSceneCamera = nullptr;
    */

    return MSceneManager::closeActiveScene();
}

MCameraEntity* MEditorSceneManager::getEditorSceneCamera() const
{
    return editorSceneCamera;
}

void MEditorSceneManager::createEditorSceneCamera()
{
    /*MSpatialEntity::destroy(editorSceneCamera);
    editorSceneCamera = new MCameraEntity(true);
    editorSceneCamera->setWorldPosition(SVector3(0, 0, 20));*/
}
