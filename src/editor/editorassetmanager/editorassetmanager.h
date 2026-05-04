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

class MEditorAssetManager : public MAssetManager
{
public:
    void refresh() override;
    void openAsset(MAsset* asset);

    // Called by MEditorApplication::run() every frame.
    // Handles both hot-reload (1s poll) and delta scan (5s poll).
    // Also ticks the thumbnail renderer — one thumbnail generated per frame.
    int tickHotReload();

    int getTotalHotReloadCount() const { return totalHotReloadCount; }

    [[nodiscard]] SAssetDirectoryNode* getAssetRootNode() const { return assetsTreeRoot; }

    // ── Asset ping — "focus in browser" ──────────────────────────────────────
    // Called by MAssetReferenceControl when the user clicks a reference slot.
    // The asset window consumes this once per frame in tickAndSync() and
    // navigates to the asset's folder, selecting it.
    void    pingAsset(const SString& assetId) { pendingPingAssetId = assetId; }
    SString consumePendingPing()              { SString id = pendingPingAssetId; pendingPingAssetId.clear(); return id; }

    // ── Thumbnail API ─────────────────────────────────────────────────────────
    // Call from the asset window's drawAssetTile(). Returns immediately —
    // if the thumbnail isn't ready yet, nullptr is returned and generation
    // is queued for the next available frame.
    sf::Texture*   getThumbnail(MAsset* asset);
    void           requestThumbnail(MAsset* asset);

private:
    void buildAssetTree();
    void recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode,
                                 MAsset* asset, SString parsedPath);
    void registerAssetsWithWatcher();
    void deltaRefresh();

private:
    SAssetDirectoryNode* assetsTreeRoot      = nullptr;
    MHotReloadWatcher    hotReloadWatcher;
    MThumbnailCache      thumbnailCache;
    MThumbnailRenderer   thumbnailRenderer;
    int                  totalHotReloadCount = 0;

    static constexpr double               DELTA_SCAN_INTERVAL_SECONDS = 5.0;
    std::chrono::steady_clock::time_point lastDeltaScanTime =
        std::chrono::steady_clock::now();

    // Paths that failed to import during a delta scan are recorded here and
    // permanently skipped on future scans.  A full refresh() clears the set
    // so manual fixes to broken files are picked up after a re-scan.
    std::set<SString> failedAssetPaths;
    SString           pendingPingAssetId;
};

#endif // EDITORASSETMANAGER_H