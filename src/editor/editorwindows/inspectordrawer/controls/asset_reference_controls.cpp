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
    return drawCompactControl(label);
}

// ─── Compact single-row control ───────────────────────────────────────────────
// Layout: [32px thumbnail] [asset name or "(none)"]  [×]
// Fits inside the material-properties table without breaking its row height.

bool MAssetReferenceControl::drawCompactControl(const SString& label)
{
    static constexpr float THUMB_SIZE  = 28.0f;
    static constexpr float ROW_H       = THUMB_SIZE + 8.0f;  // 36px total
    static constexpr float CLEAR_BTN_W = 20.0f;

    bool modified = false;
    auto* am   = MAssetManager::getInstance();
    auto* icon = assetIdReference.empty()
                     ? getFileIcon(am, nullptr)
                     : getFileIcon(am, getAssetReference());

    MAsset* currentAsset = assetIdReference.empty() ? nullptr : getAssetReference();
    const char* assetName = currentAsset ? currentAsset->getName().c_str() : "(none)";

    ImDrawList* dl    = ImGui::GetWindowDrawList();
    ImVec2      p     = ImGui::GetCursorScreenPos();
    float       avail = ImGui::GetContentRegionAvail().x;

    // ── Invisible button covers the whole row (drag-drop target + hover) ──
    ImGui::PushID(getGUID().c_str());
    const bool hovered = ImGui::IsMouseHoveringRect(p, { p.x + avail, p.y + ROW_H });

    // Subtle hover highlight
    if (hovered)
        dl->AddRectFilled(p, { p.x + avail, p.y + ROW_H }, IM_COL32(255, 255, 255, 12), 4.0f);

    // ── Thumbnail ─────────────────────────────────────────────────────────
    ImGui::SetCursorScreenPos({ p.x + 2.0f, p.y + 4.0f });
    if (icon)
        ImGui::Image(*icon, { THUMB_SIZE, THUMB_SIZE });
    else
        ImGui::Dummy({ THUMB_SIZE, THUMB_SIZE });

    // Make the thumbnail a drag-drop target
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        bool invalidRef = false;
        SString tempId;

        if (payload && strcmp(payload->DataType, ASSET_REF_TARGET_KEY.c_str()) == 0)
        {
            tempId = SString(static_cast<const char*>(payload->Data));
            if (canAcceptAssetFuncCallback &&
                !canAcceptAssetFuncCallback(am->getAssetById<MAsset>(tempId)))
            {
                invalidRef = true;
                ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(1, 0, 0, 1));
            }
        }
        if (ImGui::AcceptDragDropPayload(ASSET_REF_TARGET_KEY.c_str()) && !invalidRef)
        {
            assetIdReference = tempId;
            modified = true;
        }
        if (invalidRef) ImGui::PopStyleColor();
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsItemHovered() && currentAsset)
        ImGui::SetTooltip("%s", currentAsset->getPath().c_str());

    // ── Asset name ────────────────────────────────────────────────────────
    const float nameX   = p.x + THUMB_SIZE + 8.0f;
    const float nameMaxW = avail - THUMB_SIZE - CLEAR_BTN_W - 16.0f;
    ImGui::SetCursorScreenPos({ nameX, p.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f });

    if (currentAsset)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::PushTextWrapPos(nameX + nameMaxW);
        ImGui::TextUnformatted(assetName);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::TextUnformatted("(none)");
        ImGui::PopStyleColor();
    }

    // ── Clear (×) button — only when something is assigned ───────────────
    ImGui::SetCursorScreenPos({ p.x + avail - CLEAR_BTN_W - 2.0f,
                                p.y + (ROW_H - ImGui::GetFrameHeight()) * 0.5f });
    ImGui::BeginDisabled(!currentAsset);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.10f, 0.10f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.40f, 0.05f, 0.05f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
    if (ImGui::Button("×", { CLEAR_BTN_W, ImGui::GetFrameHeight() }))
    {
        assetIdReference.clear();
        modified = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::PopID();

    // Advance cursor past the row
    ImGui::SetCursorScreenPos({ p.x, p.y + ROW_H + 2.0f });
    ImGui::Dummy({ avail, 0.0f });

    return modified;
}