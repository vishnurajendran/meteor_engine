//
// Redesigned MEditorAssetWindow — Unity/Unreal-style asset browser
//

#ifndef EDITORASSETWINDOW_H
#define EDITORASSETWINDOW_H
#pragma once

#include "SFML/Graphics.hpp"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor/editorassetmanager/editor_asset_directory_node.h"
#include "editor/window/imgui/imguisubwindow.h"

// ── View modes ────────────────────────────────────────────────────────────────
enum class EAssetViewMode
{
    Grid,
    List
};

// ── Sorting options ───────────────────────────────────────────────────────────
enum class EAssetSortMode
{
    Name,
    Type,
    DateModified
};

// ─────────────────────────────────────────────────────────────────────────────
class MEditorAssetWindow : public MImGuiSubWindow
{
    // ── Left panel ────────────────────────────────────────────────────────────
    void drawSourcesPanel();
    void drawDirectoryTree(SAssetDirectoryNode* node, int depth = 0);

    // ── Top bar ───────────────────────────────────────────────────────────────
    void drawToolbar();
    void drawBreadcrumbs();

    // ── Content area ─────────────────────────────────────────────────────────
    void drawContentArea();
    void drawAssetGrid(SAssetDirectoryNode* root);
    void drawAssetList(SAssetDirectoryNode* root);

    // ── Individual tile / row ─────────────────────────────────────────────────
    void drawAssetTile(SAssetDirectoryNode* node, MAssetManager* am, float iconSize);
    void drawAssetListRow(SAssetDirectoryNode* node, MAssetManager* am, int rowIndex);

    // ── Helpers ───────────────────────────────────────────────────────────────
    void drawTruncatedLabel(const std::string& text, float maxWidth);
    void navigateTo(SAssetDirectoryNode* node);
    void navigateBack();
    void onFileDoubleClicked(SAssetDirectoryNode* node);
    void openContextMenu(SAssetDirectoryNode* node);
    sf::Texture* getFileIcon(MAssetManager* am, SAssetDirectoryNode* asset) const;
    void doAssetDragSource(SString key, const sf::Texture& icon, SAssetDirectoryNode* node);

    bool matchesSearch(SAssetDirectoryNode* node) const;

public:
    MEditorAssetWindow();
    explicit MEditorAssetWindow(int x, int y);
    ~MEditorAssetWindow() override;

    void onGui() override;

private:
    // ── Navigation ────────────────────────────────────────────────────────────
    SAssetDirectoryNode* rootNode                 = nullptr;
    SAssetDirectoryNode* currentDirectoryNode     = nullptr;
    SAssetDirectoryNode* selectedNode             = nullptr;
    SAssetDirectoryNode* rightClickedNode         = nullptr;

    std::vector<SAssetDirectoryNode*> historyBack;   // breadcrumb / back stack
    std::vector<SAssetDirectoryNode*> historyForward; // forward stack

    // ── View state ────────────────────────────────────────────────────────────
    EAssetViewMode  viewMode    = EAssetViewMode::Grid;
    EAssetSortMode  sortMode    = EAssetSortMode::Name;
    float           zoomLevel   = 1.0f;          // 0.5 → 2.0
    bool            showSources = true;          // left panel visibility
    float           sourcesWidth = 200.0f;       // resizable splitter

    // ── Search / filter ───────────────────────────────────────────────────────
    char  searchBuffer[256] = {};
    bool  filterDirectories = true;
    bool  filterFiles       = true;

    // ── Drag & drop ───────────────────────────────────────────────────────────
    SString draggedAssetId;

    // ── Context-menu state ────────────────────────────────────────────────────
    bool pendingContextMenu = false;
};

#endif // EDITORASSETWINDOW_H