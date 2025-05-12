//
// Created by ssj5v on 11-05-2025.
//

#include "editorassetmanager.h"

#include <iostream>
#include <queue>

#include "core/utils/logger.h"
void MEditorAssetManager::refresh()
{
    MAssetManager::refresh();
    buildAssetTree();
}

void MEditorAssetManager::buildAssetTree()
{
    //delete previous assignment
    if (assetsTreeRoot != nullptr)
        delete assetsTreeRoot;

    assetsTreeRoot = new SAssetDirectoryNode();
    assetsTreeRoot->nodeName = "Asset Root";
    assetsTreeRoot->nodePath = "Root";
    assetsTreeRoot->isDirectory = true;

    for (auto kv : assetMap)
    {
        std::queue<SString> pathQueue;
        for (const auto& sComp : kv.first.split("/"))
        {
            pathQueue.push(sComp);
        }
        recursiveBuildAssetTree(pathQueue,assetsTreeRoot,kv.second,"");
    }
}

void MEditorAssetManager::recursiveBuildAssetTree(std::queue<SString>& pathQueue, SAssetDirectoryNode* parentNode,MAsset* asset, SString parsedPath)
{
    //incase the directory is empty, we end it here
    if (pathQueue.empty())
        return;

    auto currentPathNodeName = pathQueue.front();
    pathQueue.pop();
    if (!parsedPath.empty())
        parsedPath += "/";
    parsedPath += currentPathNodeName;

    //if child already exists, skip adding new node, jump to next part of parsing.
    auto child = parentNode->getChild(currentPathNodeName);
    if (child != nullptr)
    {
        if (child->isDirectory)
        {
            recursiveBuildAssetTree(pathQueue, child, asset, parsedPath);
            return;
        }
        else
            return; //this is a duplicate entry, stop parse here.
    }

    // create new node to add as a child
    auto* node = new SAssetDirectoryNode();
    parentNode->childrenNodes.push_back(node);
    node->isDirectory = !pathQueue.empty();
    node->nodeName = currentPathNodeName;
    node->nodePath = parsedPath;

    if (!node->isDirectory)
    {
        node->assetReference = asset;
        return;
    }

    recursiveBuildAssetTree(pathQueue, node, asset, parsedPath);
}
