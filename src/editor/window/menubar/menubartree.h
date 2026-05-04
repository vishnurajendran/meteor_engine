//
// menubartree.h
//
#pragma once
#ifndef MENUBARTREE_H
#define MENUBARTREE_H
#include <map>
#include <unordered_map>

#include "core/object/object.h"
#include "core/utils/sstring_hash.h"

class MMenubarItem;

enum class EMenubarTreeNodeType
{
    Root,
    Node
};

class MMenubarTreeNode : public MObject
{
public:
    MMenubarTreeNode();
    MMenubarTreeNode(EMenubarTreeNodeType type);
    explicit MMenubarTreeNode(MMenubarItem* itemInstance);
    ~MMenubarTreeNode() override = default;

    void renderMenubar() const;

    static void registerItem(MMenubarItem* itemInstance);
    static MMenubarTreeNode* getRoot();
    static void buildTree();

    // Call every frame from the main draw loop (MImGuiWindow::drawGUI).
    // Iterates all registered items and calls their drawPopup() override.
    static void drawAllPopups();

    // Return the subtree node at a slash-delimited path, e.g. "Assets/Create".
    // Returns nullptr if the path does not exist in the tree.
    static MMenubarTreeNode* getNodeAtPath(const SString& path);

    // Render this node's children as ImGui menu items / submenus.
    // Used by context menus that want to mirror a menubar subtree.
    void renderAsContextMenu() const;

private:
    EMenubarTreeNodeType nodeType = EMenubarTreeNodeType::Node;
    SString menuTreeNodeTitle;
    std::map<SString, MMenubarTreeNode*> children;
    std::vector<SString> orderedChildrenKeys;
    MMenubarItem* itemInstance = nullptr;

    static std::vector<MMenubarItem*> registeredMenuItems;
    static MMenubarTreeNode* root;

    void registerItemInternal(MMenubarItem* itemInstance, int depth);
};

#endif // MENUBARTREE_H