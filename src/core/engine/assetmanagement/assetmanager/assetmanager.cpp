//
// Created by ssj5v on 27-09-2024.
//

#include "assetmanager.h"
#include <core/engine/assetmanagement/asset/asset.h>
#include <core/meteor_utils.h>
#include <filesystem>
#include <iostream>
#include "assetimporter.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"
#include "pugixml.hpp"
#include "core/utils/guid.h"


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
    MLOG(STR("AssetManager:: Starting Refresh"));
    for (auto path : ASSET_SEARCH_PATHS)
    {
        loadAssetRecursive(path);
    }
    for (auto asset: defferedLoadableAssetList) {

        if(asset)
            asset->defferedAssetLoad(false);
    }
    MLOG(STR("AssetManager:: Refresh Completed"));
}

void MAssetManager::cleanup() {
    for(auto [key, asset] : assetMap) {
        if(asset != nullptr)
            delete asset;
    }

    assetMap.clear();
    assetMapByAssetId.clear();
}

void MAssetManager::loadAssetRecursive(SString path) {
    std::filesystem::path directory_path(path.str());
    // Iterate over the directory and its subdirectories
    int assetCount=0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            auto fileName = STR(entry.path().filename().string());
            const SString extension = FileIO::getFileExtension(entry.path().string());
            //Ignore symbol, all files starting with ~ will be ignored by the asset manager refresh.
            if (fileName[0] == '~')
                continue;
            if (extension == META_FILE_EXTENSION)
                continue;
            SString path = entry.path().string();
            path.replace("\\", "/");
            auto res = loadAsset(path);
            if(res) {
                assetCount+=1;
            }
        }
    }
}

bool MAssetManager::loadAsset(SString path) {
    const SString extension = FileIO::getFileExtension(path);
    if (!hasMetaData(path))
        createMetaFile(path);

    pugi::xml_document metaDataDoc;
    loadMetaData(path, metaDataDoc);

    for(const auto importer : *MAssetImporter::getImporters()) {

        if(!importer)
            continue;

        if(!importer->canImport(extension))
            continue;

        const auto asset = importer->importAsset(path, metaDataDoc);
        if (asset == nullptr)
            continue;

        assetMap[path] = asset;
        const auto tag = metaDataDoc.child(ASSET_FILE_TAG.c_str());
        const auto attrib = tag.attribute(ASSET_ID_ATTRIB.c_str());
        if (tag && attrib)
        {
            asset->internal_SetAssetId(attrib.value());
            assetMapByAssetId[attrib.value()] = asset;
        }

        return true;
    }
    MWARN(STR("No compatible importer found for " + path.str()));
    return false;
}

void MAssetManager::addToDeferedLoadableAssetList(IDefferedLoadableAsset* asset)
{
    defferedLoadableAssetList.push_back(asset);
}

void MAssetManager::createMetaFile(const SString& filePath)
{
    pugi::xml_document metaDoc;
    metaDoc.append_child(ASSET_FILE_TAG.c_str()).append_attribute(ASSET_ID_ATTRIB.c_str()).set_value(SGuid::newGUID().c_str());
    auto newFilePath = filePath +"."+ META_FILE_EXTENSION;
    std::ostringstream oss;
    metaDoc.save(oss);
    SString str = oss.str();
    FileIO::writeFile(newFilePath, str);
}

bool MAssetManager::loadMetaData(const SString& path, pugi::xml_document& metaData)
{
    if (!hasMetaData(path))
        return false;

    SString data;
    if (FileIO::readFile(path+"."+ META_FILE_EXTENSION, data))
    {
        metaData.load_string(data.c_str());
        return true;
    }
    return false;
}


bool MAssetManager::hasMetaData(const SString& path)
{
    return FileIO::hasFile(path+"."+ META_FILE_EXTENSION);
}
