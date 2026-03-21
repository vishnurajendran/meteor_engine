//
// Created by ssj5v on 11-05-2025.
//

#ifndef EDITORASSETMANAGER_H
#define EDITORASSETMANAGER_H
#include <queue>
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor_asset_directory_node.h"


class MEditorAssetManager : public MAssetManager {
public:
    void refresh() override;
    SAssetDirectoryNode* getAssetRootNode() const { return assetsTreeRoot; }
private:
    void buildAssetTree();
    void recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode, MAsset* asset, SString parsedPath);
private:
    SAssetDirectoryNode* assetsTreeRoot = nullptr;
};



#endif //EDITORASSETMANAGER_H
