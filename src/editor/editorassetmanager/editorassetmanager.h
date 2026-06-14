//
// Created by ssj5v on 11-05-2025.
//
// Intended location: src/editor/editorassetmanager/editorassetmanager.h
//

#ifndef EDITORASSETMANAGER_H
#define EDITORASSETMANAGER_H

#include <queue>
#include <set>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor_asset_directory_node.h"
#include "asset_watcher_thread.h"
#include "thumbnail_renderer.h"

enum class EShaderTemplate
{
    Lit,
    Unlit,
    UnlitColor,
    Toon,
};

class MEditorAssetManager : public MAssetManager
{
    DEFINE_OBJECT_SUBCLASS(MEditorAssetManager)
public:
    void refresh() override;
    void openAsset(MAsset* asset);
    virtual int saveDirtyAssets();
    int tickHotReload();

    int getTotalHotReloadCount() const { return totalHotReloadCount; }

    [[nodiscard]] SAssetDirectoryNode* getAssetRootNode() const { return assetsTreeRoot; }

    // -- Asset ping -- "focus in browser" --------------------------------------
    void    pingAsset(const SString& assetId) { pendingPingAssetId = assetId; }
    SString consumePendingPing()              { SString id = pendingPingAssetId; pendingPingAssetId.clear(); return id; }

    // -- Thumbnail API ---------------------------------------------------------
    sf::Texture*   getThumbnail(MAsset* asset);
    void           requestThumbnail(MAsset* asset);

    // Evict a stale thumbnail (memory + disk) and re-queue rendering.
    // Call after a material save or any property change that should
    // refresh the preview.
    void invalidateThumbnail(MAsset* asset)
    {
        if (!asset) return;
        thumbnailCache.evict(asset->getAssetId());
        requestThumbnail(asset);
    }

    // -- Asset creation --------------------------------------------------------
    bool createShaderAsset(const SString& directory, const SString& name,
                           EShaderTemplate shaderTemplate);
    bool createSkyboxAsset(const SString& directory, const SString& name);

    // -- Directory management --------------------------------------------------
    bool createDirectory(const SString& parentPath, const SString& dirName);
    bool deleteDirectory(const SString& dirPath);

    // -- Asset deletion --------------------------------------------------------
    bool deleteAsset(MAsset* asset);
    bool deleteAssetByPath(const SString& path);

private:
    void buildAssetTree();
    void recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode,
                                 MAsset* asset, SString parsedPath);
    void registerAssetsWithWatcher();

    // Drains events from the background watcher thread and processes them
    // on the main thread where GL and asset-map access is safe.
    void handleWatcherEvents();

    void scanDirectories();
    void ensureDirectoryNodeExists(const SString& dirPath);

    static bool writeNewAssetFile(const SString& filePath, const SString& content);
    static SString loadTemplate(const SString& templateFileName, const SString& assetName);
    static const char* getShaderTemplateFileName(EShaderTemplate tmpl);

    static constexpr const char* DIR_TEMPLATES        = SEditorPaths::DIR_TEMPLATES_PATH;
    static constexpr const char* TEMPLATE_NAME_TOKEN  = "__ASSET_NAME__";

private:
    SAssetDirectoryNode*   assetsTreeRoot       = nullptr;
    MAssetWatcherThread    watcherThread;
    MThumbnailCache        thumbnailCache;
    MThumbnailRenderer     thumbnailRenderer;
    int                    totalHotReloadCount   = 0;

    // Paths that failed to import are skipped on future NewFile events.
    // A full refresh() clears this set so manual fixes are picked up.
    std::set<SString>      failedAssetPaths;
    SString                pendingPingAssetId;

    std::set<SString>      directoryPaths;
};

#endif // EDITORASSETMANAGER_H