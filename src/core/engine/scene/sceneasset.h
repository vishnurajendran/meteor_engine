//
// Created by ssj5v on 27-09-2024.
//

#ifndef SCENEASSET_H
#define SCENEASSET_H

#include "core/engine/assetmanagement/asset/asset.h"
#include "pugixml.hpp"

class MSceneAsset : public MAsset {
    DEFINE_OBJECT_SUBCLASS(MSceneAsset)
private:
    // Owned by value: nothing to leak on reload, nothing to double-free
    // when the initial load fails.
    pugi::xml_document sceneHierarchy;
public:

    explicit MSceneAsset(const SString& path);
    ~MSceneAsset() override = default;

    bool openAsset() override;

    // nullptr when the scene failed to load.
    pugi::xml_document* getSceneHierarchy();

    bool requestReload() override { valid = loadFromSource(); return valid; }

private:
    bool loadFromSource();
};

#endif //SCENEASSET_H
