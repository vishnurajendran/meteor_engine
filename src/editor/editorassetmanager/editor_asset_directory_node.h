//
// Created by ssj5v on 11-05-2025.
//

#ifndef EDITOR_ASSET_DIRECTORY_NODE_H
#define EDITOR_ASSET_DIRECTORY_NODE_H
#include "core/utils/sstring.h"


class MAsset;
struct SAssetDirectoryNode
{
public:
    SString nodeName; //name of the asset
    SString nodePath; //full path to asset
    bool isDirectory; // true if is node is a directory
    MAsset* assetReference=nullptr; // a pointer reference to asset;
    std::vector<SAssetDirectoryNode*> childrenNodes; //children nodes, only populated when isDirectory is true;

    SAssetDirectoryNode* getChild(const SString& name);
    ~SAssetDirectoryNode();

    [[nodiscard]] SString getName() const {return nodeName;}
    [[nodiscard]] SString getPath() const {return nodePath;}
    [[nodiscard]] bool nodeIsDirectory() const {return isDirectory;}
    [[nodiscard]] MAsset* getAsset() const {return assetReference;}
    [[nodiscard]] std::vector<SAssetDirectoryNode*> getChildrenNodes() const {return childrenNodes;}
};

#endif //EDITOR_ASSET_DIRECTORY_NODE_H
