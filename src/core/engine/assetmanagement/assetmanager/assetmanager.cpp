//
// Created by ssj5v on 27-09-2024.
//

#include <iostream>
#include <filesystem>
#include "assetmanager.h"
#include <core/meteor_utils.h>
#include <core/engine/assetmanagement/asset/asset.h>
#include "assetimporter.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"


MAssetManager* MAssetManager::managerInstance = nullptr;

MAssetManager* MAssetManager::getInstance()
{
    if (!managerInstance)
    {
        MERROR("Asset Manager not registered!!");
    }
    return managerInstance;
}

void MAssetManager::registerAssetManagerInstance(MAssetManager* instance)
{
    if (managerInstance != nullptr)
        delete managerInstance;

    managerInstance = instance;
}

void MAssetManager::refresh() {
    cleanup();
    for (auto path : ASSET_SEARCH_PATHS)
    {
        loadAssetRecursive(path);
    }

    for (auto asset: defferedLoadableAssetList) {

        if(asset)
            asset->defferedAssetLoad(false);
    }
}

void MAssetManager::cleanup() {
    for(auto [key, asset] : assetMap) {
        if(asset != nullptr)
            delete asset;
    }
}

void MAssetManager::loadAssetRecursive(SString path) {
    std::filesystem::path directory_path(path.str());
    // Iterate over the directory and its subdirectories
    int assetCount=0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            auto fileName = STR(entry.path().filename().string());
            //Ignore symbol, all files starting with ~ will be ignored by the asset manager refresh.
            if (fileName[0] == '~')
                continue;
            SString path = entry.path().string();
            path.replace("\\", "/");
            auto res = loadAsset(path);
            if(res) {
                assetCount+=1;
            }
        }
    }
    MLOG(STR("AssetManager:: Refresh Completed, Loaded " + std::to_string(assetCount) + " Assets"));
}

bool MAssetManager::loadAsset(SString path) {
    const SString extension = FileIO::getFileExtension(path);
    for(const auto importer : *MAssetImporter::getImporters()) {
        if(!importer)
            continue;
        if(!importer->canImport(extension))
            continue;
        const auto asset = importer->importAsset(path);
        if(asset == nullptr)
            continue;
        assetMap[path] = asset;
        return true;
    }
    MWARN(STR("No compatible importer found for " + path.str()));
    return false;
}

void MAssetManager::addToDeferedLoadableAssetList(IDefferedLoadableAsset *asset) {
    defferedLoadableAssetList.push_back(asset);
}