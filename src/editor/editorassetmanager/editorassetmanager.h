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

    // -- Asset ping — "focus in browser" --------------------------------------
    void    pingAsset(const SString& assetId) { pendingPingAssetId = assetId; }
    SString consumePendingPing()              { SString id = pendingPingAssetId; pendingPingAssetId.clear(); return id; }

    // -- Thumbnail API ---------------------------------------------------------
    sf::Texture*   getThumbnail(MAsset* asset);
    void           requestThumbnail(MAsset* asset);

    // -- Asset creation --------------------------------------------------------
    // Creates a .mesl shader file from a template in DIR_TEMPLATES.
    // Returns true on success.  Forces an immediate delta scan.
    bool createShaderAsset(const SString& directory, const SString& name,
                           EShaderTemplate shaderTemplate);

    // Creates a .cubemap skybox definition file from a template.
    bool createSkyboxAsset(const SString& directory, const SString& name);

    // -- Asset deletion --------------------------------------------------------
    bool deleteAsset(MAsset* asset);
    bool deleteAssetByPath(const SString& path);

private:
    void buildAssetTree();
    void recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode,
                                 MAsset* asset, SString parsedPath);
    void registerAssetsWithWatcher();
    void deltaRefresh();

    static bool writeNewAssetFile(const SString& filePath, const SString& content);

    // Reads a template from DIR_TEMPLATES, replaces __ASSET_NAME__ with
    // `assetName`, returns the result.  Empty string on failure.
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
};

#endif // EDITORASSETMANAGER_H