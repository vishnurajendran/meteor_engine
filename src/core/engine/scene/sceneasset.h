//
// Created by ssj5v on 27-09-2024.
//

#ifndef SCENEASSET_H
#define SCENEASSET_H

#include "core/engine/assetmanagement/asset/asset.h"
#include "pugixml.hpp"

class MSceneAsset : public MAsset {
private:
    pugi::xml_document* sceneHierarchy;
public:
    MSceneAsset(const SString& path);
    ~MSceneAsset();
    pugi::xml_document* getSceneHierarchy();
private:
    bool loadFromPath(const SString& path);
};

#endif //SCENEASSET_H
