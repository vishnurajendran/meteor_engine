//
// Created by ssj5v on 27-09-2024.
//

#include "assetmanager.h"
#include <core/engine/assetmanagement/asset/asset.h>
#include <core/meteor_utils.h>
#include <algorithm>
#include "assetimporter.h"
#include "core/engine/assetmanagement/source/asset_sources.h"
#include "core/engine/assetmanagement/source/directory_asset_source.h"
#include "core/engine/assetmanagement/source/routed_asset_source.h"
#include "core/utils/meteor_paths.h"
#include "core/utils/guid.h"
#include "pugixml.hpp"

// ---------------------------------------------------------------------------
// Source
// ---------------------------------------------------------------------------

void MAssetManager::setAssetSource(std::shared_ptr<IAssetSource> newSource)
{
    assetSource = std::move(newSource);
    MAssetSources::setActive(assetSource);
}

std::shared_ptr<IAssetSource> MAssetManager::createProjectSource(const std::vector<SString>& projectAssetFolders)
{
    if (SMeteorPaths::isEngineProject())
    {
        std::vector<SString> searchPaths = projectAssetFolders;
        searchPaths.push_back(ENGINE_ASSET_SEARCH_PATH);
        return std::make_shared<MDirectoryAssetSource>(PROJECT_PATH(), searchPaths);
    }

    auto project = std::make_shared<MDirectoryAssetSource>(PROJECT_PATH(), projectAssetFolders);
    auto engine  = std::make_shared<MDirectoryAssetSource>(ENGINE_PATH(),
                                                          std::vector<SString>{ ENGINE_ASSET_SEARCH_PATH });

    auto routed = std::make_shared<MRoutedAssetSource>(project);
    routed->addRoute(ENGINE_ASSET_ROUTE, engine);
    return routed;
}

std::shared_ptr<IAssetSource> MAssetManager::createDefaultSource()
{
    return createProjectSource(DEFAULT_PROJECT_ASSET_FOLDERS);
}

IAssetSource& MAssetManager::source()
{
    if (!assetSource)
        setAssetSource(createDefaultSource());
    return *assetSource;
}

// ---------------------------------------------------------------------------
// Refresh / cleanup
// ---------------------------------------------------------------------------

void MAssetManager::refresh() {
    cleanup();
    defferedLoadableAssetList.clear(); // redundant but better to have.
    MLOG(STR("AssetManager:: Starting Refresh from ") + source().getDebugName());

    int assetCount = 0;
    source().enumerate([&](const SAssetSourceEntry& entry)
    {
        if (entry.isDirectory || MAssetPath::isIgnored(entry.path))
            return;
        if (loadAsset(entry.path))
            ++assetCount;
    });

    MLOG(STR("AssetManager:: Loading Deferred Assets"));
    for (auto asset: defferedLoadableAssetList) {

        if(asset)
            asset->deferredAssetLoad(false);
    }
    MLOG(SString::format("AssetManager:: Refresh Completed ({0} assets)", SString::fromInt(assetCount)));
}

void MAssetManager::cleanup() {

    MLOG("MAssetManager:: Cleanup started");
    for(auto [key, asset] : assetMap) {
        if(asset != nullptr)
            delete asset;
    }

    assetMap.clear();
    assetMapByAssetId.clear();
    // Clear the deferred list so stale pointers from deleted assets are not
    // called on the next refresh(). Failing to clear this causes defferedAssetLoad()
    // to be called through dangling pointers, corrupting the heap.
    defferedLoadableAssetList.clear();
    MLOG("MAssetManager:: Cleanup completed");
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

bool MAssetManager::loadAsset(SString path) {
    path = MAssetPath::normalize(path);
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

        auto* asset = importer->importAsset(path, metaDataDoc);
        if (asset == nullptr)
            continue;

        if (asset->hasDeferredLoad())
            addToDeferredLoadableAssetList(asset);  // MAsset* - virtual dispatch is correct, no cast

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

void MAssetManager::addToDeferredLoadableAssetList(MAsset* asset)
{
    defferedLoadableAssetList.push_back(asset);
}

bool MAssetManager::unregisterAsset(const SString& path)
{
    auto it = assetMap.find(path);
    if (it == assetMap.end())
        return false;

    MAsset* asset = it->second;
    assetMap.erase(it);

    if (asset)
    {
        const SString id = asset->getAssetId();
        if (!id.empty())
            assetMapByAssetId.erase(id);

        // The old delete paths left the pointer in this list, so a later
        // deferred pass or dependency rebuild would call into freed memory.
        std::erase(defferedLoadableAssetList, asset);
        delete asset;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Reload with dependents
// ---------------------------------------------------------------------------

bool MAssetManager::reloadAsset(MAsset* asset)
{
    if (!asset)
        return false;

    const bool ok = asset->requestReload();

    const SString& reloadedPath = asset->getPath();
    for (auto* dependent : defferedLoadableAssetList)
    {
        if (!dependent || dependent == asset)
            continue;
        if (!dependent->dependsOn(reloadedPath))
            continue;

        MLOG(STR("AssetManager:: Rebuilding dependent ") + dependent->getPath());
        dependent->deferredAssetLoad(true);
    }
    return ok;
}

// ---------------------------------------------------------------------------
// Meta data
// ---------------------------------------------------------------------------

void MAssetManager::createMetaFile(const SString& filePath)
{
    auto* writable = source().asWritable();
    if (!writable)
        return;   // packages carry their meta in the TOC

    pugi::xml_document metaDoc;
    metaDoc.append_child(ASSET_FILE_TAG.c_str()).append_attribute(ASSET_ID_ATTRIB.c_str()).set_value(SGuid::newGUID().c_str());
    writable->writeMeta(filePath, metaDoc);
}

bool MAssetManager::loadMetaData(const SString& path, pugi::xml_document& metaData)
{
    return source().readMeta(path, metaData);
}

bool MAssetManager::hasMetaData(const SString& path)
{
    return source().hasMeta(path);
}
