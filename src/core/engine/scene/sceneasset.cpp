//
// Created by ssj5v on 27-09-2024.
//

#include "sceneasset.h"

#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/meteor_utils.h"
#include "core/utils/logger.h"
#include "scenemanager.h"

MSceneAsset::MSceneAsset(const SString& path) : MAsset(path){
    name = "SceneAsset";
    valid = loadFromSource();
}

bool MSceneAsset::openAsset()
{
    return MSceneManager::getSceneManagerInstance()->loadScene(path);
}

bool MSceneAsset::loadFromSource() {
    sceneHierarchy.reset();

    std::vector<uint8_t> bytes;
    if (!MAssetSources::getActive()->readBytes(path, bytes))
        return false;

    const auto res = sceneHierarchy.load_buffer(bytes.data(), bytes.size());
    if (res.status != pugi::status_ok)
    {
        MERROR(SString("MSceneAsset:: failed to parse ") + path + " - " + res.description());
        return false;
    }
    return true;
}

pugi::xml_document * MSceneAsset::getSceneHierarchy() {
    return valid ? &sceneHierarchy : nullptr;
}
