//
// Created by ssj5v on 27-09-2024.
//

#include "sceneasset.h"

#include "core/meteor_utils.h"
#include "scenemanager.h"

MSceneAsset::MSceneAsset(const SString& path) : MAsset(path){
    name = "SceneAsset";
    valid = loadFromPath(path);
}

MSceneAsset::~MSceneAsset()
{
    delete sceneHierarchy;
}

bool MSceneAsset::openAsset()
{
    return MSceneManager::getSceneManagerInstance()->loadScene(path);
}

bool MSceneAsset::loadFromPath(const SString& path) {

    if (!FileIO::hasFile(path)) return false;

    SString dataTxt;
    auto res = false;
    if(FileIO::readFile(path, dataTxt)) {
        sceneHierarchy = new pugi::xml_document();
        sceneHierarchy->load_string(dataTxt.c_str());
        res = true;
    }
    return res;
}

pugi::xml_document * MSceneAsset::getSceneHierarchy() {
    return sceneHierarchy;
}
