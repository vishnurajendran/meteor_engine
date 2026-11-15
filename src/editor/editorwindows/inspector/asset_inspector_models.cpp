// asset_inspector_models.cpp
//
// MAssetInspector sections for model files: the import settings shared by
// static meshes, skeletal meshes and animation clip assets, the skeletal and
// clip asset views, and the entity swap prompt after importAs changes.

#include "asset_inspector.h"

#include <cstdio>

#include "core/engine/3d/model/model_import_settings.h"
#include "core/engine/3d/skeletal/anim_clip.h"
#include "core/engine/3d/skeletal/anim_clip_asset.h"
#include "core/engine/3d/skeletal/skeletal_mesh.h"
#include "core/engine/3d/skeletal/skeletal_mesh_asset.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/utils/logger.h"
#include "editor/editor_utils/mesh_entity_swapper.h"
#include "editor/editorassetmanager/editorassetmanager.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "imgui.h"

SString MAssetInspector::pendingSwapAssetId;

namespace
{
    const char* IMPORT_AS_LABELS[] = { "Auto (detect)", "Static Mesh", "Skeletal Mesh", "Animation Only" };
    const char* PIVOT_LABELS[]     = { "File Origin", "Center", "Bottom Center" };

    EModelImportAs classOf(const MAsset* asset)
    {
        if (dynamic_cast<const MSkeletalMeshAsset*>(asset)) return EModelImportAs::SkeletalMesh;
        if (dynamic_cast<const MAnimClipAsset*>(asset))     return EModelImportAs::AnimationOnly;
        return EModelImportAs::StaticMesh;
    }

    const char* entityTypeFor(const MAsset* asset)
    {
        if (dynamic_cast<const MSkeletalMeshAsset*>(asset)) return "Skeletal Mesh";
        if (dynamic_cast<const MStaticMeshAsset*>(asset))   return "Static Mesh";
        return "matching";
    }

    void formatCount(char* buf, size_t size, long long n)
    {
        if      (n >= 1'000'000) std::snprintf(buf, size, "%.1fM", static_cast<double>(n) / 1e6);
        else if (n >= 10'000)    std::snprintf(buf, size, "%.1fK", static_cast<double>(n) / 1e3);
        else                     std::snprintf(buf, size, "%lld", n);
    }

    void statRow(const char* label, const char* value)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(value);
    }

    void drawTitle(const char* title, const MAsset* asset)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.75f, 0.4f, 1.f));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Text("Name:  %s", asset->getName().c_str());
        ImGui::Text("Path:  %s", asset->getPath().c_str());
        ImGui::Spacing();
    }

    void drawClipList(const std::vector<MAnimClip*>& clips)
    {
        char header[48];
        std::snprintf(header, sizeof(header), "Clips (%d)###clip_list", static_cast<int>(clips.size()));
        if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
            return;
        if (clips.empty())
        {
            ImGui::TextDisabled("No clips");
            return;
        }
        if (!ImGui::BeginTable("##clips", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            return;
        ImGui::TableSetupColumn("Name",     ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Tracks",   ImGuiTableColumnFlags_WidthStretch, 0.7f);
        ImGui::TableHeadersRow();
        for (const auto* clip : clips)
        {
            if (!clip) continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(clip->getName().c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%.2f s", clip->getDuration());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%d", clip->getTrackCount());
        }
        ImGui::EndTable();
    }
}

// -- Import settings ------------------------------------------------------------------

bool MAssetInspector::drawModelImportSettings(MAsset* asset, SModelImportSettings& settings)
{
    if (!ImGui::CollapsingHeader("Import Settings", ImGuiTreeNodeFlags_DefaultOpen))
        return false;

    const EModelImportAs current = classOf(asset);
    const bool skeletalOptions = settings.importAs == EModelImportAs::SkeletalMesh
                              || settings.importAs == EModelImportAs::AnimationOnly
                              || settings.importAs == EModelImportAs::Auto;

    if (ImGui::BeginTable("##model_import", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 130.f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Import As");
        ImGui::TableSetColumnIndex(1);
        int importAs = static_cast<int>(settings.importAs);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##import_as", &importAs, IMPORT_AS_LABELS, IM_ARRAYSIZE(IMPORT_AS_LABELS)))
            settings.importAs = static_cast<EModelImportAs>(importAs);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Static Mesh: one mesh, no bones - a rigged file imports in bind pose.\n"
                "Skeletal Mesh: meshes, skeleton and the file's clips.\n"
                "Animation Only: every clip in the file, for use on other characters.\n"
                "Changing it changes the asset type: Save & Reimport recreates the asset.");

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Scale Factor");
        ImGui::TableSetColumnIndex(1);
        float scale = settings.scaleFactor;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##model_scale", &scale, 0.005f, 0.0001f, 10000.0f, "%.4g",
                             ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
            settings.setScaleFactor(scale);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Uniform scale baked in when the model loads, so entities can keep a scale of 1.\n"
                "Applies to vertices, bone translations and clip translation keys alike.\n"
                "Common values: 0.01 (cm -> m), 0.0254 (inches), 0.001 (mm).\n"
                "FBX files are already converted from centimetres on import.");

        if (settings.importAs != EModelImportAs::AnimationOnly)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Pivot");
            ImGui::TableSetColumnIndex(1);
            int pivot = static_cast<int>(settings.pivot);
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::Combo("##model_pivot", &pivot, PIVOT_LABELS, IM_ARRAYSIZE(PIVOT_LABELS)))
                settings.pivot = static_cast<EMeshPivot>(pivot);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "Where the model's origin goes.\n"
                    "File Origin: as authored. Center: middle of the bounding box.\n"
                    "Bottom Center: middle of the base - for things that stand on the ground.\n"
                    "Skeletal meshes move the skeleton's root instead of the vertices.");
        }

        if (skeletalOptions && settings.importAs != EModelImportAs::AnimationOnly)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Bone Influences");
            ImGui::TableSetColumnIndex(1);
            int influences = settings.maxBoneInfluences;
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragInt("##model_influences", &influences, 0.1f, 0, 64, influences == 0 ? "no limit" : "%d"))
                settings.setMaxBoneInfluences(influences);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Per vertex: the strongest are kept and renormalized. 0 = keep all.\n"
                                  "Skinning cost follows each vertex's own count, so a high limit only\n"
                                  "costs where the mesh really uses it.");
        }

        if (skeletalOptions)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Sample Rate");
            ImGui::TableSetColumnIndex(1);
            float rate = settings.sampleRate;
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##model_rate", &rate, 0.5f, 1.0f, 240.0f, "%.0f keys/s", ImGuiSliderFlags_AlwaysClamp))
                settings.setSampleRate(rate);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Clips are resampled to evenly spaced keys at this rate.\n"
                                  "Higher is smoother for fast motion and uses more memory.");
        }

        ImGui::EndTable();
    }

    const bool changesClass = settings.importAs != current;
    if (changesClass)
        MInspectorField::statusText(EInspectorStatus::Warning, "Save & Reimport changes the asset type",
                                    "The asset is recreated. Entities that use it as the old type stop drawing -\n"
                                    "you will be offered to swap them for the matching entity type.");

    ImGui::Spacing();
    bool recreated = false;
    if (ImGui::Button("Save & Reimport##model_save", ImVec2(-FLT_MIN, 0)))
    {
        if (!settings.saveToMeta(asset->getPath()))
        {
            ImGui::OpenPopup("##model_save_err");
        }
        else
        {
            auto* editorAM = dynamic_cast<MEditorAssetManager*>(MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>());
            const SString assetId = asset->getAssetId();
            const SString path    = asset->getPath();
            if (changesClass && editorAM)
            {
                // A different class: destroy and import again (same GUID).
                if (!editorAM->reimportAsset(path))
                    MWARN("Model reimport failed for: " + path);
                else if (MMeshEntitySwapper::countMismatched(assetId) > 0)
                    pendingSwapAssetId = assetId;
                recreated = true;   // `asset` is gone
            }
            else if (editorAM)
            {
                if (!editorAM->reloadAsset(asset))
                    MWARN("Model reimport failed for: " + path);
                editorAM->invalidateThumbnail(asset);
            }
        }
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write import settings to .meta and reload the model");

    if (ImGui::BeginPopup("##model_save_err"))
    {
        ImGui::TextUnformatted("Failed to save import settings (read-only source?). Check logs.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    return recreated;
}

// -- Entity swap prompt ---------------------------------------------------------------

void MAssetInspector::drawEntitySwapPrompt()
{
    if (pendingSwapAssetId.empty())
        return;

    constexpr const char* POPUP = "Swap Entities?##model_swap";
    if (!ImGui::IsPopupOpen(POPUP))
        ImGui::OpenPopup(POPUP);

    if (ImGui::BeginPopupModal(POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto* am = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>();
        MAsset* asset = am ? am->getAssetById<MAsset>(pendingSwapAssetId).get() : nullptr;
        const int count = MMeshEntitySwapper::countMismatched(pendingSwapAssetId);

        if (!asset || count == 0)
        {
            pendingSwapAssetId.clear();
            ImGui::CloseCurrentPopup();
        }
        else
        {
            ImGui::Text("%d %s in the open scene still use '%s' as its old type,",
                        count, count == 1 ? "entity" : "entities", asset->getPath().c_str());
            ImGui::TextUnformatted("so they no longer draw.");
            ImGui::Text("Swap them for %s entities? Name, transform, materials, script and", entityTypeFor(asset));
            ImGui::TextUnformatted("children are kept.");
            ImGui::Spacing();
            if (ImGui::Button("Swap", ImVec2(120, 0)))
            {
                MMeshEntitySwapper::swapMismatched(pendingSwapAssetId);
                pendingSwapAssetId.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Keep As Is", ImVec2(120, 0)))
            {
                pendingSwapAssetId.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
}

// -- Skeletal mesh ----------------------------------------------------------------------

void MAssetInspector::drawSkeletalMeshAsset(MSkeletalMeshAsset* asset)
{
    drawTitle("Skeletal Mesh Asset", asset);

    long long vertices = 0, triangles = 0, influences = 0;
    for (const auto* mesh : asset->getMeshes())
    {
        if (!mesh) continue;
        vertices   += mesh->getVertexCount();
        triangles  += mesh->getIndexCount() / 3;
        influences += static_cast<long long>(mesh->getInfluences().size());
    }
    const auto& skeleton = asset->getSkeleton();
    int deform = 0;
    for (const auto& bone : skeleton.bones) deform += bone.isDeformBone ? 1 : 0;

    if (ImGui::BeginTable("##skeletal_stats", 2, ImGuiTableFlags_None))
    {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 130.f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
        char buf[64];

        std::snprintf(buf, sizeof(buf), "%d", asset->getSubMeshCount());
        statRow("Submeshes", buf);
        formatCount(buf, sizeof(buf), vertices);
        statRow("Vertices", buf);
        formatCount(buf, sizeof(buf), triangles);
        statRow("Triangles", buf);
        std::snprintf(buf, sizeof(buf), "%.2f", vertices > 0 ? static_cast<double>(influences) / static_cast<double>(vertices) : 0.0);
        statRow("Influences / Vertex", buf);
        std::snprintf(buf, sizeof(buf), "%d deform (%d nodes)", deform, skeleton.getBoneCount());
        statRow("Bones", buf);
        statRow("Root Bone", skeleton.rootBone >= 0 ? skeleton.bones[skeleton.rootBone].name.c_str() : "(none)");
        const SVector3 size = asset->getBoundsMax() - asset->getBoundsMin();
        std::snprintf(buf, sizeof(buf), "%.3g x %.3g x %.3g", size.x, size.y, size.z);
        statRow("Size", buf);
        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (drawModelImportSettings(asset, asset->getImportSettings()))
        return;   // recreated

    ImGui::Spacing();
    drawClipList(asset->getClips());
}

// -- Animation clips ----------------------------------------------------------------------

void MAssetInspector::drawAnimClipAsset(MAnimClipAsset* asset)
{
    drawTitle("Animation Clip Asset", asset);
    ImGui::TextDisabled("Add it to a Skeletal Mesh entity's Additional Animation Sources.");
    ImGui::TextDisabled("Clips bind by exact bone name.");

    ImGui::Spacing();
    if (drawModelImportSettings(asset, asset->getImportSettings()))
        return;   // recreated

    ImGui::Spacing();
    drawClipList(asset->getClips());
}
