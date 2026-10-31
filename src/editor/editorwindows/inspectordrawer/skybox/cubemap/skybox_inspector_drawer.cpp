//
// MSkyboxInspectorDrawer
//

#include "skybox_inspector_drawer.h"
#include "core/engine/skybox/cubemapasset.h"
#include "core/engine/skybox/skybox.h"
#include "editor/editorwindows/inspectordrawer/controls/asset_reference_controls.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "imgui.h"
#include "core/memory/mnew.h"

bool MSkyboxInspectorDrawer::registered = []() {
    registerDrawer(mnew<MSkyboxInspectorDrawer>());
    return true;
}();

MSkyboxInspectorDrawer::MSkyboxInspectorDrawer()
{
    cubemapAssetControl = mnew<MAssetReferenceControl>();
    cubemapAssetControl->canAcceptAssetFuncCallback = [](TAssetHandle<MAsset> asset) {
        return dynamic_cast<MCubemapAsset*>(asset.get()) != nullptr;
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
        // Sync the control with the entity's current cubemap asset.
        // This handles external changes (e.g. scene load, undo).
        const auto currentCubemap = skybox->getCubemapAsset();
        if (cubemapAssetControl->getAssetReference().get() != currentCubemap.get())
            cubemapAssetControl->setAssetReference(currentCubemap);

        START_FIELD_DRAW("Cubemap")
            if (cubemapAssetControl->drawControl(""))
                skybox->setCubemapAsset(cubemapAssetControl->getAssetReference());
        END_FIELD_DRAW()
    }
}