//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <map>
#include <vector>
#include "core/object/object.h"

class IDefferedLoadableAsset;
class MAsset;

namespace pugi {
    class xml_document;
}

class MAssetManager : public MObject
{
public:
    static MAssetManager* getInstance();
    void refresh();
    void cleanup();
    template <typename T>
    T* getAsset(SString path) {
        static_assert(std::is_base_of<MAsset, T>::value,"T must inherit from MAsset");

        if(assetMap.contains(path)) {
            return dynamic_cast<T*>(assetMap[path]);
        }

        return nullptr;
    }

private:
    std::vector<SString> ASSET_SEARCH_PATHS = {"assets/", "meteor_assets/engine_assets/"};
    std::map<SString, MAsset*> assetMap;
    std::vector<IDefferedLoadableAsset*> defferedLoadableAssetList;
    static MAssetManager* instance;
    void loadAssetRecursive(SString path);
    bool loadAsset(SString path);
    void addToDeferedLoadableAssetList(IDefferedLoadableAsset* asset);
};

#endif //ASSETMANAGER_H
