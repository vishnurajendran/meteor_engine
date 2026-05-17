//
// menubartree.cpp
//
#include "menubartree.h"
#include "ImGui.h"
#include "menubaritem.h"
#include "core/utils/logger.h"

MMenubarTreeNode* MMenubarTreeNode::root = nullptr;
std::vector<MMenubarItem*> MMenubarTreeNode::registeredMenuItems;

MMenubarTreeNode::MMenubarTreeNode() : children()
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
            children.at(childKey)->renderMenubar();
        return;
    }

    ImGui::PushID(this);
    if (itemInstance != nullptr)
    {
        // Pass the shortcut display text so ImGui right-aligns it in the menu.
        const std::string hint = itemInstance->getShortcut().getDisplayText();
        if (ImGui::MenuItem(menuTreeNodeTitle.c_str(),
                            hint.empty() ? nullptr : hint.c_str()))
        {
            itemInstance->onSelect();
        }
    }
    else if (!children.empty())
    {
        if (ImGui::BeginMenu(menuTreeNodeTitle.c_str()))
        {
            for (const auto& childKey : orderedChildrenKeys)
                children.at(childKey)->renderMenubar();
            ImGui::EndMenu();
        }
    }
    ImGui::PopID();
}

void MMenubarTreeNode::registerItem(MMenubarItem* itemInstance)
{
    if (itemInstance == nullptr) return;
    registeredMenuItems.push_back(itemInstance);
}

void MMenubarTreeNode::registerItemInternal(MMenubarItem* itemInstance, const int depth)
{
    const auto nameSplit = itemInstance->getPath().split("/");
    if (depth >= nameSplit.size() - 1)
    {
        const auto childTreeLeaf = new MMenubarTreeNode();
        const auto& leafTitle = nameSplit.back();
        childTreeLeaf->itemInstance = itemInstance;
        childTreeLeaf->menuTreeNodeTitle = leafTitle;
        children[leafTitle] = childTreeLeaf;
        orderedChildrenKeys.push_back(leafTitle);
    }
    else
    {
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
    std::sort(registeredMenuItems.begin(), registeredMenuItems.end(),
        [](MMenubarItem* a, MMenubarItem* b) {
            return a->getPriority() <= b->getPriority();
        });

    for (const auto itemInstance : registeredMenuItems)
        getRoot()->registerItemInternal(itemInstance, 0);

    // Wire up keyboard shortcuts after the tree is fully assembled.
    registerShortcuts();

    MLOG("Registered Menubar items : " + std::to_string(registeredMenuItems.size()));
}

void MMenubarTreeNode::registerShortcuts()
{
    MShortcutListener::clear();
    for (auto* item : registeredMenuItems)
    {
        MShortcutBinding binding = item->getShortcut();
        if (binding.isValid())
            MShortcutListener::registerShortcut(item, binding);
    }
}

void MMenubarTreeNode::drawAllPopups()
{
    for (auto* item : registeredMenuItems)
        item->drawPopup();
}

// -- getNodeAtPath ------------------------------------------------------------

MMenubarTreeNode* MMenubarTreeNode::getNodeAtPath(const SString& path)
{
    MMenubarTreeNode* current = getRoot();
    const auto segments = path.split("/");

    for (const auto& seg : segments)
    {
        if (seg.empty()) continue;
        auto it = current->children.find(seg);
        if (it == current->children.end()) return nullptr;
        current = it->second;
    }
    return current;
}

// -- renderAsContextMenu ------------------------------------------------------

void MMenubarTreeNode::renderAsContextMenu() const
{
    for (const auto& childKey : orderedChildrenKeys)
    {
        MMenubarTreeNode* child = children.at(childKey);
        ImGui::PushID(child);

        if (child->itemInstance != nullptr)
        {
            const std::string hint = child->itemInstance->getShortcut().getDisplayText();
            if (ImGui::MenuItem(child->menuTreeNodeTitle.c_str(),
                                hint.empty() ? nullptr : hint.c_str()))
            {
                child->itemInstance->onSelect();
            }
        }
        else if (!child->children.empty())
        {
            if (ImGui::BeginMenu(child->menuTreeNodeTitle.c_str()))
            {
                child->renderAsContextMenu();
                ImGui::EndMenu();
            }
        }

        ImGui::PopID();
    }
}