#include "imgui.h"
#include "editorscenecamera.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"

void MEditorSceneCameraEntity::onCreate()
{
    setPriority(-9999);
    setClipPlanes(0.1f, 1000.0f);
    setEntityFlags(EEntityFlags::HideInEditor);

    // Explicitly register with the view system.  Do NOT rely on onStart() since
    // HideInEditor is already set by the time onStart() would auto-register,
    // and some implementations skip registration for hidden entities.
    MViewManagement::addCamera(this);

    // Remove from scene root list — editor camera is not a scene entity.
    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
    if (!scene) return;
    auto& roots = scene->getRootEntities();
    auto it = std::find(roots.begin(), roots.end(), this);
    if (it != roots.end()) roots.erase(it);
}

MEditorSceneCameraEntity::~MEditorSceneCameraEntity()
{
    MViewManagement::removeCamera(this);
}