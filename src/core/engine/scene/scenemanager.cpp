//
// Created by ssj5v on 27-09-2024.
//

#include "scenemanager.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/entities/spatial/spatial.h"
#include "core/engine/lighting/dynamiclights/dynamic_light.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "functional"
#include "scene.h"
#include "sceneasset.h"
#include "serialisation/scene_serialiser.h"

MSceneManager* MSceneManager::sceneManagerInstance = nullptr;

bool MSceneManager::closeActiveScene() {
    if (activeScene == nullptr)
        return false;

    // onClose() destroys all alive entities (calling onExit() on each, which
    // unregisters lights, removes skybox draw calls, etc.) and then calls
    // destroyMarked(). No need to repeat that work here.
    activeScene->close();

    // Belt-and-braces: explicitly reset the light manager in case any
    // light forgot to call unregisterLight in its onExit.
    auto* lightMgr = MLightSystemManager::getInstance();
    for (auto* dl : std::vector<MDynamicLight*>(lightMgr->getDynamicLights()))
        lightMgr->unregisterLight(dl);

    activeScene = nullptr;
    return true;
}

MSceneManager::~MSceneManager()
{
    //close any open scenes safely.
    closeActiveScene();
}

bool MSceneManager::loadEmptyScene() {
    if (activeScene != nullptr)
        closeActiveScene();
    activeScene = new MScene();
    currentScenePath = SString();
    informSceneLoadCallbackListeners(activeScene);
    return true;
}

bool MSceneManager::loadScene(const SString& path) {
    if (path.empty()) {
        MLOG(STR("MSceneManager:: Loading Empty Scene"));
        loadEmptyScene();
        return false;
    }

    loadEmptyScene();
    auto asset = MAssetManager::getInstance()->getAsset<MSceneAsset>(path);
    if(asset == nullptr) {
        MLOG(STR("MSceneManager:: Failed to load Scene Asset"));
        return false;
    }

    if(!asset->isValid()) {
        MLOG(STR("Invalid Asset"));
        return false;
    }

   if (MSceneSerializer::load(path, activeScene))
   {
       currentScenePath= asset->getPath();
       MLOG("MSceneManager:: Scene Loaded");
       return true;
   };

    MLOG(STR("MSceneManager:: Failed to load Scene"));
    return false;
}

void MSceneManager::update(float deltaTime)
{
    if (activeScene == nullptr)
        return;

    activeScene->update(deltaTime);
}
SString MSceneManager::registerOnLoadCallback(std::function<void(MScene*)> callback)
{
    auto id = SGuid::newGUID();
    sceneLoadCallbackListeners[id] = callback;
    return id;
}
void MSceneManager::deregisterOnLoadCallback(SString callbackId)
{
    if (sceneLoadCallbackListeners.contains(callbackId))
        sceneLoadCallbackListeners.erase(callbackId);
}

void MSceneManager::registerSceneManager(MSceneManager* instance)
{
    if (instance == nullptr)
        return;

    //close previous manager
    if (sceneManagerInstance != nullptr)
        delete sceneManagerInstance;

    sceneManagerInstance = instance;
    instance->init();
}

MSceneManager* MSceneManager::getSceneManagerInstance() { return sceneManagerInstance; }

void MSceneManager::informSceneLoadCallbackListeners(MScene* scene)
{
    for (auto listener : sceneLoadCallbackListeners)
    {
        listener.second(scene);
    }
}
