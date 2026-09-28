//
// Created by ssj5v on 11-05-2025.
//
// Intended location: src/editor/editorassetmanager/editorassetmanager.cpp
//
// All project file access goes through the active asset source
// (MDirectoryAssetSource in the editor). Template files are read by
// MAssetTemplateRegistry from the editor's resources folder.
//

#include "editorassetmanager.h"

#include <queue>

#include "builtin_asset_templates.h"

#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/asset/defferedloadableasset.h"
#include "core/engine/assetmanagement/source/directory_asset_source.h"
#include "core/engine/assetmanagement/source/routed_asset_source.h"
#include "core/utils/meteor_paths.h"
#include "core/engine/scripting/interface/script_asset.h"
#include "editor/editor_utils/editor_utility.h"
#include "core/utils/logger.h"
#include "core/meteor_utils.h"

// ---------------------------------------------------------------------------
// Source access
// ---------------------------------------------------------------------------

// Every on-disk directory behind `source`: itself, or the parts of a routed
// source. Package or other non-directory sources are not watched.
static std::vector<std::shared_ptr<const MDirectoryAssetSource>>
collectDirectorySources(const std::shared_ptr<IAssetSource>& source)
{
    std::vector<std::shared_ptr<const MDirectoryAssetSource>> out;
    if (!source) return out;

    std::vector<std::shared_ptr<IAssetSource>> parts;
    if (auto routed = std::dynamic_pointer_cast<MRoutedAssetSource>(source))
        parts = routed->getSources();
    else
        parts.push_back(source);

    for (const auto& part : parts)
        if (auto dir = std::dynamic_pointer_cast<const MDirectoryAssetSource>(part))
            out.push_back(dir);
    return out;
}

IWritableAssetSource* MEditorAssetManager::writableSource()
{
    auto* writable = source().asWritable();
    if (!writable)
        MERROR("EditorAssetManager:: active asset source is read-only");
    return writable;
}

// ---------------------------------------------------------------------------
// Refresh / cleanup
// ---------------------------------------------------------------------------

void MEditorAssetManager::refresh()
{
    // Stop the background thread during a full refresh so it does not
    // report stale events while we rebuild everything.
    watcherThread.stop();
    watcherThread.unwatchAll();
    watcherThread.clearKnownPaths();
    watcherThread.clearKnownDirectories();

    thumbnailCache.evictAll();
    thumbnailRenderer.clearQueue();
    failedAssetPaths.clear();
    ensureTemplatesRegistered();

    MAssetManager::refresh();

    scanDirectories();

    buildAssetTree();

    // The watcher maps asset paths to disk through the directory source(s),
    // so it must be given them before any path is registered. With a project
    // open that is two: the project folder and the engine install.
    watcherThread.setSources(collectDirectorySources(getAssetSource()));
    registerAssetsWithWatcher();

    if (!thumbnailRenderer.isInitialised())
        thumbnailRenderer.init();

    totalHotReloadCount = 0;

    // Start the background thread now that all assets and directories
    // are registered as known paths. No-op if the source is not a directory.
    watcherThread.start();
}

void MEditorAssetManager::cleanup()
{
    // Assets are about to be deleted; make sure nothing on the watcher thread
    // is mid-scan and no events referring to them are handled afterwards.
    watcherThread.stop();
    watcherThread.drainEvents();
    MAssetManager::cleanup();
}

int MEditorAssetManager::tickHotReload()
{
    // Drain events from the background watcher thread and handle them
    // on the main thread (where GL and asset-map access is safe).
    handleWatcherEvents();

    // Thumbnail generation -- one per frame.
    thumbnailRenderer.tick(thumbnailCache);
    return 0;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

bool MEditorAssetManager::onFileAdded(const SString& rawPath, bool rebuildTree)
{
    const SString path = MAssetPath::normalize(rawPath);

    // Asset might already be loaded (race between drain and registration,
    // or the watcher reporting a file we registered synchronously).
    if (assetMap.contains(path))
        return true;

    if (failedAssetPaths.contains(path) || MAssetPath::isIgnored(path))
        return false;

    const size_t deferredBefore = defferedLoadableAssetList.size();
    if (!loadAsset(path))
    {
        failedAssetPaths.insert(path);
        return false;
    }

    MLOG(STR("EditorAssetManager:: Loaded ") + path);
    watcherThread.watchPath(path);
    watcherThread.addKnownPath(path);

    // Run deferred loads for any newly added assets that need them.
    for (size_t i = deferredBefore; i < defferedLoadableAssetList.size(); ++i)
        if (defferedLoadableAssetList[i])
            defferedLoadableAssetList[i]->deferredAssetLoad(false);

    if (rebuildTree)
    {
        buildAssetTree();
        // The asset browser looks pings up by asset id (findNodeByAssetId).
        // This used to pass the path, so pings never matched.
        pingAsset(assetMap[path]->getAssetId());
    }
    return true;
}

bool MEditorAssetManager::unregisterAsset(const SString& path)
{
    watcherThread.unwatchPath(path);
    watcherThread.removeKnownPath(path);

    auto it = assetMap.find(path);
    if (it != assetMap.end() && it->second)
        thumbnailCache.evict(it->second->getAssetId());

    return MAssetManager::unregisterAsset(path);
}

// ---------------------------------------------------------------------------
// Watcher event handling (main thread)
// ---------------------------------------------------------------------------

void MEditorAssetManager::handleWatcherEvents()
{
    auto events = watcherThread.drainEvents();
    if (events.empty())
        return;

    bool treeChanged = false;
    SString lastLoadedAssetId;

    for (const auto& evt : events)
    {
        switch (evt.type)
        {
            case EWatchEvent::Modified:
            {
                auto it = assetMap.find(evt.path);
                if (it != assetMap.end() && it->second)
                {
                    MLOG(STR("AssetWatcher:: Reloading ") + evt.path);
                    // Also rebuilds dependents (materials of a shader,
                    // cubemaps of a face texture).
                    reloadAsset(it->second);
                    ++totalHotReloadCount;
                }
                break;
            }

            case EWatchEvent::NewFile:
            {
                if (assetMap.contains(evt.path))
                    break;
                if (onFileAdded(evt.path, false))
                {
                    lastLoadedAssetId = assetMap[evt.path]->getAssetId();
                    treeChanged = true;
                }
                break;
            }

            case EWatchEvent::Deleted:
            {
                if (!assetMap.contains(evt.path))
                    break;

                MLOG(STR("AssetWatcher:: Delta unloading: ") + evt.path);
                unregisterAsset(evt.path);
                treeChanged = true;
                break;
            }

            case EWatchEvent::NewDirectory:
            {
                if (directoryPaths.contains(evt.path))
                    break;

                if (!hasMetaData(evt.path))
                    createMetaFile(evt.path);

                directoryPaths.insert(evt.path);
                watcherThread.addKnownDirectory(evt.path);
                treeChanged = true;
                break;
            }

            case EWatchEvent::DeletedDirectory:
            {
                if (!directoryPaths.contains(evt.path))
                    break;

                directoryPaths.erase(evt.path);
                watcherThread.removeKnownDirectory(evt.path);
                treeChanged = true;
                break;
            }
        }
    }

    if (treeChanged)
    {
        buildAssetTree();
        if (!lastLoadedAssetId.empty())
            pingAsset(lastLoadedAssetId);
        MLOG(STR("AssetWatcher:: Tree rebuilt after delta changes"));
    }
}

// ---------------------------------------------------------------------------
// Thumbnail API
// ---------------------------------------------------------------------------

sf::Texture* MEditorAssetManager::getThumbnail(MAsset* asset)
{
    if (!asset) return nullptr;

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

// ---------------------------------------------------------------------------
// Watcher registration
// ---------------------------------------------------------------------------

void MEditorAssetManager::registerAssetsWithWatcher()
{
    for (auto& [path, asset] : assetMap)
    {
        watcherThread.watchPath(path);
        watcherThread.addKnownPath(path);
    }

    for (const auto& dirPath : directoryPaths)
    {
        watcherThread.addKnownDirectory(dirPath);
    }
}

// ---------------------------------------------------------------------------
// Directory scanning (during full refresh, runs on main thread)
// ---------------------------------------------------------------------------

void MEditorAssetManager::scanDirectories()
{
    directoryPaths.clear();

    source().enumerate([this](const SAssetSourceEntry& entry)
    {
        if (!entry.isDirectory) return;

        if (!hasMetaData(entry.path))
            createMetaFile(entry.path);

        directoryPaths.insert(entry.path);
    });
}

void MEditorAssetManager::ensureDirectoryNodeExists(const SString& dirPath)
{
    auto segments = dirPath.split("/");
    SAssetDirectoryNode* current = assetsTreeRoot;
    SString builtPath;

    for (const auto& segment : segments)
    {
        if (!builtPath.empty()) builtPath += "/";
        builtPath += segment;

        auto* child = current->getChild(segment);
        if (!child)
        {
            auto* node = new SAssetDirectoryNode();
            node->nodeName    = segment;
            node->nodePath    = builtPath;
            node->isDirectory = true;
            current->childrenNodes.push_back(node);
            child = node;
        }
        current = child;
    }
}

// ---------------------------------------------------------------------------
// Tree building
// ---------------------------------------------------------------------------

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

    for (const auto& dirPath : directoryPaths)
    {
        ensureDirectoryNodeExists(dirPath);
    }
}

void MEditorAssetManager::recursiveBuildAssetTree(std::queue<SString>& pathQueue,
                                                   SAssetDirectoryNode* parentNode,
                                                   MAsset* asset, SString parsedPath)
{
    if (pathQueue.empty()) return;

    SString current = pathQueue.front();
    pathQueue.pop();

    if (!parsedPath.empty()) parsedPath += "/";
    parsedPath += current;

    auto* existing = parentNode->getChild(current);

    if (pathQueue.empty())
    {
        if (existing)
        {
            existing->assetReference = asset;
            existing->isDirectory    = false;
        }
        else
        {
            auto* node           = new SAssetDirectoryNode();
            node->nodeName       = current;
            node->nodePath       = parsedPath;
            node->isDirectory    = false;
            node->assetReference = asset;
            parentNode->childrenNodes.push_back(node);
        }
        return;
    }

    if (!existing)
    {
        existing = new SAssetDirectoryNode();
        existing->nodeName    = current;
        existing->nodePath    = parsedPath;
        existing->isDirectory = true;
        parentNode->childrenNodes.push_back(existing);
    }

    recursiveBuildAssetTree(pathQueue, existing, asset, parsedPath);
}

// ---------------------------------------------------------------------------
// Directory management
// ---------------------------------------------------------------------------

bool MEditorAssetManager::createDirectory(const SString& parentPath,
                                           const SString& dirName)
{
    auto* target = writableSource();
    if (!target) return false;

    SString fullPath = parentPath;
    if (!fullPath.empty() && fullPath.str().back() != '/')
        fullPath += "/";
    fullPath += dirName;
    fullPath = MAssetPath::normalize(fullPath);

    if (target->exists(fullPath))
    {
        MWARN("EditorAssetManager:: Directory already exists: " + fullPath);
        return false;
    }

    if (!target->createDirectory(fullPath))
        return false;

    createMetaFile(fullPath);
    directoryPaths.insert(fullPath);
    watcherThread.addKnownDirectory(fullPath);
    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Created directory: ") + fullPath);
    return true;
}

bool MEditorAssetManager::deleteDirectory(const SString& rawDirPath)
{
    auto* target = writableSource();
    if (!target) return false;

    const SString dirPath = MAssetPath::normalize(rawDirPath);
    if (!target->exists(dirPath))
    {
        MWARN("EditorAssetManager:: Directory does not exist: " + dirPath);
        return false;
    }

    MLOG(STR("EditorAssetManager:: Deleting directory: ") + dirPath);

    const std::string prefix = dirPath.str() + "/";

    std::vector<SString> assetsToRemove;
    for (const auto& [path, asset] : assetMap)
    {
        const std::string& ps = path.str();
        if (ps.compare(0, prefix.size(), prefix) == 0)
            assetsToRemove.push_back(path);
    }
    for (const auto& path : assetsToRemove)
        unregisterAsset(path);

    std::vector<SString> dirsToRemove;
    for (const auto& dp : directoryPaths)
    {
        const std::string& ds = dp.str();
        if (dp == dirPath || ds.compare(0, prefix.size(), prefix) == 0)
            dirsToRemove.push_back(dp);
    }
    for (const auto& dp : dirsToRemove)
    {
        directoryPaths.erase(dp);
        watcherThread.removeKnownDirectory(dp);
    }

    // Removes the directory, everything in it, and its own .meta sidecar.
    target->removeDirectory(dirPath);

    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Directory deleted successfully"));
    return true;
}

// ---------------------------------------------------------------------------
// Open asset
// ---------------------------------------------------------------------------

void MEditorAssetManager::openAsset(MAsset* asset)
{
    if (asset == nullptr)
        return;
    MLOG(SString::format("EditorAssetManager:: Open Asset {0}", asset->getName()));
    if (!asset->openAsset())
    {
        const SString fullPath = asset->getFullPath();
        if (fullPath.empty())
        {
            MWARN("EditorAssetManager:: asset has no file on disk: " + asset->getPath());
            return;
        }

        // Scripts open in VS Code with the project folder as the workspace,
        // so LuaLS picks up .vscode/settings.json and the engine stubs.
        // Falls back to the OS default handler if VS Code isn't installed.
        if (dynamic_cast<IScriptAsset*>(asset) != nullptr &&
            MEditorUtility::openInVsCode(fullPath.str(), PROJECT_PATH().str()))
            return;

        auto cmd = STR("\"") + fullPath + STR("\"");
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
            invalidateThumbnail(asset);
            ++savedCount;
            MLOG(SString::format("Auto-saved dirty asset: {0}", path));
        }
    }
    return savedCount;
}

// ---------------------------------------------------------------------------
// Asset deletion
// ---------------------------------------------------------------------------

bool MEditorAssetManager::deleteAsset(MAsset* asset)
{
    if (!asset) return false;

    // The asset knows its own key; no need to search the map.
    const SString path = asset->getPath();
    if (!assetMap.contains(path) || assetMap[path] != asset)
    {
        MWARN("EditorAssetManager:: deleteAsset -- asset not found in map");
        return false;
    }

    return deleteAssetByPath(path);
}

bool MEditorAssetManager::deleteAssetByPath(const SString& rawPath)
{
    if (rawPath.empty()) return false;

    auto* target = writableSource();
    if (!target) return false;

    const SString path = MAssetPath::normalize(rawPath);
    MLOG(STR("EditorAssetManager:: Deleting asset: ") + path);

    unregisterAsset(path);
    target->remove(path);   // file + .meta sidecar

    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Asset deleted successfully"));
    return true;
}

// ---------------------------------------------------------------------------
// Templates
// ---------------------------------------------------------------------------

void MEditorAssetManager::ensureTemplatesRegistered()
{
    if (templatesRegistered)
        return;
    templatesRegistered = true;

    // Built-ins first, then templates.xml from the engine templates folder, so
    // data can add templates or override a built-in by id. Phase 4 adds
    // <project>/templates on top in the same way.
    registerBuiltInAssetTemplates(templateRegistry);
    templateRegistry.addTemplateDirectory(ENGINE_PATH(SEditorPaths::DIR_TEMPLATES_PATH));
}

SString MEditorAssetManager::makeUniquePath(const SString& directory, const SString& baseName,
                                             const SString& extension)
{
    SString filePath = MAssetPath::normalize(directory + "/" + baseName + extension);
    int suffix = 1;
    while (source().exists(filePath))
    {
        filePath = MAssetPath::normalize(directory + "/" + baseName + "_" + SString::fromInt(suffix) + extension);
        ++suffix;
    }
    return filePath;
}

SString MEditorAssetManager::createAssetFromTemplate(const SString& templateId, const SString& directory,
                                                     const SString& name,
                                                     const std::map<SString, SString>& params)
{
    ensureTemplatesRegistered();

    auto* target = writableSource();
    if (!target) return {};

    const SAssetTemplate* tmpl = templateRegistry.find(templateId);
    if (!tmpl)
    {
        MERROR(STR("EditorAssetManager:: unknown template ") + templateId);
        return {};
    }

    const SString fileName = name.empty() ? tmpl->defaultName : name;
    const SString filePath = makeUniquePath(directory, fileName, tmpl->extension);

    // Tokens use the final file name so __ASSET_NAME__ matches the file
    // (e.g. "Lit_1" when "Lit" already existed).
    std::string stem = filePath.str().substr(filePath.str().find_last_of('/') + 1);
    stem = stem.substr(0, stem.size() - tmpl->extension.str().size());

    SString content;
    if (!templateRegistry.generate(templateId, directory, SString(stem), params, content))
        return {};

    if (!target->writeText(filePath, content))
    {
        MERROR(SString::format("EditorAssetManager:: Failed to create file: {0}", filePath));
        return {};
    }

    MLOG(SString::format("EditorAssetManager:: Created {0}: {1}", tmpl->displayName, filePath));

    // Register now instead of waiting for the watcher, so the new asset is
    // selectable in the same frame.
    if (!onFileAdded(filePath))
        return {};
    return filePath;
}

// ---------------------------------------------------------------------------
// Legacy creation wrappers
// ---------------------------------------------------------------------------

bool MEditorAssetManager::createShaderAsset(const SString& directory,
                                             const SString& name,
                                             EShaderTemplate shaderTemplate)
{
    const char* id = nullptr;
    switch (shaderTemplate)
    {
    case EShaderTemplate::Lit:        id = SBuiltInTemplateIds::ShaderLit;        break;
    case EShaderTemplate::Unlit:      id = SBuiltInTemplateIds::ShaderUnlit;      break;
    case EShaderTemplate::UnlitColor: id = SBuiltInTemplateIds::ShaderUnlitColor; break;
    case EShaderTemplate::Toon:       id = SBuiltInTemplateIds::ShaderToon;       break;
    }
    if (!id)
    {
        MERROR("EditorAssetManager:: Unknown shader template");
        return false;
    }
    return !createAssetFromTemplate(id, directory, name).empty();
}

bool MEditorAssetManager::createSkyboxAsset(const SString& directory,
                                             const SString& name)
{
    return !createAssetFromTemplate(SBuiltInTemplateIds::Skybox, directory, name).empty();
}
