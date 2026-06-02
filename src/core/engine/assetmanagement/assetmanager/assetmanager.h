//
// Created by ssj5v on 27-09-2024.
//
#pragma once
#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <map>
#include <vector>

#include "asset_manager_subsystem.h"
#include "core/engine/assetmanagement/asset/asset_handle.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "core/object/object.h"

class IDefferedLoadableAsset;
class MAsset;

namespace pugi {
    class xml_document;
}

class MAssetManager : public MObject, public IAssetManagerSubsystem
{
    DEFINE_OBJECT_SUBCLASS(MAssetManager)
public:
    virtual void refresh();
    virtual void init() override { };
    virtual void cleanup() override;

protected:
    std::vector<SString> ASSET_SEARCH_PATHS = {"assets/", "meteor_assets/engine_assets/"};
    std::vector<MAsset*> defferedLoadableAssetList;  // MAsset* - no cast needed, deferredAssetLoad is virtual on MAsset

    static MAssetManager* managerInstance;
    virtual void loadAssetRecursive(SString path);
    virtual bool loadAsset(SString path);
    virtual void addToDeferredLoadableAssetList(MAsset* asset);

    void createMetaFile(const SString& filePath);
    bool loadMetaData(const SString& path, pugi::xml_document& metaData);
    bool hasMetaData(const SString& path);
};

#endif //ASSETMANAGER_H