//
// Created by ssj5v on 11-05-2025.
//
#include "editor_asset_directory_node.h"

SAssetDirectoryNode* SAssetDirectoryNode::getChild(const SString& name)
{
    for (auto child : childrenNodes)
        if (child->nodeName == name)
            return child;

    return nullptr;
}


SAssetDirectoryNode::~SAssetDirectoryNode()
{
    for (auto children : childrenNodes)
    {
        if (children)
            delete children;
    }
}
