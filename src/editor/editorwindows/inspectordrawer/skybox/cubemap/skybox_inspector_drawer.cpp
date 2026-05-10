//
// MSkyboxInspectorDrawer
//

#include "skybox_inspector_drawer.h"
#include "core/engine/skybox/cubemapasset.h"
#include "core/engine/skybox/skybox.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "imgui.h"

bool MSkyboxInspectorDrawer::registered = []() {
    registerDrawer(new MSkyboxInspectorDrawer());
    return true;
}();

MSkyboxInspectorDrawer::MSkyboxInspectorDrawer()
{
    cubemapAssetControl = new MAssetReferenceControl();
    cubemapAssetControl->canAcceptAssetFuncCallback = [](MAsset* asset) {
        return dynamic_cast<MCubemapAsset*>(asset) != nullptr;
    };
}

bool MSkyboxInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MSkyboxEntity*>(entity) != nullptr;
}

void MSkyboxInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    auto* skybox = dynamic_cast<MSkyboxEntity*>(target);
    if (!skybox) return;

    // Draw base spatial inspector (name, enabled, transform)
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LABEL_W = 120.f;
        if (ImGui::BeginTable("##skybox_props", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LABEL_W);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // Sync the control with the entity's current cubemap asset.
            // This handles external changes (e.g. scene load, undo).
            auto* currentCubemap = skybox->getCubemapAsset();
            if (cubemapAssetControl->getAssetReference() != currentCubemap)
                cubemapAssetControl->setAssetReference(currentCubemap);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Cubemap:");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-FLT_MIN);

            if (cubemapAssetControl->drawControl(""))
            {
                auto* newCubemap = dynamic_cast<MCubemapAsset*>(
                    cubemapAssetControl->getAssetReference());
                skybox->setCubemapAsset(newCubemap);
            }

            ImGui::EndTable();
        }
    }
}