//
// Created by ssj5v on 24-04-2025.
//

#ifndef EDITORSCENECAMERA_H
#define EDITORSCENECAMERA_H
#include "core/engine/camera/camera.h"


class MEditorSceneCameraEntity : public MCameraEntity {
private:
    const SString EDITOR_CAMERA_NAME = "_EditorSceneCamera";
public:
    MEditorSceneCameraEntity();
};



#endif //EDITORSCENECAMERA_H
