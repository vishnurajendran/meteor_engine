//
// Created by ssj5v on 21-01-2025.
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

class MMenubarTreeNode : public MObject {
public:
    MMenubarTreeNode();
    MMenubarTreeNode(EMenubarTreeNodeType type);
    explicit MMenubarTreeNode(MMenubarItem* itemInstance);
    ~MMenubarTreeNode() override = default;
    void renderMenubar() const;

    static void registerItem(MMenubarItem* itemInstance);
    static MMenubarTreeNode* getRoot();
    static void buildTree();

private:
    EMenubarTreeNodeType nodeType = EMenubarTreeNodeType::Node;
    SString menuTreeNodeTitle;
    std::map<SString,MMenubarTreeNode*> children;
    std::vector<SString> orderedChildrenKeys;
    MMenubarItem* itemInstance;

    static std::vector<MMenubarItem*> registeredMenuItems;

    static MMenubarTreeNode* root;
    void registerItemInternal(MMenubarItem* itemInstance, int depth);
};



#endif //MENUBARTREE_H
