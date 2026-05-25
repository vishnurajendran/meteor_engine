//
// editorscenemanager.cpp
//

#include "editorscenemanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/engine_statics.h"
#include "core/engine/scene/serialisation/scene_serialiser.h"
#include "editor/app/editorapplication.h"
#include "editor/settings/editor_settings.h"
#include "editorscenecamera.h"


bool MEditorSceneManager::loadEmptyScene()
{
    const bool res = MSceneManager::loadEmptyScene();
    if (res)
        createEditorSceneCamera();
    return res;
}


bool MEditorSceneManager::loadScene(const SString& path)
{
    if (!MSceneManager::loadEmptyScene())
    {
        MERROR("MEditorSceneManager::loadScene -- failed to create empty scene");
        return false;
    }
    if (!MSceneSerializer::load(path, activeScene))
    {
        MERROR("MEditorSceneManager::loadScene -- MSceneSerializer failed for: " + path);
        return false;
    }
    currentScenePath = path;
    createEditorSceneCamera();

    if (const auto settings = dynamic_cast<MEditorSettings*>(MEngineStatics::getEngineSettings()))
    {
        // only do this when app is not simulating
        const auto* appInstRef = dynamic_cast<MEditorApplication*>(MApplication::getAppInstance());
        if (appInstRef && !appInstRef->isPlaying() && !appInstRef->isPaused())
        {
            MVERBOSE(SString::format("[MEditorApplication]::Setting last opened scene {0}", currentScenePath));
            settings->lastOpenedScene.set(currentScenePath);
        }
    }

    return true;
}


bool MEditorSceneManager::saveCurrentScene(const SString& pathOverride)
{
    // Prefer the explicitly provided path; fall back to the remembered path.
    SString savePath = !pathOverride.empty() ? pathOverride : currentScenePath;

    if (savePath.empty())
    {
        MWARN("MEditorSceneManager::saveCurrentScene -- no path set. Open a scene first.");
        return false;
    }

    // Persist so subsequent saves (without a path override) go to the same file.
    currentScenePath = savePath;

    return MSceneSerializer::save(activeScene, currentScenePath);
}


bool MEditorSceneManager::closeActiveScene()
{
    // Clear the editor selection before destroying any entities. SelectedObject
    // is a raw MObject* that would otherwise dangle after the scene frees its
    // entities, crashing on the next frame when the hierarchy or scene view
    // tries to dereference it.
    MEditorApplication::SelectedObject = nullptr;

    if (editorSceneCamera)
    {
        editorSceneCamera->destroy();
        editorSceneCamera = nullptr;
    }
    return MSceneManager::closeActiveScene();
}


void MEditorSceneManager::update(float deltaTime)
{
    MSceneManager::update(deltaTime);
    if (editorSceneCamera && editorSceneCamera->getCanTick())
    {
        editorSceneCamera->onUpdate(deltaTime);
    }
}


MCameraEntity* MEditorSceneManager::getEditorSceneCamera() const
{
    return editorSceneCamera;
}

void MEditorSceneManager::createEditorSceneCamera()
{
    if (editorSceneCamera)
    {
        MViewManagement::removeCamera(editorSceneCamera);
        editorSceneCamera->destroy();
        editorSceneCamera = nullptr;
    }

    auto* cam = MSpatialEntity::createInstance<MEditorSceneCameraEntity>(EDITOR_CAMERA_NAME);
    // Hide from serializer so it is never written to the scene file.
    cam->setEntityFlags(EEntityFlags::HideInEditor);

    editorSceneCamera = cam;
}