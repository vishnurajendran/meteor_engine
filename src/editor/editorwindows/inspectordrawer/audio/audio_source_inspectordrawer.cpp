#include "audio_source_inspectordrawer.h"
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/audio/audio_entity/audio_source_entity.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"

// Self-registration -- runs at static-init time, before any inspector lookup.
bool MAudioSourceInspectorDrawer::registered = []()
{
    registerDrawer(new MAudioSourceInspectorDrawer());
    return true;
}();

MAudioSourceInspectorDrawer::MAudioSourceInspectorDrawer()
{
    clipAssetControl = new MAssetReferenceControl();
    clipAssetControl->canAcceptAssetFuncCallback = [](const TAssetHandle<MAsset>& asset)
    { return dynamic_cast<MAudioClipAsset*>(asset.get()) != nullptr; };
}

bool MAudioSourceInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MAudioSource*>(entity) != nullptr;
}

void MAudioSourceInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* src = dynamic_cast<MAudioSource*>(target);

    // Draw transform and basic properties from the base drawer.
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    // ---- Audio Source section ------------------------------------------------

    if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 120.f;
        if (ImGui::BeginTable("##audio_src", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // -- Clip asset reference --
            const auto curClip     = src->getClip();
            const auto controlClip = clipAssetControl->getAssetReference();

            if (controlClip.getAssetId() != curClip.getAssetId())
                clipAssetControl->setAssetReference(curClip.getHandle());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Clip:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            if (clipAssetControl->drawControl(""))
            {
                TAssetHandle<MAudioClipAsset> newClip = clipAssetControl->getAssetReference();
                src->setClip(newClip);
                lastKnownClip = newClip;
            }

            // -- Volume --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Volume:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float vol = src->getVolume();
            if (ImGui::DragFloat("##vol", &vol, 0.005f, 0.0f, 1.0f, "%.3f"))
                src->setVolume(glm::clamp(vol, 0.0f, 1.0f));

            // -- Pitch --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Pitch:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float p = src->getPitch();
            if (ImGui::DragFloat("##pitch", &p, 0.005f, 0.01f, 4.0f, "%.3f"))
                src->setPitch(glm::max(p, 0.01f));

            // -- Loop --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Loop:");
            ImGui::TableSetColumnIndex(1);
            bool lp = src->getLoop();
            if (ImGui::Checkbox("##loop", &lp))
                src->setLoop(lp);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Auto Start:");
            ImGui::TableSetColumnIndex(1);
            bool as = src->getAutoStart();
            if (ImGui::Checkbox("##autoStart", &as))
                src->setAutoStart(as);

            ImGui::EndTable();
        }
    }

    // ---- Spatialization section ----------------------------------------------
    if (ImGui::CollapsingHeader("Spatialization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 120.f;
        if (ImGui::BeginTable("##audio_spatial", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // -- Use Spatial --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Spatial:");
            ImGui::TableSetColumnIndex(1);
            bool spatial = src->getUseSpatial();
            if (ImGui::Checkbox("##spatial", &spatial))
                src->setUseSpatial(spatial);

            // The remaining spatial properties only matter when
            // spatialization is on -- grey them out otherwise.
            ImGui::BeginDisabled(!src->getUseSpatial());

            // -- Roll Off --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Roll Off:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float ro = src->getRollOff();
            if (ImGui::DragFloat("##rolloff", &ro, 0.01f, 0.0f, FLT_MAX, "%.2f"))
                src->setRollOff(glm::max(ro, 0.0f));

            // -- Min Distance --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Min Distance:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float mn = src->getMinDist();
            if (ImGui::DragFloat("##mindist", &mn, 0.1f, 0.0f, FLT_MAX, "%.2f"))
                src->setMinDist(glm::max(mn, 0.0f));

            // -- Max Distance --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Max Distance:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float mx = src->getMaxDist();
            if (ImGui::DragFloat("##maxdist", &mx, 0.1f, 0.0f, FLT_MAX, "%.2f"))
                src->setMaxDist(glm::max(mx, 0.0f));

            // -- Doppler Strength --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Doppler:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float ds = src->getDopplerStrength();
            if (ImGui::DragFloat("##doppler", &ds, 0.01f, 0.0f, FLT_MAX, "%.2f"))
                src->setDopplerStrength(glm::max(ds, 0.0f));

            ImGui::EndDisabled();
            ImGui::EndTable();
        }
    }

    if (ImGui::BeginTable("##audio_controls", 2, ImGuiTableFlags_None))
    {
        const bool isPlaying = src->isPlaying();

        ImGui::TableNextRow();

        // Play / Stop button
        ImGui::TableNextColumn();
        if (ImGui::Button(isPlaying ? "Stop" : "Play"))
        {
            const bool nowPlaying = !isPlaying;
            nowPlaying ? src->play() : src->stop();
        }

        // Status indicator
        ImGui::TableNextColumn();
        ImGui::TextDisabled(isPlaying ? "Playing..." : "Stopped");

        ImGui::EndTable();
    }
}