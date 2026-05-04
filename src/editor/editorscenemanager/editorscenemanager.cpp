//
// editorscenemanager.cpp
//

#include "editorscenemanager.h"

#include "core/engine/camera/camera.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/scene/serialisation/scene_serialiser.h"
#include "editorscenecamera.h"

// ── Empty scene ───────────────────────────────────────────────────────────────

bool MEditorSceneManager::loadEmptyScene()
{
    const bool res = MSceneManager::loadEmptyScene();
    if (res)
        createEditorSceneCamera();
    return res;
}

// ── Load scene ────────────────────────────────────────────────────────────────

bool MEditorSceneManager::loadScene(const SString& path)
{
    if (!MSceneManager::loadEmptyScene())
    {
        MERROR("MEditorSceneManager::loadScene — failed to create empty scene");
        return false;
    }
    if (!MSceneSerializer::load(path, activeScene))
    {
        MERROR("MEditorSceneManager::loadScene — MSceneSerializer failed for: " + path);
        return false;
    }
    currentScenePath = path;
    createEditorSceneCamera();
    return true;
}

// ── Save scene ────────────────────────────────────────────────────────────────

bool MEditorSceneManager::saveCurrentScene(const SString& pathOverride)
{
    // Prefer the explicitly provided path; fall back to the remembered path.
    SString savePath = !pathOverride.empty() ? pathOverride : currentScenePath;

    if (savePath.empty())
    {
        MWARN("MEditorSceneManager::saveCurrentScene — no path set. Open a scene first.");
        return false;
    }

    // Persist so subsequent saves (without a path override) go to the same file.
    currentScenePath = savePath;

    return MSceneSerializer::save(activeScene, currentScenePath);
}

// ── Close scene ───────────────────────────────────────────────────────────────

bool MEditorSceneManager::closeActiveScene()
{
    if (editorSceneCamera)
    {
        editorSceneCamera->destroy();
        editorSceneCamera = nullptr;
    }
    return MSceneManager::closeActiveScene();
}

// ── Editor camera ─────────────────────────────────────────────────────────────

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
    cam->setWorldPosition(SVector3(0, 0, 20));

    // Hide from serializer so it is never written to the scene file.
    cam->setEntityFlags(EEntityFlags::HideInEditor);

    editorSceneCamera = cam;
}