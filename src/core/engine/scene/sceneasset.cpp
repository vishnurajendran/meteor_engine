//
// Created by ssj5v on 27-09-2024.
//

#include "sceneasset.h"

#include "core/meteor_utils.h"

MSceneAsset::MSceneAsset(SString path) : MAsset(path){
}

MSceneAsset::~MSceneAsset() {
    delete sceneHierarchy;
}

bool MSceneAsset::loadFromPath(const SString& path) {
    SString dataTxt;
    valid = false;
    if(FileIO::readFile(path, dataTxt)) {
        sceneHierarchy = new pugi::xml_document();
        sceneHierarchy->load_string(dataTxt.c_str());
        valid = true;
    }
    return valid;
}

pugi::xml_document * MSceneAsset::getSceneHierarchy() {
    return sceneHierarchy;
}