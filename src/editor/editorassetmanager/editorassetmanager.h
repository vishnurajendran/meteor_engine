//
// Created by ssj5v on 11-05-2025.
//

#ifndef EDITORASSETMANAGER_H
#define EDITORASSETMANAGER_H

#include <chrono>
#include <queue>
#include <set>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor_asset_directory_node.h"
#include "hotreloadwatcher.h"
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
    void deltaRefresh();

    void scanDirectories();
    void ensureDirectoryNodeExists(const SString& dirPath);

    static bool writeNewAssetFile(const SString& filePath, const SString& content);
    static SString loadTemplate(const SString& templateFileName, const SString& assetName);
    static const char* getShaderTemplateFileName(EShaderTemplate tmpl);

    static constexpr const char* DIR_TEMPLATES        = SEditorPaths::DIR_TEMPLATES_PATH;
    static constexpr const char* TEMPLATE_NAME_TOKEN  = "__ASSET_NAME__";

private:
    SAssetDirectoryNode* assetsTreeRoot      = nullptr;
    MHotReloadWatcher    hotReloadWatcher;
    MThumbnailCache      thumbnailCache;
    MThumbnailRenderer   thumbnailRenderer;
    int                  totalHotReloadCount = 0;

    static constexpr double               DELTA_SCAN_INTERVAL_SECONDS = 1.0;
    std::chrono::steady_clock::time_point lastDeltaScanTime =
        std::chrono::steady_clock::now();

    std::set<SString> failedAssetPaths;
    SString           pendingPingAssetId;

    std::set<SString> directoryPaths;
};

#endif // EDITORASSETMANAGER_H