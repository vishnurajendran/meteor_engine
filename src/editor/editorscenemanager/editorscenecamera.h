//
// Created by ssj5v on 24-04-2025.
//

#ifndef EDITORSCENECAMERA_H
#define EDITORSCENECAMERA_H
#include "core/engine/camera/camera_spatial_entity.h"


class MEditorSettings;
class MEditorSceneCameraEntity : public MCameraEntity {
    DEFINE_SPATIAL_CLASS(MEditorSceneCameraEntity)
private:
    const SString EDITOR_CAMERA_NAME = "_EditorSceneCamera";
public:
    void onCreate() override;
    void onUpdate(float deltaTime) override;
    MEditorSceneCameraEntity() = default;
    ~MEditorSceneCameraEntity() override;

private:
    static SVector3 lastPosition;
    static SQuaternion lastRotation;
    MEditorSettings* settingsRef;
};

#endif //EDITORSCENECAMERA_H