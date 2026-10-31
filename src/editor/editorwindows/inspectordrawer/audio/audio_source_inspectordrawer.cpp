#include "audio_source_inspectordrawer.h"
#include "core/engine/audio/asset/audioclip_asset.h"
#include "core/engine/audio/audio_entity/audio_source_entity.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/memory/mnew.h"

// Self-registration -- runs at static-init time, before any inspector lookup.
bool MAudioSourceInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MAudioSourceInspectorDrawer>());
    return true;
}();

MAudioSourceInspectorDrawer::MAudioSourceInspectorDrawer()
{
    clipAssetControl = mnew<MAssetReferenceControl>();
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
        // -- Clip asset reference --
        const auto curClip     = src->getClip();
        const auto controlClip = clipAssetControl->getAssetReference();

        if (controlClip.getAssetId() != curClip.getAssetId())
            clipAssetControl->setAssetReference(curClip.getHandle());

        START_FIELD_DRAW("Clip")
            if (clipAssetControl->drawControl(""))
            {
                TAssetHandle<MAudioClipAsset> newClip = clipAssetControl->getAssetReference();
                src->setClip(newClip);
                lastKnownClip = newClip;
            }
        END_FIELD_DRAW()

        START_FIELD_DRAW("Volume")
            float vol = src->getVolume();
            if (MInspectorField::DragFloat("##v", &vol, 0.005f, 0.0f, 1.0f, "%.3f"))
                src->setVolume(glm::clamp(vol, 0.0f, 1.0f));
        END_FIELD_DRAW()

        START_FIELD_DRAW("Pitch")
            float p = src->getPitch();
            if (MInspectorField::DragFloat("##v", &p, 0.005f, 0.01f, 4.0f, "%.3f"))
                src->setPitch(glm::max(p, 0.01f));
        END_FIELD_DRAW()

        START_FIELD_DRAW("Loop")
            bool lp = src->getLoop();
            if (ImGui::Checkbox("##v", &lp))
                src->setLoop(lp);
        END_FIELD_DRAW()

        START_FIELD_DRAW("Auto Start")
            bool as = src->getAutoStart();
            if (ImGui::Checkbox("##v", &as))
                src->setAutoStart(as);
        END_FIELD_DRAW()

        // -- Preview --
        START_FIELD_DRAW("Preview")
            const bool isPlaying = src->isPlaying();
            if (ImGui::Button(isPlaying ? "Stop" : "Play"))
                isPlaying ? src->stop() : src->play();
            ImGui::SameLine();
            ImGui::TextDisabled(isPlaying ? "Playing..." : "Stopped");
        END_FIELD_DRAW()
    }

    // ---- Spatialization section ----------------------------------------------
    if (ImGui::CollapsingHeader("Spatialization", ImGuiTreeNodeFlags_DefaultOpen))
    {
        START_FIELD_DRAW("Spatial")
            bool spatial = src->getUseSpatial();
            if (ImGui::Checkbox("##v", &spatial))
                src->setUseSpatial(spatial);
        END_FIELD_DRAW()

        // The remaining spatial properties only matter when
        // spatialization is on -- grey them out otherwise.
        ImGui::BeginDisabled(!src->getUseSpatial());

        START_FIELD_DRAW("Roll Off")
            float ro = src->getRollOff();
            if (MInspectorField::DragFloat("##v", &ro, 0.01f, 0.0f, FLT_MAX, "%.2f"))
                src->setRollOff(glm::max(ro, 0.0f));
        END_FIELD_DRAW()

        START_FIELD_DRAW("Min Distance")
            float mn = src->getMinDist();
            if (MInspectorField::DragFloat("##v", &mn, 0.1f, 0.0f, FLT_MAX, "%.2f"))
                src->setMinDist(glm::max(mn, 0.0f));
        END_FIELD_DRAW()

        START_FIELD_DRAW("Max Distance")
            float mx = src->getMaxDist();
            if (MInspectorField::DragFloat("##v", &mx, 0.1f, 0.0f, FLT_MAX, "%.2f"))
                src->setMaxDist(glm::max(mx, 0.0f));
        END_FIELD_DRAW()

        START_FIELD_DRAW("Doppler")
            float ds = src->getDopplerStrength();
            if (MInspectorField::DragFloat("##v", &ds, 0.01f, 0.0f, FLT_MAX, "%.2f"))
                src->setDopplerStrength(glm::max(ds, 0.0f));
        END_FIELD_DRAW()

        ImGui::EndDisabled();
    }
}