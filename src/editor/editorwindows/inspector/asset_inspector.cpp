// asset_inspector.cpp

#include "asset_inspector.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/engine/texture/textureasset.h"
#include "editor/editorwindows/inspectordrawer/controls/material_properties_controls.h"
#include "core/utils/logger.h"

std::map<SString, MMaterialPropertyControl*> MAssetInspector::propControlCache;

void MAssetInspector::draw(MAsset* asset)
{
    if (!asset) return;

    if (auto* mat = dynamic_cast<MMaterialAsset*>(asset))
        drawMaterialAsset(mat);
    else if (auto* tex = dynamic_cast<MTextureAsset*>(asset))
        drawTextureAsset(tex);
    else
        drawGenericAsset(asset);
}

void MAssetInspector::drawMaterialAsset(MMaterialAsset* asset)
{
    // Header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.f));
    ImGui::TextUnformatted("Material Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##mat_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:",   asset->getName().c_str());
        row("Shader:", asset->getShaderPath().c_str());
        row("Mode:",   asset->getShadingMode() == MMaterial::ShadingMode::Lit ? "Lit" : "Unlit");
        row("Path:",   asset->getPath().c_str());

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Check buildability without storing the pointer — getMaterial() is called
    // fresh inside ctrl->draw(asset) each frame to avoid dangling pointers.
    if (!asset->getMaterial())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("Material not built yet (shader may be missing).");
        ImGui::PopStyleColor();
        return;
    }

    const SString& id = asset->getAssetId();
    if (!propControlCache.contains(id))
        propControlCache[id] = new MMaterialPropertyControl();

    auto* ctrl = propControlCache[id];

    // Flush texture reference controls when property count changes (hot reload).
    const int propCount = (int)asset->getMaterial()->getProperties().size();
    if (ctrl->getLastPropertyCount() != propCount)
    {
        ctrl->clearTextureReferences();
        ctrl->setLastPropertyCount(propCount);
    }

    // Pass the asset — draw() calls getMaterial() fresh internally.
    ctrl->draw(asset);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool saved = ImGui::Button("Save##mat_save", ImVec2(-FLT_MIN, 0));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write current properties back to %s", asset->getPath().c_str());

    if (saved)
    {
        if (!asset->save())
        {
            ImGui::OpenPopup("##save_err");
        }
    }

    if (ImGui::BeginPopup("##save_err"))
    {
        ImGui::TextUnformatted("Failed to save material. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void MAssetInspector::drawTextureAsset(MTextureAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.5f, 1.f, 1.f));
    ImGui::TextUnformatted("Texture Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##tex_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
        ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

        auto row = [&](const char* label, const char* value) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.f));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(value);
        };

        row("Name:", asset->getName().c_str());
        row("Path:", asset->getPath().c_str());

        auto* tex = asset->getTexture();
        if (tex && tex->getCoreTexture())
        {
            auto size = tex->getCoreTexture()->getSize();
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%u × %u", size.x, size.y);
            row("Size:", buf);
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Preview — fits to the available width while keeping aspect ratio.
    auto* tex = asset->getTexture();
    if (tex && tex->getCoreTexture())
    {
        auto sfSize    = tex->getCoreTexture()->getSize();
        float avail    = ImGui::GetContentRegionAvail().x;
        float aspect   = sfSize.x > 0 ? (float)sfSize.y / (float)sfSize.x : 1.f;
        float previewW = std::min(avail, 512.f);
        float previewH = previewW * aspect;

        // Subtle border
        ImVec2 tl = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(tl.x - 1, tl.y - 1),
            ImVec2(tl.x + previewW + 1, tl.y + previewH + 1),
            IM_COL32(60, 60, 60, 255));

        ImGui::Image(*tex->getCoreTexture(), ImVec2(previewW, previewH));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.4f, 0.4f, 1.f));
        ImGui::TextUnformatted("Texture not loaded.");
        ImGui::PopStyleColor();
    }
}

void MAssetInspector::drawGenericAsset(MAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.f));
    ImGui::TextUnformatted("Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
    ImGui::Text("ID:   %s", asset->getAssetId().c_str());
}