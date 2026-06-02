#include "editorscenecamera.h"
#include "imgui.h"

#include "core/engine/camera/viewmanagement.h"
#include "core/engine/engine_statics.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "editor/settings/editor_settings.h"

SVector3 MEditorSceneCameraEntity::lastPosition = SVector3(0, 0, 0);
SQuaternion MEditorSceneCameraEntity::lastRotation = SQuaternion(0, 0, 0, 0);

void MEditorSceneCameraEntity::onCreate()
{
    setPriority(-9999);
    setClipPlanes(0.1f, 1000.0f);
    setEntityFlags(EEntityFlags::HideInEditor);

    // Explicitly register with the view system.  Do NOT rely on onStart() since
    // HideInEditor is already set by the time onStart() would auto-register,
    // and some implementations skip registration for hidden entities.
    MViewManagement::addCamera(this);

    // Remove from scene root list - editor camera is not a scene entity.
    auto* scene = MSceneManager::getSceneManagerInstance()->getActiveScene();
    if (!scene)
        return;
    auto& roots = scene->getRootEntities();
    auto it = std::find(roots.begin(), roots.end(), this);
    if (it != roots.end())
        roots.erase(it);

    if (settingsRef = dynamic_cast<MEditorSettings*>(MEngineStatics::getEngineSettings()))
    {
        setWorldPosition(settingsRef->lastEdCameraPos.get());
        setWorldRotation(settingsRef->lastEdCameraRot.get());

        lastPosition = settingsRef->lastEdCameraPos.get();
        lastRotation = settingsRef->lastEdCameraRot.get();
    }
    setCanTick(true);
}

void MEditorSceneCameraEntity::onUpdate(float deltaTime)
{
    MCameraEntity::onUpdate(deltaTime);

    if (lastPosition != getWorldPosition() || lastRotation != getWorldRotation())
    {
       if (!settingsRef)
           return;

        settingsRef->lastEdCameraPos.set(getWorldPosition());
        settingsRef->lastEdCameraRot.set(getWorldRotation());
        lastPosition = getWorldPosition();
        lastRotation = getWorldRotation();
    }
}

MEditorSceneCameraEntity::~MEditorSceneCameraEntity()
{
    MViewManagement::removeCamera(this);
}