//
// Redesigned MEditorAssetWindow — Unity/Unreal-style asset browser
//

#ifndef EDITORASSETWINDOW_H
#define EDITORASSETWINDOW_H
#pragma once

#include <queue>
#include "SFML/Graphics.hpp"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor/editorassetmanager/editor_asset_directory_node.h"
#include "editor/window/imgui/imguisubwindow.h"

enum class EAssetViewMode
{
    Grid,
    List
};

enum class EAssetSortMode
{
    Name,
    Type,
    DateModified
};

class MEditorAssetWindow : public MImGuiSubWindow
{
    // ── Left panel ────────────────────────────────────────────────────────────
    void drawSourcesPanel();
    void drawDirectoryTree(SAssetDirectoryNode* node, int depth = 0);

    void drawToolbar();
    void drawBreadcrumbs();

    void drawContentArea();
    void drawAssetGrid(SAssetDirectoryNode* root);
    void drawAssetList(SAssetDirectoryNode* root);

    void drawAssetTile(SAssetDirectoryNode* node, MAssetManager* am, float iconSize);
    void drawAssetListRow(SAssetDirectoryNode* node, MAssetManager* am, int rowIndex);

    void drawTruncatedLabel(const std::string& text, float maxWidth);
    void navigateTo(SAssetDirectoryNode* node);
    void navigateBack();
    void openContextMenu(SAssetDirectoryNode* node);
    sf::Texture* getFileIcon(MAssetManager* am, SAssetDirectoryNode* asset) const;
    void doAssetDragSource(SString key, const sf::Texture& icon, SAssetDirectoryNode* node);

    bool matchesSearch(SAssetDirectoryNode* node) const;

    void onFileDoubleClicked(SAssetDirectoryNode* node);
    // Single-click on an asset tile — sets MEditorApplication::Selected so
    // the inspector immediately shows the asset's properties.
    void selectAssetForInspector(SAssetDirectoryNode* node);

    // Called at the top of onGui() each frame.
    // - Detects if a full refresh() was called externally by comparing rootNode
    //   to the manager's current tree root. If stale, calls syncWithAssetManager().
    // - Detects new hot-reloads by comparing against getTotalHotReloadCount(),
    //   and arms the flash timer when new reloads are found.
    void tickAndSync(float deltaTime);

    // Re-binds rootNode and tries to navigate back to the same folder path
    // after a full refresh() has deleted and recreated the tree.
    void syncWithAssetManager();

    // BFS search for the node whose nodePath matches `path`.
    static SAssetDirectoryNode* findNodeByPath(SAssetDirectoryNode* root,
                                               const SString& path);

    // Consumes a pending ping from MEditorAssetManager, navigates to the
    // asset's parent folder, and selects the asset node.
    void processPendingPing();

    // BFS: find the asset node matching assetId and optionally its parent.
    static SAssetDirectoryNode* findNodeByAssetId(SAssetDirectoryNode* root,
                                                  const SString& assetId,
                                                  SAssetDirectoryNode** outParent = nullptr);

public:
    MEditorAssetWindow();
    explicit MEditorAssetWindow(int x, int y);
    ~MEditorAssetWindow() override;

    void onGui(float deltaTime) override;

private:
    SAssetDirectoryNode* rootNode             = nullptr;
    SAssetDirectoryNode* currentDirectoryNode = nullptr;
    SAssetDirectoryNode* selectedNode         = nullptr;
    SAssetDirectoryNode* rightClickedNode     = nullptr;

    std::vector<SAssetDirectoryNode*> historyBack;
    std::vector<SAssetDirectoryNode*> historyForward;

    EAssetViewMode viewMode     = EAssetViewMode::Grid;
    EAssetSortMode sortMode     = EAssetSortMode::Name;
    float          zoomLevel    = 1.0f;
    bool           showSources  = true;
    float          sourcesWidth = 200.0f;

    char searchBuffer[256] = {};
    bool filterDirectories = true;
    bool filterFiles       = true;

    SString draggedAssetId;
    bool pendingContextMenu = false;

    // Last value of getTotalHotReloadCount() we saw — used to detect new reloads.
    int   lastSeenReloadCount = 0;
    // Counts down from RELOAD_FLASH_DURATION after a hot-reload is detected.
    // While > 0, a flash indicator is shown in the toolbar.
    float reloadFlashTimer    = 0.0f;
    static constexpr float RELOAD_FLASH_DURATION = 2.5f;
};

#endif // EDITORASSETWINDOW_H