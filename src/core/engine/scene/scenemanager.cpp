//
// Created by ssj5v on 27-09-2024.
//

#include "scenemanager.h"
#include "scene.h"
#include "sceneasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"

MScene* MSceneManager::activeScene = nullptr;

bool MSceneManager::closeActiveScene() {
    if (activeScene == nullptr)
        return false;


    activeScene->onClose();
    activeScene = nullptr;
    return true;
}

bool MSceneManager::loadEmptyScene() {
    if (activeScene != nullptr)
        closeActiveScene();
    activeScene = new MScene();
    return true;
}

bool MSceneManager::loadScene(const SString& path) {
    if (path.empty()) {
        MLOG(STR("Loading Empty Scene"));
        loadEmptyScene();
        return false;
    }

    loadEmptyScene();
    auto asset = MAssetManager::getInstance()->getAsset<MSceneAsset>(path);
    if(asset == nullptr) {
        return false;
    }

    if(!asset->isValid()) {
        MLOG(STR("Invalid Asset"));
        return false;
    }

    if (activeScene->tryParse(asset->getSceneHierarchy())) {
        MLOG(STR("Scene Loaded Complete"));
        return true;
    }

    return false;
}

void MSceneManager::update(float deltaTime) {
    if (activeScene == nullptr)
        return;

    activeScene->update(deltaTime);
}