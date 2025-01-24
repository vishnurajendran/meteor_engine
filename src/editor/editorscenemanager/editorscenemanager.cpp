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
    delete editorSceneCamera;
    return MSceneManager::closeActiveScene();
}

MCameraEntity* MEditorSceneManager::getEditorSceneCamera() const
{
    return editorSceneCamera;
}

void MEditorSceneManager::createEditorSceneCamera()
{
    delete editorSceneCamera;
    editorSceneCamera = new MCameraEntity();
    editorSceneCamera->setEntityFlags(EEntityFlags::HideInEditor);
    editorSceneCamera->setWorldPosition(SVector3(0, 0, 10));
    editorSceneCamera->setName("_EditorSceneCamera");
}
