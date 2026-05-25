//
// Created by ssj5v on 22-01-2025.
//

#ifndef EDITORSCENEMANAGER_H
#define EDITORSCENEMANAGER_H
#include "core/engine/camera/camera_spatial_entity.h" // full type needed by MObjectPtr
#include "core/engine/scene/scenemanager.h"

class MEditorSceneManager : public MSceneManager {
    DEFINE_OBJECT_SUBCLASS(MEditorSceneManager)
public:
    MEditorSceneManager() = default;
    ~MEditorSceneManager() override = default;

    bool loadEmptyScene() override;
    bool loadScene(const SString& path) override;
    bool saveCurrentScene(const SString& pathOverride="");
    bool closeActiveScene() override;
    void update(float deltaTime) override;

    MCameraEntity* getEditorSceneCamera() const;

    void internal_OverrideCurrentScenePath(const SString& path) { currentScenePath = path; };
    SString internal_getCurrentScenePath() const { return currentScenePath; };

private:
    void createEditorSceneCamera();
private:
    MCameraEntity* editorSceneCamera = nullptr;
    const SString EDITOR_CAMERA_NAME = "_EditorSceneCamera";
};



#endif //EDITORSCENEMANAGER_H