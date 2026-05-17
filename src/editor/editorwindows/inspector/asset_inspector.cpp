// asset_inspector.cpp

#include "asset_inspector.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/skybox/cubemapasset.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/material_properties_controls.h"
#include "core/utils/logger.h"

std::map<SString, MMaterialPropertyControl*> MAssetInspector::propControlCache;
std::map<SString, MAssetInspector::SCubemapFaceControls> MAssetInspector::cubemapFaceCache;

// -- dispatch -----------------------------------------------------------------

void MAssetInspector::draw(MAsset* asset)
{
    if (!asset) return;

    if (auto* mat = dynamic_cast<MMaterialAsset*>(asset))
        drawMaterialAsset(mat);
    else if (auto* cube = dynamic_cast<MCubemapAsset*>(asset))
        drawCubemapAsset(cube);
    else if (auto* tex = dynamic_cast<MTextureAsset*>(asset))
        drawTextureAsset(tex);
    else if (auto* txt = dynamic_cast<MTextAsset*>(asset))
        drawTextAsset(txt);
    else
        drawGenericAsset(asset);
}

// -- material -----------------------------------------------------------------

void MAssetInspector::drawMaterialAsset(MMaterialAsset* asset)
{
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

    const int propCount = (int)asset->getMaterial()->getProperties().size();
    if (ctrl->getLastPropertyCount() != propCount)
    {
        ctrl->clearTextureReferences();
        ctrl->setLastPropertyCount(propCount);
    }

    ctrl->draw(asset);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Save##mat_save", ImVec2(-FLT_MIN, 0)))
    {
        if (!asset->save())
            ImGui::OpenPopup("##save_err");
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write current properties back to %s", asset->getPath().c_str());

    if (ImGui::BeginPopup("##save_err"))
    {
        ImGui::TextUnformatted("Failed to save material. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// -- texture ------------------------------------------------------------------

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
            std::snprintf(buf, sizeof(buf), "%u x %u", size.x, size.y);
            row("Size:", buf);
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    auto* tex = asset->getTexture();
    if (tex && tex->getCoreTexture())
    {
        auto sfSize  = tex->getCoreTexture()->getSize();
        float avail  = ImGui::GetContentRegionAvail().x;
        float aspect = sfSize.x > 0 ? (float)sfSize.y / (float)sfSize.x : 1.f;
        float w      = std::min(avail, 256.f);
        ImGui::Image(*tex->getCoreTexture(), { w, w * aspect });
    }
}

// -- cubemap ------------------------------------------------------------------

void MAssetInspector::drawCubemapAsset(MCubemapAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 0.9f, 1.f));
    ImGui::TextUnformatted("Cubemap Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##cube_info", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LW);
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

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // -- Face reference controls ----------------------------------------------

    const SString& id = asset->getAssetId();
    if (!cubemapFaceCache.contains(id))
    {
        SCubemapFaceControls ctrls;
        for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
        {
            ctrls.faces[i] = new MAssetReferenceControl();
            ctrls.faces[i]->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> a)
            { return dynamic_cast<MTextureAsset*>(a.get()) != nullptr; };
        }
        cubemapFaceCache[id] = ctrls;
    }

    auto& ctrls = cubemapFaceCache[id];

    static const char* faceDisplayNames[] = {
        "Right (+X):", "Left (-X):", "Top (+Y):",
        "Bottom (-Y):", "Back (+Z):", "Front (-Z):"
    };

    if (ImGui::CollapsingHeader("Cubemap Faces", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("##cube_faces", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed, LW);
            ImGui::TableSetupColumn("v", ImGuiTableColumnFlags_WidthStretch);

            for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
            {
                // Sync the control with the asset's current face path.
                SString facePath = asset->getFacePath(i);
                TAssetHandle<MAsset> expected = !facePath.empty()
                    ? MAssetManager::getInstance()->getAsset<MTextureAsset>(facePath)
                    : TAssetHandle<MTextureAsset>();
                if (ctrls.faces[i]->getAssetReference() != expected)
                    ctrls.faces[i]->setAssetReference(expected);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(faceDisplayNames[i]);
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                ImGui::PushID(i);
                if (ctrls.faces[i]->drawControl(""))
                {
                    const auto newRef = ctrls.faces[i]->getAssetReference();
                    asset->setFacePath(i,
                        newRef ? newRef->getPath() : SString(""));
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // -- Save & Rebuild -------------------------------------------------------

    if (ImGui::Button("Save & Rebuild##cube_save", ImVec2(-FLT_MIN, 0)))
    {
        if (asset->save())
        {
            if (!asset->requestReload())
                MWARN("Cubemap rebuild failed -- check face paths in the log.");
        }
        else
        {
            ImGui::OpenPopup("##cube_save_err");
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Save cubemap XML and rebuild GPU cubemap texture");

    if (ImGui::BeginPopup("##cube_save_err"))
    {
        ImGui::TextUnformatted("Failed to save cubemap. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// -- text ---------------------------------------------------------------------

void MAssetInspector::drawTextAsset(MTextAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 0.6f, 1.f));
    ImGui::TextUnformatted("Text Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
}

// -- generic fallback ---------------------------------------------------------

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