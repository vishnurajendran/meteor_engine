//
// Created by ssj5v on 12-05-2025.
//

#include "asset_reference_controls.h"
#include "SFML/Graphics/Texture.hpp"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/engine/texture/textureasset.h"
#include "imgui.h"
#include "imgui-SFML.h"

const SString MAssetReferenceControl::ASSET_REF_TARGET_KEY = "_AssetFileRef";

MAssetReferenceControl::MAssetReferenceControl() : MAssetReferenceControl(nullptr)
{

}

MAssetReferenceControl::MAssetReferenceControl(MAsset* asset)
{
    this->assetIdReference = "";
    canAcceptAssetFuncCallback = defaultTestFuncCallback;

    if (!asset)
        return;

    this->assetIdReference = asset->getAssetId();
}
MAsset* MAssetReferenceControl::getAssetReference() const
{
    if (assetIdReference.empty())
        return nullptr;
    auto asset = MAssetManager::getInstance()->getAssetById<MAsset>(assetIdReference);
    if (!asset)
        return nullptr;
    return asset;
}
void MAssetReferenceControl::setAssetReference(MAsset* asset)
{
    if (asset)
        assetIdReference = asset->getAssetId();
}

sf::Texture* MAssetReferenceControl::getFileIcon(MAssetManager* assetManager, MAsset* asset) const
{
    // default
    auto defaultFileTextureAsset =
        assetManager->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/file-default.png")
            ->getTexture()
            ->getCoreTexture();

    if (asset == nullptr)
        return defaultFileTextureAsset;

    const auto& textureAsset = dynamic_cast<MTextureAsset*>(asset);
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

bool MAssetReferenceControl::defaultTestFuncCallback(MAsset* asset)
{
    return true;
}

bool MAssetReferenceControl::drawControl(const SString& label)
{
    bool modified=false;
    const auto size = ImVec2(64, 64);
    const auto containerY = 85;
    sf::Texture* icon = nullptr;
    if (assetIdReference.empty())
        icon = getFileIcon(MAssetManager::getInstance(), nullptr);
    else
        icon = getFileIcon(MAssetManager::getInstance(), getAssetReference());

    const auto& childId = STR("##ControlContainer") + getGUID();
    if (ImGui::BeginChild(childId.c_str(), ImVec2(0, containerY), true, ImGuiChildFlags_Borders))
    {

        const auto& bttnId = STR("##AssetReferenceControl") + getGUID();
        if (ImGui::ImageButton(bttnId.c_str(), *icon, size))
        {

        }

        if (ImGui::IsItemHovered()) {
            if (const auto asset = getAssetReference())
                ImGui::SetTooltip("%s", asset->getPath().c_str());
        }

        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            SString tempAssetIdReference = "";
            bool invalidRef = false;
            if (payload && strcmp(payload->DataType,ASSET_REF_TARGET_KEY.c_str()) == 0)
            {
                tempAssetIdReference = *static_cast<SString*>(payload->Data);
                if (canAcceptAssetFuncCallback && !canAcceptAssetFuncCallback(MAssetManager::getInstance()->getAssetById<MAsset>(tempAssetIdReference)))
                {
                    invalidRef = true;
                    ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(1,0,0,1));
                }
            }

            if (ImGui::AcceptDragDropPayload(ASSET_REF_TARGET_KEY.c_str()) && !invalidRef)
            {
                assetIdReference = tempAssetIdReference;
                modified = true;
            }

            if (invalidRef)
                ImGui::PopStyleColor();

            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));         // Red
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));  // Light Red
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));   // Dark Red
        const auto& closeButtonId = STR("X_") + getGUID();
        ImGui::PushID(closeButtonId.c_str());
        if (ImGui::Button("X", ImVec2(24,size.y+3)))
        {
            assetIdReference.clear();
            modified = true;
        }
        ImGui::PopID();
        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        const auto& labelId = STR("LABEL_") + getGUID();
        drawCenteredLabel(label, labelId);
    }
    ImGui::EndChild();
    return modified;
}

void MAssetReferenceControl::drawCenteredLabel(SString label, const SString& id)
{
    auto containerY = ImGui::GetContentRegionAvail().y;
    auto cursorPos = ImGui::GetCursorPos();
    cursorPos.x += 16;
    auto textSize = ImGui::CalcTextSize(label.c_str());
    ImGui::SetCursorPos(ImVec2(cursorPos.x, (containerY-textSize.y)*0.5f));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::PopID();
}
