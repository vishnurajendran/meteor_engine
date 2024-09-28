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
    MSceneAsset(SString path);
    ~MSceneAsset();
    bool loadFromPath(const SString& path) override;
    pugi::xml_document* getSceneHierarchy();
};

#endif //SCENEASSET_H
