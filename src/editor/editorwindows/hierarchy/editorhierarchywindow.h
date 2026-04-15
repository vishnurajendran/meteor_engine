//
// Redesigned MEditorHierarchyWindow — Unity Hierarchy / Unreal World Outliner style
//

#pragma once
#ifndef METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#define METEOR_ENGINE_EDITORHIERARCHYWINDOW_H

#include "editor/meteorite_minimal.h"

class MEditorHierarchyWindow : public MImGuiSubWindow
{
    // ── Top bar ───────────────────────────────────────────────────────────────
    void drawToolbar();

    // ── Tree ─────────────────────────────────────────────────────────────────
    void drawSceneRoot(MScene* scene);
    void drawEntityRow(MSpatialEntity* entity);

    // ── Helpers ───────────────────────────────────────────────────────────────
    bool matchesSearch(MSpatialEntity* entity) const;
    bool subtreeMatchesSearch(MSpatialEntity* entity) const;
    void openContextMenu(MSpatialEntity* entity);
    void handleDragDrop(MSpatialEntity* entity);
    int  countVisibleEntities(MScene* scene) const;

public:
    MEditorHierarchyWindow();
    explicit MEditorHierarchyWindow(int x, int y);
    void onGui() override;

private:
    sf::Texture sceneTex;
    sf::Texture entityTex;
    sf::Vector2f sceneTexSize;
    sf::Vector2f entityTexSize;

    // ── Search ────────────────────────────────────────────────────────────────
    char searchBuffer[256] = {};

    // ── Interaction state ─────────────────────────────────────────────────────
    MSpatialEntity* rightClickedEntity  = nullptr;
    MSpatialEntity* renamingEntity      = nullptr;
    char            renameBuffer[256]   = {};
    bool            pendingFocusRename  = false;

    // ── Drag & drop ───────────────────────────────────────────────────────────
    MSpatialEntity* draggedEntity       = nullptr;
    MSpatialEntity* dropTargetEntity    = nullptr;
};

#endif // METEOR_ENGINE_EDITORHIERARCHYWINDOW_H