//
// Created by ssj5v on 21-01-2025.
//

#include "menubartree.h"
#include "ImGui.h"
#include "menubaritem.h"
#include "core/utils/logger.h"

MMenubarTreeNode* MMenubarTreeNode::root = nullptr;
std::vector<MMenubarItem*> MMenubarTreeNode::registeredMenuItems;

MMenubarTreeNode::MMenubarTreeNode(): children()
{
    this->itemInstance = nullptr;
    nodeType = EMenubarTreeNodeType::Node;
}

MMenubarTreeNode::MMenubarTreeNode(EMenubarTreeNodeType type) : MMenubarTreeNode()
{
    nodeType = type;
}

MMenubarTreeNode::MMenubarTreeNode(MMenubarItem* itemInstance) : MMenubarTreeNode()
{
    this->itemInstance = itemInstance;
}

void MMenubarTreeNode::renderMenubar() const
{
    if (nodeType == EMenubarTreeNodeType::Root)
    {
        for (const auto& childKey : orderedChildrenKeys)
        {
            children.at(childKey)->renderMenubar();
        }
        return;
    }

    ImGui::PushID(this);
    if (itemInstance != nullptr)
    {
        //get the last item from the path and display as the menu title
        if (ImGui::MenuItem(menuTreeNodeTitle.c_str()))
        {
            itemInstance->onSelect();
        }
    }
    else if (!children.empty())
    {
        //recursively render the sub-menus
        if (ImGui::BeginMenu(menuTreeNodeTitle.c_str()))
        {
            for (const auto& childKey : orderedChildrenKeys)
            {
                children.at(childKey)->renderMenubar();
            }
            ImGui::EndMenu();
        }
    }
    ImGui::PopID();
}


void MMenubarTreeNode::registerItem(MMenubarItem* itemInstance)
{
    if (itemInstance == nullptr)
        return;

    registeredMenuItems.push_back(itemInstance);
}

// depth index is used to check, how deep into the path we are, while parsing the tree
void MMenubarTreeNode::registerItemInternal(MMenubarItem* itemInstance, const int depth)
{
    const auto nameSplit = itemInstance->getPath().split("/");
    if (depth >= nameSplit.size()-1)
    {
        //add as leaf.
        const auto childTreeLeaf = new MMenubarTreeNode();
        const auto& leafTitle = nameSplit.back();
        childTreeLeaf->itemInstance = itemInstance;
        childTreeLeaf->menuTreeNodeTitle = leafTitle;
        children[leafTitle] = childTreeLeaf;
        orderedChildrenKeys.push_back(leafTitle);
    }
    else
    {
        //recursively add to children
        const auto& nodeTitle = nameSplit[depth];

        if (children.contains(nodeTitle))
        {
            children.at(nodeTitle)->registerItemInternal(itemInstance, depth + 1);
            return;
        }

        const auto childTreeNode = new MMenubarTreeNode();
        childTreeNode->menuTreeNodeTitle = nodeTitle;
        children[nodeTitle] = childTreeNode;
        orderedChildrenKeys.push_back(nodeTitle);
        childTreeNode->registerItemInternal(itemInstance, depth + 1);
    }
}

MMenubarTreeNode* MMenubarTreeNode::getRoot()
{
    if (root == nullptr)
        root = new MMenubarTreeNode(EMenubarTreeNodeType::Root);
    return root;
}

void MMenubarTreeNode::buildTree()
{
    //sort the items according to their priorities.
    std::sort(registeredMenuItems.begin(), registeredMenuItems.end(), [](MMenubarItem* a, MMenubarItem* b)
    {
        return a->getPriority() <= b->getPriority();
    });

    for (const auto itemInstance : registeredMenuItems)
    {
        getRoot()->registerItemInternal(itemInstance, 0);
    }
    MLOG("Registered Menubar items : " + std::to_string(registeredMenuItems.size()));
}
