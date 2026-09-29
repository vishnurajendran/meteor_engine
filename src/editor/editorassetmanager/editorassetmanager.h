//
// Created by ssj5v on 11-05-2025.
//
// Intended location: src/editor/editorassetmanager/editorassetmanager.h
//

#ifndef EDITORASSETMANAGER_H
#define EDITORASSETMANAGER_H

#include <memory>
#include <queue>
#include <set>
#include <map>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "asset_template_registry.h"
#include "editor_asset_directory_node.h"
#include "asset_watcher_thread.h"
#include "thumbnail_renderer.h"

class IWritableAssetSource;
class MDirectoryAssetSource;

// Legacy: prefer createAssetFromTemplate() with the ids in
// builtin_asset_templates.h. Kept so existing UI code keeps compiling.
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
    void cleanup() override;
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

    // -- Asset registration ----------------------------------------------------
    // Imports a file that was just written and registers it with the watcher,
    // synchronously. Every create* method calls this, so new assets appear in
    // the browser in the same frame. Safe to call for a path the watcher also
    // reports later (it is a no-op for already-loaded paths).
    bool onFileAdded(const SString& path, bool rebuildTree = true);

    // -- Asset creation --------------------------------------------------------
    // Templates available for the "Create >" menu. Built-ins are registered on
    // first refresh(); templates.xml files in template directories add more.
    MAssetTemplateRegistry&       getTemplateRegistry()       { ensureTemplatesRegistered(); return templateRegistry; }
    const MAssetTemplateRegistry& getTemplateRegistry() const { return templateRegistry; }

    // Generates content from the template, writes it to a unique path in
    // `directory` (name, name_1, ...), registers it and pings it.
    // Returns the new asset path, or an empty string on failure.
    SString createAssetFromTemplate(const SString& templateId, const SString& directory,
                                    const SString& name,
                                    const std::map<SString, SString>& params = {});

    // Legacy wrappers around createAssetFromTemplate().
    bool createShaderAsset(const SString& directory, const SString& name,
                           EShaderTemplate shaderTemplate);
    bool createSkyboxAsset(const SString& directory, const SString& name);

    // -- Directory management --------------------------------------------------
    bool createDirectory(const SString& parentPath, const SString& dirName);
    bool deleteDirectory(const SString& dirPath);

    // -- Asset deletion --------------------------------------------------------
    bool deleteAsset(MAsset* asset);
    bool deleteAssetByPath(const SString& path);

protected:
    bool unregisterAsset(const SString& path) override;

private:
    // Writable view of the active source. nullptr if the active source is
    // read-only (should not happen in the editor).
    IWritableAssetSource* writableSource();

    void buildAssetTree();
    void recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode,
                                 MAsset* asset, SString parsedPath);
    void registerAssetsWithWatcher();

    // Drains events from the background watcher thread and processes them
    // on the main thread where GL and asset-map access is safe.
    void handleWatcherEvents();

    void scanDirectories();
    void ensureDirectoryNodeExists(const SString& dirPath);

    SString makeUniquePath(const SString& directory, const SString& baseName,
                           const SString& extension);
    void    ensureTemplatesRegistered();

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

    MAssetTemplateRegistry templateRegistry;
    bool                   templatesRegistered = false;
};

#endif // EDITORASSETMANAGER_H
