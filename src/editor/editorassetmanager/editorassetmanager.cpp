//
// Created by ssj5v on 11-05-2025.
//

#include "editorassetmanager.h"

#include <filesystem>
#include <queue>

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"
#include "core/utils/logger.h"
#include "core/meteor_utils.h"

void MEditorAssetManager::refresh()
{
    hotReloadWatcher.unwatchAll();
    thumbnailCache.evictAll();
    thumbnailRenderer.clearQueue();
    failedAssetPaths.clear();

    MAssetManager::refresh();

    buildAssetTree();
    registerAssetsWithWatcher();

    if (!thumbnailRenderer.isInitialised())
        thumbnailRenderer.init();

    totalHotReloadCount = 0;
    lastDeltaScanTime   = std::chrono::steady_clock::now();
}

int MEditorAssetManager::tickHotReload()
{
    const int count = hotReloadWatcher.tick();
    if (count > 0)
    {
        totalHotReloadCount += count;
    }

    deltaRefresh();

    thumbnailRenderer.tick(thumbnailCache);

    return count;
}

sf::Texture* MEditorAssetManager::getThumbnail(MAsset* asset)
{
    if (!asset) return nullptr;

    // Dirty asset → evict stale thumbnail so it gets re-rendered
    // on the next requestThumbnail() call.
    if (asset->isDirty() && thumbnailCache.has(asset->getAssetId()))
    {
        thumbnailCache.evict(asset->getAssetId());
        return nullptr;
    }

    return thumbnailCache.get(asset->getAssetId());
}

void MEditorAssetManager::requestThumbnail(MAsset* asset)
{
    if (!thumbnailRenderer.isInitialised()) return;
    thumbnailRenderer.requestThumbnail(asset, thumbnailCache);
}

void MEditorAssetManager::deltaRefresh()
{
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(now - lastDeltaScanTime).count()
            < DELTA_SCAN_INTERVAL_SECONDS)
        return;

    lastDeltaScanTime = now;
    bool changed = false;

    // Pass 1: new files
    for (const auto& searchPath : ASSET_SEARCH_PATHS)
    {
        std::filesystem::path dir(searchPath.str());
        if (!std::filesystem::exists(dir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (!entry.is_regular_file()) continue;
            auto fileName = STR(entry.path().filename().string());
            if (fileName[0] == '~') continue;

            SString path = STR(entry.path().string());
            path.replace("\\", "/");
            if (FileIO::getFileExtension(path) == STR("meta")) continue;
            if (assetMap.contains(path)) continue;

            if (failedAssetPaths.contains(path)) continue;

            const size_t deferredBefore = defferedLoadableAssetList.size();
            if (loadAsset(path))
            {
                MLOG(STR("EditorAssetManager:: Delta loaded: ") + path);
                hotReloadWatcher.watchAsset(assetMap[path]);
                changed = true;
                for (size_t i = deferredBefore; i < defferedLoadableAssetList.size(); ++i)
                    if (defferedLoadableAssetList[i])
                        defferedLoadableAssetList[i]->deferredAssetLoad(false);
            }
            else
            {
                failedAssetPaths.insert(path);
            }
        }
    }

    // Pass 2: deleted files
    std::vector<SString> toRemove;
    for (const auto& [path, asset] : assetMap)
        if (!FileIO::hasFile(path))
            toRemove.push_back(path);

    for (const auto& path : toRemove)
    {
        MLOG(STR("EditorAssetManager:: Delta unloading: ") + path);
        hotReloadWatcher.unwatchAsset(path);
        MAsset* asset = assetMap[path];
        if (asset)
        {
            thumbnailCache.evict(asset->getAssetId());
            const SString id = asset->getAssetId();
            if (!id.empty()) assetMapByAssetId.erase(id);
            delete asset;
        }
        assetMap.erase(path);
        changed = true;
    }

    if (changed)
    {
        buildAssetTree();
        MLOG(STR("EditorAssetManager:: Delta refresh complete"));
    }
}

void MEditorAssetManager::registerAssetsWithWatcher()
{
    for (auto& [path, asset] : assetMap)
        hotReloadWatcher.watchAsset(asset);
}

void MEditorAssetManager::buildAssetTree()
{
    if (assetsTreeRoot != nullptr)
        delete assetsTreeRoot;

    assetsTreeRoot = new SAssetDirectoryNode();
    assetsTreeRoot->nodeName    = "Asset Root";
    assetsTreeRoot->nodePath    = "Root";
    assetsTreeRoot->isDirectory = true;

    for (auto kv : assetMap)
    {
        std::queue<SString> pathQueue;
        for (const auto& sComp : kv.first.split("/"))
            pathQueue.push(sComp);
        recursiveBuildAssetTree(pathQueue, assetsTreeRoot, kv.second, "");
    }
}

void MEditorAssetManager::recursiveBuildAssetTree(std::queue<SString>& pathQueue,
                                                   SAssetDirectoryNode* parentNode,
                                                   MAsset* asset,
                                                   SString parsedPath)
{
    if (pathQueue.empty()) return;

    auto currentPathNodeName = pathQueue.front();
    pathQueue.pop();
    if (!parsedPath.empty()) parsedPath += "/";
    parsedPath += currentPathNodeName;

    auto child = parentNode->getChild(currentPathNodeName);
    if (child != nullptr)
    {
        if (child->isDirectory)
            recursiveBuildAssetTree(pathQueue, child, asset, parsedPath);
        return;
    }

    auto* node = new SAssetDirectoryNode();
    parentNode->childrenNodes.push_back(node);
    node->isDirectory = !pathQueue.empty();
    node->nodeName    = currentPathNodeName;
    node->nodePath    = parsedPath;

    if (!node->isDirectory)
    {
        node->assetReference = asset;
        return;
    }

    recursiveBuildAssetTree(pathQueue, node, asset, parsedPath);
}

void MEditorAssetManager::openAsset(MAsset* asset)
{
    if (asset == nullptr)
        return;
    MLOG(SString::format("EditorAssetManager:: Open Asset {0}", asset->getName()));
    if (!asset->openAsset())
    {
        auto cmd = STR("\"") + asset->getFullPath() + STR("\"");
        system(cmd.c_str());
    }
}


int MEditorAssetManager::saveDirtyAssets()
{
    int savedCount = 0;
    for (auto& [path, asset] : assetMap)
    {
        if (!asset || !asset->isDirty()) continue;
        if (asset->save())
        {
            asset->clearDirty();
            ++savedCount;
            MLOG(SString::format("Auto-saved dirty asset: {0}", path));
        }
    }
    return savedCount;
}