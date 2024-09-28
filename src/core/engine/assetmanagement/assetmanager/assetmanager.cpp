//
// Created by ssj5v on 27-09-2024.
//

#include <iostream>
#include <filesystem>
#include "assetmanager.h"
#include <core/meteor_utils.h>
#include <core/engine/assetmanagement/asset/asset.h>

#include "assetimporter.h"
#include "core/engine/assetmanagement/textasset/textasset.h"
#include "core/engine/scene/sceneasset.h"

MAssetManager* MAssetManager::instance;

MAssetManager * MAssetManager::getInstance() {
    if(instance == nullptr)
        instance = new MAssetManager();

    return instance;
}

void MAssetManager::refresh() {
    cleanup();
    loadAssetRecursive("assets/");
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
    SString extension = FileIO::getFileExtension(path);
    for(auto importer : *MAssetImporter::getImporters()) {
        if(importer && importer->canImport(extension)) {
            auto asset = importer->importAsset(path);
            if(asset) {
                assetMap[path] = asset;
                return true;
            }
        }
    }
    MWARN(STR("No compatible importer found for " + path.str()));
    return false;
}
