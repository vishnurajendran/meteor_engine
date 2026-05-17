//
// Redesigned MEditorHierarchyWindow — Unity Hierarchy / Unreal World Outliner style
//

#pragma once
#ifndef METEOR_ENGINE_EDITORHIERARCHYWINDOW_H
#define METEOR_ENGINE_EDITORHIERARCHYWINDOW_H

#include "editor/meteorite_minimal.h"

class MEditorHierarchyWindow : public MImGuiSubWindow
{
    DEFINE_OBJECT_SUBCLASS(MEditorHierarchyWindow)
    void drawToolbar();

    void drawSceneRoot(MScene* scene);
    void drawEntityRow(MSpatialEntity* entity);

    bool matchesSearch(MSpatialEntity* entity) const;
    bool subtreeMatchesSearch(MSpatialEntity* entity) const;
    void openContextMenu(MSpatialEntity* entity);
    void handleDragDrop(MSpatialEntity* entity);
    int  countVisibleEntities(MScene* scene) const;

public:
    MEditorHierarchyWindow();
    explicit MEditorHierarchyWindow(int x, int y);
    void onGui(float deltaTime) override;

private:
    sf::Texture sceneTex;
    sf::Texture entityTex;

    std::unordered_map<MTypeInfo, sf::Texture> typeToIcon;

    sf::Vector2f sceneTexSize;
    sf::Vector2f entityTexSize;

    char searchBuffer[256] = {};

    MSpatialEntity* rightClickedEntity  = nullptr;
    MSpatialEntity* renamingEntity      = nullptr;
    char            renameBuffer[256]   = {};
    bool            pendingFocusRename  = false;

    MSpatialEntity* draggedEntity       = nullptr;
    MSpatialEntity* dropTargetEntity    = nullptr;
};

#endif // METEOR_ENGINE_EDITORHIERARCHYWINDOW_H