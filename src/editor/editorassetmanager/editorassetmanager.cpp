//
// Created by ssj5v on 11-05-2025.
//

#include "editorassetmanager.h"

#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>

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

    scanDirectories();

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
    SString lastLoadedAsset = "";

    std::set<SString> currentDirs;

    // Pass 1: new files + directory discovery
    for (const auto& searchPath : ASSET_SEARCH_PATHS)
    {
        std::filesystem::path dir(searchPath.str());
        if (!std::filesystem::exists(dir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (entry.is_directory())
            {
                SString path = STR(entry.path().string());
                path.replace("\\", "/");
                currentDirs.insert(path);
                continue;
            }

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

                lastLoadedAsset = path;
            }
            else
            {
                failedAssetPaths.insert(path);
            }
        }
    }

    // Detect directory additions or removals
    if (currentDirs != directoryPaths)
    {
        for (const auto& dirPath : currentDirs)
        {
            if (!directoryPaths.contains(dirPath) && !hasMetaData(dirPath))
                createMetaFile(dirPath);
        }
        directoryPaths = currentDirs;
        changed = true;
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
        pingAsset(lastLoadedAsset);
        MLOG(STR("EditorAssetManager:: Delta refresh complete"));
    }
}

void MEditorAssetManager::registerAssetsWithWatcher()
{
    for (auto& [path, asset] : assetMap)
        hotReloadWatcher.watchAsset(asset);
}

// ── Directory scanning ──────────────────────────────────────────────────────

void MEditorAssetManager::scanDirectories()
{
    directoryPaths.clear();

    for (const auto& searchPath : ASSET_SEARCH_PATHS)
    {
        std::filesystem::path dir(searchPath.str());
        if (!std::filesystem::exists(dir)) continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (!entry.is_directory()) continue;

            SString path = STR(entry.path().string());
            path.replace("\\", "/");

            if (!hasMetaData(path))
                createMetaFile(path);

            directoryPaths.insert(path);
        }
    }
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

// ── Tree building ───────────────────────────────────────────────────────────

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

// ── Directory management ────────────────────────────────────────────────────

bool MEditorAssetManager::createDirectory(const SString& parentPath,
                                           const SString& dirName)
{
    SString fullPath = parentPath;
    if (!fullPath.empty() && fullPath.str().back() != '/')
        fullPath += "/";
    fullPath += dirName;

    std::filesystem::path dirFsPath(fullPath.str());
    if (std::filesystem::exists(dirFsPath))
    {
        MWARN("EditorAssetManager:: Directory already exists: " + fullPath);
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::create_directories(dirFsPath, ec) || ec)
    {
        MERROR(SString::format("EditorAssetManager:: Failed to create directory: {0} ({1})",
                                fullPath, SString(ec.message().c_str())));
        return false;
    }

    createMetaFile(fullPath);
    directoryPaths.insert(fullPath);
    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Created directory: ") + fullPath);
    return true;
}

bool MEditorAssetManager::deleteDirectory(const SString& dirPath)
{
    std::filesystem::path dirFsPath(dirPath.str());
    if (!std::filesystem::exists(dirFsPath) || !std::filesystem::is_directory(dirFsPath))
    {
        MWARN("EditorAssetManager:: Directory does not exist: " + dirPath);
        return false;
    }

    MLOG(STR("EditorAssetManager:: Deleting directory: ") + dirPath);

    std::string prefix = dirPath.str() + "/";

    std::vector<SString> assetsToRemove;
    for (const auto& [path, asset] : assetMap)
    {
        const std::string& ps = path.str();
        if (ps.compare(0, prefix.size(), prefix) == 0)
            assetsToRemove.push_back(path);
    }

    for (const auto& path : assetsToRemove)
    {
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
    }

    std::vector<SString> dirsToRemove;
    for (const auto& dp : directoryPaths)
    {
        const std::string& ds = dp.str();
        if (dp == dirPath || ds.compare(0, prefix.size(), prefix) == 0)
            dirsToRemove.push_back(dp);
    }
    for (const auto& dp : dirsToRemove)
        directoryPaths.erase(dp);

    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(dirPath.str()), ec);
    if (ec)
    {
        MWARN(SString::format("EditorAssetManager:: Failed to delete directory: {0} ({1})",
                              dirPath, SString(ec.message().c_str())));
    }

    SString metaPath = dirPath + ".meta";
    std::filesystem::remove(std::filesystem::path(metaPath.str()), ec);

    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Directory deleted successfully"));
    return true;
}

// ── Open asset ──────────────────────────────────────────────────────────────

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
            invalidateThumbnail(asset);
            ++savedCount;
            MLOG(SString::format("Auto-saved dirty asset: {0}", path));
        }
    }
    return savedCount;
}

// --- Asset deletion ----------------------------------------------------------

bool MEditorAssetManager::deleteAsset(MAsset* asset)
{
    if (!asset) return false;

    SString targetPath;
    for (const auto& [path, a] : assetMap)
    {
        if (a == asset)
        {
            targetPath = path;
            break;
        }
    }

    if (targetPath.empty())
    {
        MWARN("EditorAssetManager:: deleteAsset -- asset not found in map");
        return false;
    }

    return deleteAssetByPath(targetPath);
}

bool MEditorAssetManager::deleteAssetByPath(const SString& path)
{
    if (path.empty()) return false;

    MLOG(STR("EditorAssetManager:: Deleting asset: ") + path);

    hotReloadWatcher.unwatchAsset(path);

    auto it = assetMap.find(path);
    if (it != assetMap.end())
    {
        MAsset* asset = it->second;
        if (asset)
        {
            thumbnailCache.evict(asset->getAssetId());
            const SString id = asset->getAssetId();
            if (!id.empty())
                assetMapByAssetId.erase(id);
            delete asset;
        }
        assetMap.erase(it);
    }

    std::error_code ec;
    std::filesystem::remove(std::filesystem::path(path.str()), ec);
    if (ec)
    {
        MWARN(SString::format("EditorAssetManager:: Failed to delete file: {0} ({1})",
                              path, SString(ec.message().c_str())));
    }

    SString metaPath = path + ".meta";
    std::filesystem::remove(std::filesystem::path(metaPath.str()), ec);

    buildAssetTree();

    MLOG(STR("EditorAssetManager:: Asset deleted successfully"));
    return true;
}

// --- Template loading --------------------------------------------------------

SString MEditorAssetManager::loadTemplate(const SString& templateFileName,
                                           const SString& assetName)
{
    std::filesystem::path templatePath =
        std::filesystem::path(DIR_TEMPLATES) / templateFileName.str();

    std::ifstream ifs(templatePath);
    if (!ifs.is_open())
    {
        MERROR(SString::format("EditorAssetManager:: Template not found: {0}",
                               SString(templatePath.string().c_str())));
        return SString();
    }

    std::ostringstream ss;
    ss << ifs.rdbuf();
    ifs.close();

    SString content = STR(ss.str().c_str());
    content.replace(TEMPLATE_NAME_TOKEN, assetName);
    return content;
}

const char* MEditorAssetManager::getShaderTemplateFileName(EShaderTemplate tmpl)
{
    switch (tmpl)
    {
    case EShaderTemplate::Lit:            return SEditorPaths::TEMPLATE_SHADER_LIT_FILE;
    case EShaderTemplate::Unlit:          return SEditorPaths::TEMPLATE_SHADER_UNLIT_FILE;
    case EShaderTemplate::UnlitColor:     return SEditorPaths::TEMPLATE_SHADER_UNLIT_COLOR_FILE;
    case EShaderTemplate::Toon:           return SEditorPaths::TEMPLATE_SHADER_TOONLIT_FILE;
    }
    return nullptr;
}

// --- File writing ------------------------------------------------------------

bool MEditorAssetManager::writeNewAssetFile(const SString& filePath,
                                             const SString& content)
{
    std::filesystem::path p(filePath.str());
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);

    std::ofstream ofs(p, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
    {
        MERROR(SString::format("EditorAssetManager:: Failed to create file: {0}", filePath));
        return false;
    }
    ofs << content.str();
    ofs.close();
    return true;
}

static SString makeUniquePath(const SString& directory, const SString& baseName,
                               const SString& extension)
{
    SString filePath = directory + "/" + baseName + extension;
    int suffix = 1;
    while (std::filesystem::exists(std::filesystem::path(filePath.str())))
    {
        filePath = directory + "/" + baseName + "_" + SString::fromInt(suffix) + extension;
        ++suffix;
    }
    return filePath;
}

// --- Create Shader -----------------------------------------------------------

bool MEditorAssetManager::createShaderAsset(const SString& directory,
                                             const SString& name,
                                             EShaderTemplate shaderTemplate)
{
    const char* tmplFile = getShaderTemplateFileName(shaderTemplate);
    if (!tmplFile)
    {
        MERROR("EditorAssetManager:: Unknown shader template");
        return false;
    }

    SString fileName = name.empty() ? SString("New_Shader") : name;
    SString filePath = makeUniquePath(directory, fileName, STR(SEditorPaths::EXTENSION_SHADER));

    SString content = loadTemplate(STR(tmplFile), fileName);
    if (content.empty())
        return false;

    if (!writeNewAssetFile(filePath, content))
        return false;

    MLOG(SString::format("EditorAssetManager:: Created shader: {0}", filePath));
    lastDeltaScanTime = std::chrono::steady_clock::time_point{};
    return true;
}

// --- Create Skybox -----------------------------------------------------------

bool MEditorAssetManager::createSkyboxAsset(const SString& directory,
                                             const SString& name)
{
    SString fileName = name.empty() ? SString("New_Skybox") : name;
    SString filePath = makeUniquePath(directory, fileName, STR(".skybox"));

    SString content = loadTemplate(STR(SEditorPaths::TEMPLATE_SKYBOX_FILE), fileName);
    if (content.empty())
        return false;

    if (!writeNewAssetFile(filePath, content))
        return false;

    MLOG(SString::format("EditorAssetManager:: Created skybox: {0}", filePath));
    lastDeltaScanTime = std::chrono::steady_clock::time_point{};
    return true;
}