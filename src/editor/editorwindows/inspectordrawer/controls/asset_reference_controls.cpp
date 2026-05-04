//
// Created by ssj5v on 12-05-2025.
//

#include "asset_reference_controls.h"
#include "SFML/Graphics/Texture.hpp"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetimporter.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/texture/textureasset.h"
#include "core/utils/fileio.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "imgui-SFML.h"
#include "imgui.h"

const SString MAssetReferenceControl::ASSET_REF_TARGET_KEY = "_AssetFileRef";

MAssetReferenceControl::MAssetReferenceControl() : MAssetReferenceControl(nullptr) {}

MAssetReferenceControl::MAssetReferenceControl(TAssetHandle<MAsset> asset)
{
    assetIdReference           = "";
    canAcceptAssetFuncCallback = defaultTestFuncCallback;
    if (!asset) return;
    assetIdReference = asset->getAssetId();
}

MAsset* MAssetReferenceControl::getAssetReference() const
{
    if (assetIdReference.empty()) return nullptr;
    return MAssetManager::getInstance()->getAssetById<MAsset>(assetIdReference);
}

void MAssetReferenceControl::setAssetReference(MAsset* asset)
{
    if (asset) assetIdReference = asset->getAssetId();
}

sf::Texture* MAssetReferenceControl::getFileIcon(MAssetManager* am, TAssetHandle<MAsset> asset) const
{
    // Guard every asset + texture lookup — any of these can be null if the
    // asset hasn't loaded yet or the icon file is missing.
    sf::Texture* defaultIcon = nullptr;
    if (auto* iconAsset = am->getAsset<MTextureAsset>("meteor_assets/engine_assets/icons/file-default.png"))
        if (auto* tex = iconAsset->getTexture())
            defaultIcon = tex->getCoreTexture();

    if (!asset) return defaultIcon;

    if (auto* tex = dynamic_cast<MTextureAsset*>(asset.get()))
        if (tex->getTexture() && tex->getTexture()->getCoreTexture())
            return tex->getTexture()->getCoreTexture();

    for (const auto& imp : *MAssetImporter::getImporters())
    {
        auto ext = FileIO::getFileExtension(asset->getPath());
        if (imp->canImport(ext))
        {
            auto* iconAsset = am->getAsset<MTextureAsset>(imp->getIconPath());
            if (iconAsset && iconAsset->getTexture())
                if (auto* core = iconAsset->getTexture()->getCoreTexture())
                    return core;
        }
    }
    return defaultIcon;
}

bool MAssetReferenceControl::defaultTestFuncCallback(TAssetHandle<MAsset>) { return true; }

bool MAssetReferenceControl::drawControl(const SString& label)
{
    return drawCompactControl(label);
}

// drawCompactControl
//
// Layout:  [← dropZoneW ─────────────────────────] [×]
//          [32px thumb] [asset name / "(none)"]     [×]
//
// The InvisibleButton covers the entire drop zone so drag-drop works anywhere
// on the row, not just the thumbnail.  It also handles hover detection.
//
// Visual feedback while dragging:
//   green tint = payload type matches this control's filter (canAccept)
//   red   tint = payload rejected by canAcceptAssetFuncCallback
bool MAssetReferenceControl::drawCompactControl(const SString& label)
{
    static constexpr float THUMB_SIZE   = 28.0f;
    static constexpr float ROW_H        = THUMB_SIZE + 8.0f;   // 36 px
    static constexpr float CLEAR_BTN_W  = 20.0f;
    static constexpr float GAP          = 4.0f;

    bool modified = false;
    auto* am      = MAssetManager::getInstance();
    MAsset* current   = assetIdReference.empty() ? nullptr : getAssetReference();
    sf::Texture* icon = getFileIcon(am, current);

    ImDrawList* dl    = ImGui::GetWindowDrawList();
    ImVec2      p     = ImGui::GetCursorScreenPos();
    float       avail = ImGui::GetContentRegionAvail().x;
    const float dropW = std::max(avail - CLEAR_BTN_W - GAP, 10.f);

    ImGui::PushID(getGUID().c_str());

    // It must be the last item when BeginDragDropTarget() is called, so we
    // add it first and draw everything else on top with SetCursorScreenPos.
    ImGui::InvisibleButton("##dropzone", ImVec2(dropW, ROW_H));
    const bool hovered = ImGui::IsItemHovered();

    bool dragHovering = false;
    bool dragValid    = false;
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* peek = ImGui::GetDragDropPayload();
        SString tempId;

        if (peek && strcmp(peek->DataType, ASSET_REF_TARGET_KEY.c_str()) == 0)
        {
            tempId      = SString(static_cast<const char*>(peek->Data));
            dragHovering = true;
            dragValid    = !canAcceptAssetFuncCallback ||
                            canAcceptAssetFuncCallback(am->getAssetById<MAsset>(tempId));

            if (!dragValid)
                ImGui::PushStyleColor(ImGuiCol_DragDropTarget, ImVec4(0.8f, 0.1f, 0.1f, 0.5f));
        }

        // AcceptDragDropPayload fires on mouse-release.
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(ASSET_REF_TARGET_KEY.c_str()))
        {
            SString droppedId(static_cast<const char*>(payload->Data));
            bool accept = !canAcceptAssetFuncCallback ||
                           canAcceptAssetFuncCallback(am->getAssetById<MAsset>(droppedId));
            if (accept)
            {
                assetIdReference = droppedId;
                modified         = true;
            }
        }

        if (dragHovering && !dragValid) ImGui::PopStyleColor();
        ImGui::EndDragDropTarget();
    }

    // IsItemClicked fires on mouse-down; at that instant IsMouseDragging is
    // always false so this is a reliable click-not-drag check.
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && current)
    {
        auto* editorAM = dynamic_cast<MEditorAssetManager*>(MAssetManager::getInstance());
        if (editorAM) editorAM->pingAsset(current->getAssetId());
    }

    if (dragHovering)
    {
        ImU32 tint = dragValid ? IM_COL32(50, 180, 80, 100) : IM_COL32(180, 50, 50, 100);
        dl->AddRectFilled(p, {p.x + dropW, p.y + ROW_H}, tint, 4.f);
    }
    else if (hovered)
    {
        dl->AddRectFilled(p, {p.x + avail, p.y + ROW_H}, IM_COL32(255, 255, 255, 12), 4.f);
    }

    ImGui::SetCursorScreenPos({p.x + 2.f, p.y + 4.f});
    if (icon) ImGui::Image(*icon, {THUMB_SIZE, THUMB_SIZE});
    else      ImGui::Dummy({THUMB_SIZE, THUMB_SIZE});

    if (ImGui::IsItemHovered() && current)
        ImGui::SetTooltip("%s", current->getPath().c_str());

    const float nameX = p.x + THUMB_SIZE + 8.f;
    ImGui::SetCursorScreenPos({nameX, p.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f});

    if (current)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.f));
        ImGui::PushTextWrapPos(p.x + dropW - 2.f);
        ImGui::TextUnformatted(current->getName().c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }
    else
    {
        // Show the asset type hint while dragging so the user knows what to drop.
        ImGui::PushStyleColor(ImGuiCol_Text,
                              dragHovering ? ImVec4(0.8f, 0.8f, 0.8f, 1.f)
                                           : ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted(dragHovering
                               ? (dragValid ? "Drop here" : "Invalid type")
                               : "(none)");
        ImGui::PopStyleColor();
    }

    ImGui::SetCursorScreenPos({p.x + avail - CLEAR_BTN_W - 2.f,
                               p.y + (ROW_H - ImGui::GetFrameHeight()) * 0.5f});
    ImGui::BeginDisabled(!current);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.10f, 0.10f, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.40f, 0.05f, 0.05f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f));
    if (ImGui::Button("x", {CLEAR_BTN_W, ImGui::GetFrameHeight()}))
    {
        assetIdReference.clear();
        modified = true;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::PopID();

    // Advance the cursor past the row.
    ImGui::SetCursorScreenPos({p.x, p.y + ROW_H + 2.f});
    ImGui::Dummy({avail, 0.f});

    return modified;
}