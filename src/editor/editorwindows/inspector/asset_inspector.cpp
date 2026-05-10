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

std::map<SString, MMaterialPropertyControl*>     MAssetInspector::propControlCache;
std::map<SString, MAssetInspector::SCubemapFaceControls> MAssetInspector::cubemapFaceCache;

// ── Dispatch ──────────────────────────────────────────────────────────────────

void MAssetInspector::draw(MAsset* asset)
{
    if (!asset) return;

    if (auto* mat = dynamic_cast<MMaterialAsset*>(asset))
        drawMaterialAsset(mat);
    // MCubemapAsset inherits MTextureAsset — check cubemap FIRST
    else if (auto* cubemap = dynamic_cast<MCubemapAsset*>(asset))
        drawCubemapAsset(cubemap);
    else if (auto* tex = dynamic_cast<MTextureAsset*>(asset))
        drawTextureAsset(tex);
    else if (auto* text = dynamic_cast<MTextAsset*>(asset))
        drawTextAsset(text);
    else
        drawGenericAsset(asset);
}

// ── Material inspector (unchanged) ────────────────────────────────────────────

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
        if (!asset->save())
            ImGui::OpenPopup("##save_err");
    }

    if (ImGui::BeginPopup("##save_err"))
    {
        ImGui::TextUnformatted("Failed to save material. Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ── Texture inspector ─────────────────────────────────────────────────────────

// Label arrays for combo boxes — indices must match enum order.

static const char* filterMinLabels[] = {
    "Nearest", "Linear",
    "Nearest Mipmap Nearest", "Linear Mipmap Nearest",
    "Nearest Mipmap Linear",  "Linear Mipmap Linear"
};

static const char* filterMagLabels[] = { "Nearest", "Linear" };

static const char* wrapLabels[] = {
    "Repeat", "Mirrored Repeat", "Clamp to Edge", "Clamp to Border"
};

static const char* compressionLabels[] = { "None", "DXT1 (BC1, RGB)", "DXT5 (BC3, RGBA)" };

static const char* maxSizeLabels[] = {
    "No Limit", "4096", "2048", "1024", "512", "256", "128"
};
static const int maxSizeValues[] = { 0, 4096, 2048, 1024, 512, 256, 128 };
static constexpr int maxSizeCount = 7;

static int maxSizeToIndex(int val)
{
    for (int i = 0; i < maxSizeCount; ++i)
        if (maxSizeValues[i] == val) return i;
    return 0;
}

void MAssetInspector::drawTextureAsset(MTextureAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.5f, 1.f, 1.f));
    ImGui::TextUnformatted("Texture Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // ── Info table ────────────────────────────────────────────────────────
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

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Import settings ───────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LABEL_W = 120.f;
        if (ImGui::BeginTable("##tex_settings", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LABEL_W);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // ── Max Import Size ───────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Size:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int sizeIdx = maxSizeToIndex(asset->getMaxImportSize());
            if (ImGui::Combo("##maxSize", &sizeIdx, maxSizeLabels, maxSizeCount))
                asset->setMaxImportSize(maxSizeValues[sizeIdx]);

            // ── Min Filter ────────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Min Filter:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int minIdx = (int)asset->getFilterMin();
            if (ImGui::Combo("##filterMin", &minIdx, filterMinLabels, IM_ARRAYSIZE(filterMinLabels)))
                asset->setFilterMin((ETextureFilterMin)minIdx);

            // ── Mag Filter ────────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mag Filter:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int magIdx = (int)asset->getFilterMag();
            if (ImGui::Combo("##filterMag", &magIdx, filterMagLabels, IM_ARRAYSIZE(filterMagLabels)))
                asset->setFilterMag((ETextureFilterMag)magIdx);

            // ── Wrap S ────────────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Wrap S:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int wrapSIdx = (int)asset->getWrapS();
            if (ImGui::Combo("##wrapS", &wrapSIdx, wrapLabels, IM_ARRAYSIZE(wrapLabels)))
                asset->setWrapS((ETextureWrap)wrapSIdx);

            // ── Wrap T ────────────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Wrap T:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int wrapTIdx = (int)asset->getWrapT();
            if (ImGui::Combo("##wrapT", &wrapTIdx, wrapLabels, IM_ARRAYSIZE(wrapLabels)))
                asset->setWrapT((ETextureWrap)wrapTIdx);

            // ── Compression ───────────────────────────────────────────────
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Compression:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int compIdx = (int)asset->getCompression();
            if (ImGui::Combo("##compression", &compIdx, compressionLabels, IM_ARRAYSIZE(compressionLabels)))
                asset->setCompression((ETextureCompression)compIdx);

            ImGui::EndTable();
        }

        ImGui::Spacing();

        if (ImGui::Button("Save & Reimport##tex_save", ImVec2(-FLT_MIN, 0)))
        {
            if (asset->saveImportSettings())
            {
                asset->requestReload();
            }
            else
            {
                ImGui::OpenPopup("##tex_save_err");
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save import settings to .meta and reload texture");

        if (ImGui::BeginPopup("##tex_save_err"))
        {
            ImGui::TextUnformatted("Failed to save import settings. Check logs.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
}

// ── Cubemap inspector ─────────────────────────────────────────────────────────

static bool canAcceptTextureOnly(MAsset* asset)
{
    // Accept MTextureAsset but NOT MCubemapAsset (which inherits from it)
    if (!asset) return false;
    if (dynamic_cast<MCubemapAsset*>(asset)) return false;
    return dynamic_cast<MTextureAsset*>(asset) != nullptr;
}

void MAssetInspector::drawCubemapAsset(MCubemapAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.8f, 0.9f, 1.f));
    ImGui::TextUnformatted("Cubemap Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // ── Info ──────────────────────────────────────────────────────────────
    constexpr float LW = 100.f;
    if (ImGui::BeginTable("##cube_info", 2, ImGuiTableFlags_None))
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

        ImGui::EndTable();
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Face Textures", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const SString& id = asset->getAssetId();

        // Lazy-create controls for this cubemap
        if (!cubemapFaceCache.contains(id))
        {
            auto& entry = cubemapFaceCache[id];
            auto* am = MAssetManager::getInstance();
            for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
            {
                entry.faces[i] = new MAssetReferenceControl();
                entry.faces[i]->canAcceptAssetFuncCallback = canAcceptTextureOnly;

                // Seed with current face texture if it exists
                SString facePath = asset->getFacePath(i);
                if (!facePath.empty())
                {
                    auto* faceAsset = am->getAsset<MTextureAsset>(facePath);
                    if (faceAsset)
                        entry.faces[i]->setAssetReference(faceAsset);
                }
            }
        }

        auto& controls = cubemapFaceCache[id];

        // Human-readable labels for each face
        static const char* faceDisplayNames[6] = {
            "Right (+X)", "Left (-X)", "Top (+Y)",
            "Bottom (-Y)", "Back (+Z)", "Front (-Z)"
        };

        constexpr float LABEL_W = 100.f;
        if (ImGui::BeginTable("##cube_faces", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LABEL_W);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(faceDisplayNames[i]);
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                ImGui::PushID(i);
                controls.faces[i]->drawControl("");
                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();

        if (ImGui::Button("Save & Rebuild##cube_save", ImVec2(-FLT_MIN, 0)))
        {
            // Update face paths from the reference controls
            for (int i = 0; i < MCubemapAsset::FACE_COUNT; ++i)
            {
                auto* ref = controls.faces[i]->getAssetReference();
                asset->setFacePath(i, ref ? ref->getPath() : SString(""));
            }

            if (asset->save())
            {
                asset->requestReload();
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
}

void MAssetInspector::drawTextAsset(MTextAsset* asset)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 0.6f, 1.f));
    ImGui::TextUnformatted("Text Asset");
    ImGui::PopStyleColor();
    ImGui::Separator();

    ImGui::Text("Name: %s", asset->getName().c_str());
    ImGui::Text("Path: %s", asset->getPath().c_str());
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