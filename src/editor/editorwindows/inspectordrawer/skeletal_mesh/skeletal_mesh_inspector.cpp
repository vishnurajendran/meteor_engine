#include "skeletal_mesh_inspector.h"

#include <algorithm>
#include <cstdio>

#include "core/application/application.h"
#include "core/engine/3d/skeletal/skeletal_mesh_asset.h"
#include "core/engine/3d/skeletal/skeletal_mesh_entity.h"
#include "core/memory/mnew.h"
#include "core/utils/fileio.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"

bool MSkeletalMeshInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MSkeletalMeshInspectorDrawer>());
    return true;
}();

namespace
{
    const ImVec4 COLOR_ERROR(0.95f, 0.35f, 0.35f, 1.0f);

    const char* CULLING_LABELS[] = { "Advance Time Only", "Pause", "Always Animate" };

    SString fileName(const SString& path) { return FileIO::getFileName(path); }

    bool isSimulating()
    {
        const MApplication* app = MApplication::getAppInstance();
        return app && app->isSimulating();
    }
}

MSkeletalMeshInspectorDrawer::MSkeletalMeshInspectorDrawer()
{
    meshAssetControl = mnew<MAssetReferenceControl>();
    meshAssetControl->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
    { return dynamic_cast<MSkeletalMeshAsset*>(asset.get()) != nullptr; };
}

bool MSkeletalMeshInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MSkeletalMeshEntity*>(entity) != nullptr;
}

void MSkeletalMeshInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* entity = dynamic_cast<MSkeletalMeshEntity*>(target);
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    if (!entity)
        return;

    drawMeshSection(entity);
    drawSourcesSection(entity);
    drawClipsSection(entity);
    drawPreviewSection(entity);
    materialSlots.draw(entity);
}

// -- Mesh, default clip, playback settings ---------------------------------------------

void MSkeletalMeshInspectorDrawer::drawMeshSection(MSkeletalMeshEntity* entity)
{
    if (!ImGui::CollapsingHeader("Skeletal Mesh", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    const auto current = entity->getSkeletalMeshAsset();
    if (meshAssetControl->getAssetReference().getAssetId() != current.getAssetId())
        meshAssetControl->setAssetReference(current);

    START_FIELD_DRAW("Skeletal Mesh")
        if (meshAssetControl->drawControl(""))
        {
            TAssetHandle<MSkeletalMeshAsset> meshRef = meshAssetControl->getAssetReference();
            entity->setSkeletalMeshAsset(meshRef);
        }
    END_FIELD_DRAW()

    // Default clip: available clips only. A saved name that is no longer
    // available is kept and shown in red.
    {
        const auto names    = entity->getClipNames();
        SString    selected = entity->defaultClip.get();
        const bool missing  = !selected.empty() && std::find(names.begin(), names.end(), selected) == names.end();

        START_FIELD_DRAW("Default Clip", "Played on start when Play On Start is set. Available clips only.")
            if (missing) ImGui::PushStyleColor(ImGuiCol_Text, COLOR_ERROR);
            const bool open = ImGui::BeginCombo("##default_clip", selected.empty() ? "(none)" : selected.c_str());
            if (missing)
            {
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("'%s' is not an available clip - check the Available Clips table", selected.c_str());
            }
            if (open)
            {
                if (ImGui::Selectable("(none)", selected.empty()))
                    entity->defaultClip = SString();
                for (const auto& name : names)
                    if (ImGui::Selectable(name.c_str(), name == selected))
                        entity->defaultClip = name;
                ImGui::EndCombo();
            }
        END_FIELD_DRAW()
    }

    START_FIELD_DRAW("Play On Start")
        bool v = entity->playOnStart.get();
        if (ImGui::Checkbox("##v", &v)) entity->playOnStart = v;
    END_FIELD_DRAW()

    START_FIELD_DRAW("Loop", "Whether the default clip loops")
        bool v = entity->loop.get();
        if (ImGui::Checkbox("##v", &v)) entity->loop = v;
    END_FIELD_DRAW()

    START_FIELD_DRAW("Speed")
        float v = entity->getSpeed();
        if (MInspectorField::DragFloat("##v", &v, 0.01f, -10.0f, 10.0f, "%.2f"))
            entity->setSpeed(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Culling Mode",
                     "What happens while the character is outside every camera:\n"
                     "Advance Time Only - clip time and onClipFinished keep going; no bones or skinning.\n"
                     "Pause - nothing updates; playback resumes where it stopped.\n"
                     "Always Animate - full evaluation and skinning, as if visible.")
        int mode = static_cast<int>(entity->getCullingMode());
        if (ImGui::Combo("##v", &mode, CULLING_LABELS, IM_ARRAYSIZE(CULLING_LABELS)))
            entity->setCullingMode(static_cast<EAnimationCullingMode>(mode));
    END_FIELD_DRAW()

    START_FIELD_DRAW("Cast Shadow")
        bool v = entity->getCastsShadow();
        if (ImGui::Checkbox("##v", &v)) entity->setCastsShadow(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Show Mesh Bounds")
        bool v = entity->getDrawGizmo();
        if (ImGui::Checkbox("##v", &v)) entity->setDrawGizmo(v);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Show Skeleton")
        bool v = entity->getDrawSkeleton();
        if (ImGui::Checkbox("##v", &v)) entity->setDrawSkeleton(v);
    END_FIELD_DRAW()
}

// -- Additional animation sources -------------------------------------------------------

void MSkeletalMeshInspectorDrawer::drawSourcesSection(MSkeletalMeshEntity* entity)
{
    const int count = entity->getAnimationSourceCount();
    char header[64];
    std::snprintf(header, sizeof(header), "Additional Animation Sources (%d)###anim_sources", count);
    if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        return;

    while (static_cast<int>(sourceControls.size()) < count)
    {
        auto* control = mnew<MAssetReferenceControl>();
        control->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
        { return MSkeletalMeshEntity::isAnimationSourceAsset(asset.get()); };
        sourceControls.push_back(control);
    }

    const auto& table = entity->getClipTable();
    int removeIndex = -1, moveFrom = -1, moveTo = -1;

    for (int i = 0; i < count; ++i)
    {
        ImGui::PushID(i);
        const auto& ref = entity->getAnimationSource(i);
        MAsset* asset   = ref.resolve();

        // Up / down - order matters: for clips with the same name, later wins.
        ImGui::BeginDisabled(i == 0);
        if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { moveFrom = i; moveTo = i - 1; }
        ImGui::EndDisabled();
        ImGui::SameLine(0, 2);
        ImGui::BeginDisabled(i == count - 1);
        if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { moveFrom = i; moveTo = i + 1; }
        ImGui::EndDisabled();
        ImGui::SameLine();

        auto* control = sourceControls[i];
        if (control->getAssetId() != ref.getAssetId())
            control->setAssetId(ref.getAssetId());

        const float removeWidth = MInspectorField::getResetButtonWidth();
        ImGui::BeginGroup();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - removeWidth - ImGui::GetStyle().ItemSpacing.x);
        if (control->drawCompactControl("##source"))
            entity->setAnimationSource(i, TAssetRef<MAsset>(control->getAssetReference()));
        ImGui::PopItemWidth();
        ImGui::EndGroup();
        ImGui::SameLine();
        if (MInspectorField::RemoveButton("##remove", "Remove this source"))
            removeIndex = i;

        // Status of the source's clips.
        if (!asset)
        {
            if (!ref.isEmpty())
                MInspectorField::statusText(EInspectorStatus::Error, "missing asset");
        }
        else
        {
            int usable = 0, total = 0;
            for (const auto& entry : table)
            {
                if (entry.sourcePath != asset->getPath()) continue;
                ++total;
                if (entry.isPlayable()) ++usable;
            }
            if (total == 0)
                MInspectorField::statusText(EInspectorStatus::Warning, "no clips in this asset");
            else if (usable == 0)
                MInspectorField::statusText(EInspectorStatus::Warning, "no clips usable",
                                            "Every clip from this source is unavailable or overridden - see Available Clips.");
        }
        ImGui::PopID();
    }

    if (removeIndex >= 0)
        entity->removeAnimationSource(removeIndex);
    else if (moveFrom >= 0)
        entity->moveAnimationSource(moveFrom, moveTo);

    if (ImGui::Button("+ Add Source", ImVec2(-FLT_MIN, 0)))
        entity->addAnimationSource(TAssetRef<MAsset>());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Animation clip assets, or skeletal meshes whose clips should be shared.\n"
                          "Clips bind by exact bone name. Later sources win name conflicts.");
}

// -- Available clips ---------------------------------------------------------------

void MSkeletalMeshInspectorDrawer::drawClipsSection(MSkeletalMeshEntity* entity)
{
    const auto& table = entity->getClipTable();
    char header[64];
    std::snprintf(header, sizeof(header), "Available Clips (%d)###available_clips", static_cast<int>(table.size()));
    if (!ImGui::CollapsingHeader(header))
        return;

    if (table.empty())
    {
        ImGui::TextDisabled(entity->resolveMeshAsset() ? "No clips - add an animation source" : "No skeletal mesh");
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV
                                | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable;
    if (!ImGui::BeginTable("##clips", 4, flags))
        return;

    ImGui::TableSetupColumn("Clip",     ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("Source",   ImGuiTableColumnFlags_WidthStretch, 1.2f);
    ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthStretch, 0.6f);
    ImGui::TableSetupColumn("Status",   ImGuiTableColumnFlags_WidthStretch, 1.6f);
    ImGui::TableHeadersRow();

    for (size_t i = 0; i < table.size(); ++i)
    {
        const auto& entry = table[i];
        ImGui::PushID(static_cast<int>(i));
        ImGui::TableNextRow();

        // Unavailable and overridden clips stay in the table, greyed out with
        // the reason - hiding them makes a missing clip look like a failed import.
        const bool greyed = !entry.isPlayable();
        if (greyed) ImGui::BeginDisabled();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(entry.name.c_str());

        ImGui::TableSetColumnIndex(1);
        const SString source = fileName(entry.sourcePath) + (entry.fromMesh ? " (mesh)" : "");
        ImGui::TextUnformatted(source.c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", entry.sourcePath.c_str());

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.2f s", entry.duration);

        if (greyed) ImGui::EndDisabled();

        ImGui::TableSetColumnIndex(3);
        char text[160];
        switch (entry.status)
        {
            case EClipStatus::Available:
                MInspectorField::statusText(EInspectorStatus::Info, "available");
                break;
            case EClipStatus::Warning:
                std::snprintf(text, sizeof(text), "%d/%d bones matched", entry.matchedBones, entry.totalBones);
                MInspectorField::statusText(EInspectorStatus::Warning, text,
                                            "Playable. Normal for upper-body clips, or skeletons with extra socket, IK or face bones.");
                break;
            case EClipStatus::Unavailable:
                MInspectorField::statusText(EInspectorStatus::Error, entry.reason.c_str());
                break;
            case EClipStatus::Overridden:
                std::snprintf(text, sizeof(text), "overridden by %s", fileName(entry.overriddenBy).c_str());
                ImGui::TextDisabled("%s", text);
                break;
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

// -- Preview -------------------------------------------------------------------------

void MSkeletalMeshInspectorDrawer::drawPreviewSection(MSkeletalMeshEntity* entity)
{
    if (!ImGui::CollapsingHeader("Preview"))
        return;

    if (isSimulating())
    {
        const SString clip = entity->getCurrentClip();
        START_FIELD_DRAW("Playing")
            ImGui::TextUnformatted(clip.empty() ? "(bind pose)" : clip.c_str());
        END_FIELD_DRAW()
        START_FIELD_DRAW("Time")
            ImGui::ProgressBar(entity->getNormalizedTime(), ImVec2(-FLT_MIN, 0));
        END_FIELD_DRAW()
        return;
    }

    // Edit mode: plays a clip on this entity only, without pressing Play.
    // Sockets and bone controls follow, so offsets and limits can be tuned.
    const auto names = entity->getClipNames();
    SString preview  = entity->getPreviewClip();

    START_FIELD_DRAW("Clip", "Preview a clip in edit mode. Not saved.")
        if (ImGui::BeginCombo("##preview_clip", preview.empty() ? "(bind pose)" : preview.c_str()))
        {
            if (ImGui::Selectable("(bind pose)", preview.empty()))
            {
                entity->setPreviewClip(SString());
                entity->setPreviewPlaying(false);
            }
            for (const auto& name : names)
                if (ImGui::Selectable(name.c_str(), name == preview))
                    entity->setPreviewClip(name);
            ImGui::EndCombo();
        }
    END_FIELD_DRAW()

    if (preview.empty())
        return;

    START_FIELD_DRAW("Playback")
        const bool playing = entity->isPreviewPlaying();
        if (ImGui::Button(playing ? "Pause" : "Play", ImVec2(80, 0)))
            entity->setPreviewPlaying(!playing);
        ImGui::SameLine();
        if (ImGui::Button("Stop", ImVec2(80, 0)))
        {
            entity->setPreviewPlaying(false);
            entity->setPreviewNormalizedTime(0.0f);
        }
    END_FIELD_DRAW()

    START_FIELD_DRAW("Time")
        float t = entity->getNormalizedTime();
        if (MInspectorField::SliderFloat("##preview_time", &t, 0.0f, 1.0f, "%.2f"))
            entity->setPreviewNormalizedTime(t);
    END_FIELD_DRAW()
}
