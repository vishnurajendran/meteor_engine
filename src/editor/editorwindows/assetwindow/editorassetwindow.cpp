//
// Created by ssj5v on 11-05-2025.
//

#include "editorassetwindow.h"

#include <iostream>
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "core/engine/assetmanagement/textasset/textasset.h"
#include "core/engine/skybox/cubemaptexture.h"
#include "core/engine/texture/textureasset.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "glm/glm.hpp"
#include "imgui-SFML.h"

void MEditorAssetWindow::drawCurrentPath()
{
    if (currentAssetDirectoryNode == nullptr)
        return;

    int total = nodeHistoryStack.size();
    int curr = 0;
    for (auto currentNode : nodeHistoryStack)
    {
        if (currentNode == nullptr)
            continue;

        SString lblId = "##LBL" + std::to_string(curr);
        ImGui::PushID(lblId.c_str());
        if (ImGui::Button(currentNode->getName().c_str()))
        {
            // keep popping, until we reach here.
            while (nodeHistoryStack.back() != currentNode)
            {
                nodeHistoryStack.resize(nodeHistoryStack.size() - 1);
            }

            currentAssetDirectoryNode = currentNode;
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (curr < total - 1)
        {
            SString arrId = "##ARROW" + std::to_string(curr);
            ImGui::Text("%s", ">");
            ImGui::SameLine();
        }
        curr++;
    }

    for (int i = 0; i < nodeHistoryStack.size(); i++)
    {
    }
    ImGui::NewLine();
}

void MEditorAssetWindow::drawAssetWindowControls()
{
    if (selectedAssetNode != nullptr)
    {
        ImGui::Text("%s", selectedAssetNode->getPath().c_str());
        ImGui::SameLine();
    }

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("Zoom", &scalingFactor, 1.0f, 3.0f);
}


void MEditorAssetWindow::drawAssetWindow(SAssetDirectoryNode* root)
{
    MAssetManager* assetManager = MAssetManager::getInstance();
    const float padding = 18.0f * scalingFactor;
    const float iconSize = 64.0f * scalingFactor; // Width and height of each button
    const float cellSize = iconSize + padding;
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1) columnCount = 1;

    int itemIndex = 0;
    for (const auto& node : root->getChildrenNodes()) // Replace with your actual asset list
    {
        if (node == nullptr)
            continue;

        ImVec2 containerSize = ImVec2(iconSize + padding, iconSize + padding + 8);
        ImGui::PushID(++itemIndex);
        if (ImGui::BeginChild("##Container",containerSize))
        {
            if (node->isDirectory)
                drawNodeAsDirectory(itemIndex, assetManager, node,containerSize, ImVec2(iconSize, iconSize));
            else
                drawNodeAsFile(itemIndex, assetManager, node,containerSize,ImVec2(iconSize, iconSize));
        }

        ImGui::EndChild();
        ImGui::PopID();

        // Tooltip on hover
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", node->getPath().c_str());
        }

        // Move to next column
        if ((itemIndex + 1) % columnCount != 0)
            ImGui::SameLine();

        ++itemIndex;
    }

}

void MEditorAssetWindow::drawNodeAsDirectory(const int& id, MAssetManager* assetManager, SAssetDirectoryNode* node,
                                             const ImVec2& containerSize, const ImVec2& iconSize)
{
    const auto& textureAsset = assetManager->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/folder.png");
    if (!textureAsset)
        return;

    const auto& sfmlTexture = *textureAsset->getTexture()->getCoreTexture();
    const SString selId = "##Directory_Selectable" + std::to_string(id);
    bool selected = node == selectedAssetNode;
    auto flags = ImGuiSelectableFlags_AllowDoubleClick;
    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 region = ImGui::GetContentRegionAvail();
    if (ImGui::Selectable(selId.c_str(), selected, flags, region))
    {
        selectedAssetNode = node;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            currentAssetDirectoryNode = node;
            nodeHistoryStack.push_back(node);
        }
    }
    const SString ImgId = "##Directory_Ico";
    ImGui::SetCursorPos(pos);
    ImGui::SetCursorPosX((region.x - iconSize.x) * 0.5f);
    ImGui::Image(sfmlTexture, iconSize);

    drawAssetText(node);
}
void MEditorAssetWindow::onFileDoubleClicked(SAssetDirectoryNode* node)
{
    if (!node || node->isDirectory || !node->assetReference)
        return;

    auto cmd = STR("\"") + node->getAsset()->getFullPath() + STR("\"");
    system(cmd.c_str());
}

void MEditorAssetWindow::drawNodeAsFile(const int& id, MAssetManager* assetManager, SAssetDirectoryNode* node, const ImVec2& containerSize, const ImVec2& iconSize)
{
    MAsset* asset = node->getAsset();
    if (!asset)
        return;

    sf::Texture* sfmlTextureRef = getFileIcon(assetManager, node);

    if (!sfmlTextureRef)
        return;

    const SString fileId = "##File" + std::to_string(id);
    bool selected = node == selectedAssetNode;
    auto flags = ImGuiSelectableFlags_AllowDoubleClick;
    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 region = ImGui::GetContentRegionAvail();

    if (ImGui::Selectable(fileId.c_str(),selected,flags, region)) {
        selectedAssetNode = node;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            onFileDoubleClicked(node);
        }
    }

    doAssetDragSource(MAssetReferenceControl::ASSET_REF_TARGET_KEY.c_str(), *sfmlTextureRef, node);

    const SString ImgId = "##Directory_Ico";
    ImGui::SetCursorPos(pos);
    ImGui::SetCursorPosX((region.x - iconSize.x) * 0.5f);
    ImGui::Image(*sfmlTextureRef, iconSize);
    drawAssetText(node);
}

void MEditorAssetWindow::doAssetDragSource(SString key, const sf::Texture& icon, SAssetDirectoryNode* node)
{
    if (ImGui::BeginDragDropSource())
    {
        draggedAssetId = node->assetReference->getAssetId();
        ImGui::SetDragDropPayload(key.c_str(), &draggedAssetId, sizeof(draggedAssetId));
        ImGui::Image(icon, ImVec2(64, 64));
        drawAssetText(node);
        ImGui::EndDragDropSource();
    }
}

sf::Texture* MEditorAssetWindow::getFileIcon(MAssetManager* assetManager, SAssetDirectoryNode* asset) const
{
    // default
    auto defaultFileTextureAsset =
        assetManager->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/file-default.png")
            ->getTexture()
            ->getCoreTexture();

    // if asset is image type
    // kinda hacky, but for now ignore
    // todo: optimise this, casting always is going to be slow
    const auto& textureAsset = dynamic_cast<MTextureAsset*>(asset->getAsset());
    //hacky, but YOLO!!
    if (textureAsset && textureAsset->getTexture()->getCoreTexture())
    {
        return textureAsset->getTexture()->getCoreTexture();
    }

    for (const auto& importer : *MAssetImporter::getImporters())
    {
        auto extension = FileIO::getFileExtension(asset->getPath());
        if (importer->canImport(extension))
        {
            auto iconPath = importer->getIconPath();
            return MAssetManager::getInstance()->getAsset<MTextureAsset>(iconPath)->getTexture()->getCoreTexture();
        }
    }
    return defaultFileTextureAsset;
}

void MEditorAssetWindow::drawAssetText(SAssetDirectoryNode* node)
{
    SString text = node->getName();
    auto textSize = ImGui::CalcTextSize(node->getName().c_str());
    if (textSize.x > ImGui::GetContentRegionAvail().x)
        text = text.substring(0, glm::min(4, (int)text.length())) + "...";

    auto areaWidth = ImGui::GetContentRegionAvail().x;
    auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((areaWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text.c_str());
}


MEditorAssetWindow::MEditorAssetWindow() : MEditorAssetWindow(700, 300) {

}

MEditorAssetWindow::MEditorAssetWindow(int x, int y) : MImGuiSubWindow(x, y)
{
    title = "Assets";
    auto editorAssetManager = dynamic_cast<MEditorAssetManager*>(MAssetManager ::getInstance());
    auto root = editorAssetManager->getAssetRootNode();
    if (!root)
        return;
    auto rootChild = root->getChildrenNodes();
    for (const auto& child : rootChild)
    {
        auto childName = child->getName();
        childName.toLowerCase();
        if (childName == "assets")
        {
            currentAssetDirectoryNode = child;
            nodeHistoryStack.push_back(child);
            break;
        }
    }
}

MEditorAssetWindow::~MEditorAssetWindow()
{

}

void MEditorAssetWindow::onGui()
{
    if (currentAssetDirectoryNode == nullptr)
    {
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2);
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
        ImGui::Text("This should not visible, something went very wrong.");
        return;
    }

    auto contentRect = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("##AssetPathBar", ImVec2(contentRect.x, 24)))
    {
        drawCurrentPath();
    }
    ImGui::EndChild();
    contentRect = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("##AssetGrid", ImVec2(contentRect.x, contentRect.y - 24)))
    {
        drawAssetWindow(currentAssetDirectoryNode);
    }
    ImGui::EndChild();
    if (ImGui::BeginChild("##AssetWindowControls", ImVec2(contentRect.x, 24)))
    {
        drawAssetWindowControls();
    }
    ImGui::EndChild();
}
