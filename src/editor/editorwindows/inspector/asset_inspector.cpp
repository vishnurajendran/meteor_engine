// asset_inspector.cpp

#include "asset_inspector.h"
#include "imgui.h"
#include "imgui-SFML.h"
#include "core/engine/assetmanagement/asset/asset.h"
#include "core/graphics/core/material/MMaterialAsset.h"
#include "core/graphics/core/material/material.h"
#include "core/engine/texture/textureasset.h"
#include "core/engine/assetmanagement/textasset/textasset.h"
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
    else if (auto* txt = dynamic_cast<MTextAsset*>(asset))
        drawTextAsset(txt);
    else
        drawGenericAsset(asset);
}

// ── Material ──────────────────────────────────────────────────────────────────

void MAssetInspector::drawMaterialAsset(MMaterialAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.3f, 1.f));
    SString matTitle = asset->isDirty()
        ? SString("Material Asset *")
        : SString("Material Asset");
    ImGui::TextUnformatted(matTitle.c_str());
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

    const bool saved = ImGui::Button("Save##mat_save", ImVec2(-FLT_MIN, 0));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write current properties back to %s", asset->getPath().c_str());

    if (saved)
    {
        if (asset->save())
            asset->clearDirty();
        else
            ImGui::OpenPopup("##save_err");
    }

    if (ImGui::BeginPopup("##save_err"))
    {
        ImGui::TextUnformatted("Failed to save material. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── Texture ───────────────────────────────────────────────────────────────────

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

    // ── Preview ───────────────────────────────────────────────────────────
    auto* tex = asset->getTexture();
    if (tex && tex->getCoreTexture())
    {
        auto sfSize    = tex->getCoreTexture()->getSize();
        float avail    = ImGui::GetContentRegionAvail().x;
        float aspect   = sfSize.x > 0 ? (float)sfSize.y / (float)sfSize.x : 1.f;
        float previewW = std::min(avail, 512.f);
        float previewH = previewW * aspect;

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

    // ── Import Settings ───────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LABEL_W = 120.f;
        bool changed = false;

        if (ImGui::BeginTable("##tex_import", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LABEL_W);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // ── Min Filter ────────────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Min Filter:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                static const char* filterMinNames[] = {
                    "Nearest", "Linear",
                    "Nearest Mipmap Nearest", "Linear Mipmap Nearest",
                    "Nearest Mipmap Linear",  "Linear Mipmap Linear"
                };
                int cur = (int)asset->getFilterMin();
                if (ImGui::Combo("##filterMin", &cur, filterMinNames, IM_ARRAYSIZE(filterMinNames)))
                {
                    asset->setFilterMin((ETextureFilterMin)cur);
                    changed = true;
                }
            }

            // ── Mag Filter ────────────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Mag Filter:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                static const char* filterMagNames[] = { "Nearest", "Linear" };
                int cur = (int)asset->getFilterMag();
                if (ImGui::Combo("##filterMag", &cur, filterMagNames, IM_ARRAYSIZE(filterMagNames)))
                {
                    asset->setFilterMag((ETextureFilterMag)cur);
                    changed = true;
                }
            }

            // ── Wrap S ────────────────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Wrap S:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                static const char* wrapNames[] = {
                    "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
                };
                int cur = (int)asset->getWrapS();
                if (ImGui::Combo("##wrapS", &cur, wrapNames, IM_ARRAYSIZE(wrapNames)))
                {
                    asset->setWrapS((ETextureWrap)cur);
                    changed = true;
                }
            }

            // ── Wrap T ────────────────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Wrap T:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                static const char* wrapNames[] = {
                    "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
                };
                int cur = (int)asset->getWrapT();
                if (ImGui::Combo("##wrapT", &cur, wrapNames, IM_ARRAYSIZE(wrapNames)))
                {
                    asset->setWrapT((ETextureWrap)cur);
                    changed = true;
                }
            }

            // ── Compression ───────────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Compression:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                static const char* compNames[] = { "None", "DXT1 (BC1)", "DXT5 (BC3)" };
                int cur = (int)asset->getCompression();
                if (ImGui::Combo("##compression", &cur, compNames, IM_ARRAYSIZE(compNames)))
                {
                    asset->setCompression((ETextureCompression)cur);
                    changed = true;
                }
            }

            // ── Max Import Size ───────────────────────────────────────────
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Max Size:");
                ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);

                int maxSize = asset->getMaxImportSize();
                if (ImGui::InputInt("##maxSize", &maxSize, 0, 0))
                {
                    if (maxSize < 0) maxSize = 0;
                    asset->setMaxImportSize(maxSize);
                    changed = true;
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("0 = no limit. Common values: 512, 1024, 2048, 4096");
            }

            ImGui::EndTable();
        }

        if (changed)
            asset->markDirty();

        ImGui::Spacing();

        if (ImGui::Button("Apply & Save##tex_save", ImVec2(-FLT_MIN, 0)))
        {
            if (asset->saveImportSettings())
            {
                asset->requestReload();
                asset->clearDirty();
                MLOG(SString::format("Texture import settings saved: {0}", asset->getPath()));
            }
            else
            {
                ImGui::OpenPopup("##tex_save_err");
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save settings to .meta and re-import the texture");

        if (ImGui::BeginPopup("##tex_save_err"))
        {
            ImGui::TextUnformatted("Failed to save import settings. Check logs.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}

// ── Text ──────────────────────────────────────────────────────────────────────

void MAssetInspector::drawTextAsset(MTextAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.f, 1.f));
    ImGui::TextUnformatted("Text Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
}

// ── Generic ───────────────────────────────────────────────────────────────────

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