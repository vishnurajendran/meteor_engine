//
// Created by ssj5v on 24-04-2025.
//

#include "editorscenecamera.h"

#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
MEditorSceneCameraEntity::MEditorSceneCameraEntity()
{
    name = EDITOR_CAMERA_NAME;
    setPriority(-9999);
    setClipPlanes(0.1f, 1000.0f);
    setEntityFlags(EEntityFlags::HideInEditor);

    auto scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
    if (scene == nullptr) return;
    auto& roots = scene->getRootEntities();
    auto it = std::find(roots.begin(), roots.end(), this);
    if (it == roots.end()) return;
    roots.erase(it);
}
