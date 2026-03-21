//
// Created by ssj5v on 11-05-2025.
//

#ifndef EDITORASSETWINDOW_H
#define EDITORASSETWINDOW_H
#include "SFML/Graphics.hpp"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "editor/editorassetmanager/editor_asset_directory_node.h"
#include "editor/window/imgui/imguisubwindow.h"

class MEditorAssetWindow : public MImGuiSubWindow {
private:
    void drawCurrentPath();
    void drawAssetWindowControls();
    void drawAssetWindow(SAssetDirectoryNode* root);
    void drawNodeAsDirectory(const int& id, MAssetManager* assetManager, SAssetDirectoryNode* node,
                             const ImVec2& containerSize, const ImVec2& iconSize);
    void onFileDoubleClicked(SAssetDirectoryNode* node);
    void drawNodeAsFile(const int& id, MAssetManager* assetManager, SAssetDirectoryNode* node,const ImVec2& containerSize, const ImVec2& iconSize);
    sf::Texture* getFileIcon(MAssetManager* assetManager, SAssetDirectoryNode* asset) const;
    void drawAssetText(SAssetDirectoryNode* node);

    void doAssetDragSource(SString key, const sf::Texture& icon, SAssetDirectoryNode* node);

public:
    MEditorAssetWindow();
    MEditorAssetWindow(int x, int y);
    ~MEditorAssetWindow() override;
    void onGui() override;
private:
    SAssetDirectoryNode* currentAssetDirectoryNode = nullptr;
    SAssetDirectoryNode* selectedAssetNode = nullptr;
    std::vector<SAssetDirectoryNode*> nodeHistoryStack;
    float scalingFactor = 1.559f;

    SString draggedAssetId;
};



#endif //EDITORASSETWINDOW_H
