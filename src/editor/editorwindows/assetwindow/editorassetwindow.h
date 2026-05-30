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
    DEFINE_OBJECT_SUBCLASS(MEditorAssetWindow)

    void drawSourcesPanel();
    void drawDirectoryTree(SAssetDirectoryNode* node, int depth = 0);

    void drawToolbar();
    void drawBreadcrumbs();

    void drawContentArea();
    void drawAssetGrid(SAssetDirectoryNode* root);
    void drawAssetList(SAssetDirectoryNode* root);

    void drawAssetTile(SAssetDirectoryNode* node, IAssetManagerSubsystem* am, float iconSize);
    void drawAssetListRow(SAssetDirectoryNode* node, IAssetManagerSubsystem* am, int rowIndex);

    void drawTruncatedLabel(const std::string& text, float maxWidth);
    void navigateTo(SAssetDirectoryNode* node);
    void navigateBack();
    void openContextMenu(SAssetDirectoryNode* node);
    sf::Texture* getFileIcon(IAssetManagerSubsystem* am, SAssetDirectoryNode* asset) const;
    void doAssetDragSource(SString key, const sf::Texture& icon, SAssetDirectoryNode* node);

    bool matchesSearch(SAssetDirectoryNode* node) const;

    void onFileDoubleClicked(SAssetDirectoryNode* node);
    void selectAssetForInspector(SAssetDirectoryNode* node);

    void tickAndSync(float deltaTime);
    void syncWithAssetManager();

    static SAssetDirectoryNode* findNodeByPath(SAssetDirectoryNode* root,
                                               const SString& path);

    void processPendingPing();

    static SAssetDirectoryNode* findNodeByAssetId(SAssetDirectoryNode* root,
                                                  const SString& assetId,
                                                  SAssetDirectoryNode** outParent = nullptr);

    // Draws the delete-confirmation modal each frame.
    void drawDeleteConfirmModal();

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

    // Cached paths — kept in sync during normal navigation so that
    // syncWithAssetManager() never needs to dereference stale pointers.
    SString              cachedCurrentPath;
    SString              cachedSelectedPath;
    std::vector<SString> cachedHistoryBackPaths;
    std::vector<SString> cachedHistoryForwardPaths;

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

    int   lastSeenReloadCount = 0;
    float reloadFlashTimer    = 0.0f;
    static constexpr float RELOAD_FLASH_DURATION = 2.5f;

    // -- Delete confirmation state ---------------------------------------------
    bool                 pendingDeleteConfirm = false;
    SAssetDirectoryNode* pendingDeleteNode    = nullptr;
};

#endif // EDITORASSETWINDOW_H