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

/// The Asset Manager is the central repository for all assets in the engine.
/// Any asset reference needed will be retrieved through this class instance.
/// At any point during engine runtime, there is only one instance of the asset-manager.
class MAssetManager : public MObject
{
    DEFINE_OBJECT_CLASS(MAssetManager)
private:
    const SString META_FILE_EXTENSION = "meta";
    const SString ASSET_FILE_TAG = "asset_id";
    const SString ASSET_ID_ATTRIB = "id";
public:

    /// Get instance of asset-manager singleton
    static MAssetManager* getInstance();
    static void registerAssetManagerInstance(MAssetManager* instance);

    /// Refresh the asset-manager
    virtual void refresh();
    /// Cleanup asset-manager
    virtual void cleanup();

    /// Get an asset using relative path.
    template <typename T>
    T* getAsset(SString path) {
        static_assert(std::is_base_of<MAsset, T>::value,"T must inherit from MAsset");

        if(assetMap.contains(path)) {
            return dynamic_cast<T*>(assetMap[path]);
        }

        return nullptr;
    }

    /// Get an asset using Asset-Id
    template <typename T>
    T* getAssetById(const SString& assetId) {
        static_assert(std::is_base_of_v<MAsset, T>,"T must inherit from MAsset");

        if(assetMapByAssetId.contains(assetId)) {
            return dynamic_cast<T*>(assetMapByAssetId[assetId]);
        }

        return nullptr;
    }

protected:
    std::vector<SString> ASSET_SEARCH_PATHS = {"assets/", "meteor_assets/engine_assets/"};
    std::map<SString, MAsset*> assetMap;
    std::map<SString, MAsset*> assetMapByAssetId;

    std::vector<MAsset*> defferedLoadableAssetList;  // MAsset* — no cast needed, deferredAssetLoad is virtual on MAsset

    static MAssetManager* managerInstance;
    virtual void loadAssetRecursive(SString path);
    virtual bool loadAsset(SString path);
    virtual void addToDeferredLoadableAssetList(MAsset* asset);

    void createMetaFile(const SString& filePath);
    bool loadMetaData(const SString& path, pugi::xml_document& metaData);
    bool hasMetaData(const SString& path);
};

#endif //ASSETMANAGER_H