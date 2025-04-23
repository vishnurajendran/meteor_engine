//
// Created by ssj5v on 22-01-2025.
//

#ifndef EDITORSCENEMANAGER_H
#define EDITORSCENEMANAGER_H
#include "core/engine/scene/scenemanager.h"


class MCameraEntity;

class MEditorSceneManager : public MSceneManager {
public:
    bool loadEmptyScene() override;
    bool loadScene(const SString& path) override;
    bool closeActiveScene() override;
    MCameraEntity* getEditorSceneCamera() const;
private:
    void createEditorSceneCamera();
private:
    MCameraEntity* editorSceneCamera = nullptr;
};



#endif //EDITORSCENEMANAGER_H
