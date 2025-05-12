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
    static void registerAssetManagerInstance(MAssetManager* instance);
    virtual void refresh();
    virtual void cleanup();
    template <typename T>
    T* getAsset(SString path) {
        static_assert(std::is_base_of<MAsset, T>::value,"T must inherit from MAsset");

        if(assetMap.contains(path)) {
            return dynamic_cast<T*>(assetMap[path]);
        }

        return nullptr;
    }

protected:
    std::vector<SString> ASSET_SEARCH_PATHS = {"assets/", "meteor_assets/engine_assets/"};
    std::map<SString, MAsset*> assetMap;
    std::vector<IDefferedLoadableAsset*> defferedLoadableAssetList;
    static MAssetManager* managerInstance;
    virtual void loadAssetRecursive(SString path);
    virtual bool loadAsset(SString path);
    virtual void addToDeferedLoadableAssetList(IDefferedLoadableAsset* asset);
};

#endif //ASSETMANAGER_H
