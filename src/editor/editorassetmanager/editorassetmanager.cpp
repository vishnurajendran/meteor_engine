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
    failedAssetPaths.clear();   // allow re-import after user fixes broken files

    MAssetManager::refresh();

    buildAssetTree();
    registerAssetsWithWatcher();

    // Initialise the thumbnail renderer on the first refresh (GL context is
    // guaranteed to be active by the time the editor calls refresh()).
    if (!thumbnailRenderer.isInitialised())
        thumbnailRenderer.init();

    totalHotReloadCount = 0;
    lastDeltaScanTime   = std::chrono::steady_clock::now();
}

int MEditorAssetManager::tickHotReload()
{
    // Content-change hot reload — 1s poll inside the watcher.
    const int count = hotReloadWatcher.tick();
    if (count > 0)
    {
        totalHotReloadCount += count;
        // Evict thumbnails for assets that were just reloaded.
        // The watcher doesn't tell us which paths changed, so we rely on
        // the asset window re-requesting thumbnails after eviction.
        // A more surgical approach: add an onReload callback to SWatchEntry.
    }

    // New / deleted files — 5s poll.
    deltaRefresh();

    // Thumbnail generation — one per frame.
    thumbnailRenderer.tick(thumbnailCache);

    return count;
}

sf::Texture* MEditorAssetManager::getThumbnail(MAsset* asset)
{
    if (!asset) return nullptr;
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

            // Skip paths that have already failed — don't spam the log.
            // A full refresh() clears this set so fixed files are retried.
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
                // Record permanently — stops the delta scan retrying a broken
                // file every 5 seconds and spamming the log.
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

// ── Open asset ────────────────────────────────────────────────────────────────

void MEditorAssetManager::openAsset(MAsset* asset)
{
    if (asset == nullptr) return;
    MLOG(SString::format("EditorAssetManager:: Open Asset {0}", asset->getName()));
    if (!asset->openAsset())
    {
        auto cmd = STR("\"") + asset->getFullPath() + STR("\"");
        system(cmd.c_str());
    }
}