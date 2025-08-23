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
private:
    const SString META_FILE_EXTENSION = "meta";
    const SString ASSET_FILE_TAG = "asset_id";
    const SString ASSET_ID_ATTRIB = "id";
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

    template <typename T>
    T* getAssetById(SString path) {
        static_assert(std::is_base_of<MAsset, T>::value,"T must inherit from MAsset");

        if(assetMapByAssetId.contains(path)) {
            return dynamic_cast<T*>(assetMapByAssetId[path]);
        }

        return nullptr;
    }

protected:
    std::vector<SString> ASSET_SEARCH_PATHS = {"assets/", "meteor_assets/engine_assets/"};
    std::map<SString, MAsset*> assetMap;
    std::map<SString, MAsset*> assetMapByAssetId;

    std::vector<IDefferedLoadableAsset*> defferedLoadableAssetList;

    static MAssetManager* managerInstance;
    virtual void loadAssetRecursive(SString path);
    virtual bool loadAsset(SString path);
    virtual void addToDeferedLoadableAssetList(IDefferedLoadableAsset* asset);

    void createMetaFile(const SString& filePath);
    bool loadMetaData(const SString& path, pugi::xml_document& metaData);
    bool hasMetaData(const SString& path);
};

#endif //ASSETMANAGER_H
