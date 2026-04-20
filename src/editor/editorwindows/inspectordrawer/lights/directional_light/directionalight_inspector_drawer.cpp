#include "directionalight_inspector_drawer.h"
#include "core/engine/lighting/directional/directional_light.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"

const bool MDirecionalLightInspectorDrawer::registered = []() {
    registerDrawer(new MDirecionalLightInspectorDrawer());
    return true;
}();

void MDirecionalLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawDLGui(dynamic_cast<MDirectionalLight*>(target));
}

bool MDirecionalLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MDirectionalLight*>(entity) != nullptr;
}

void MDirecionalLightInspectorDrawer::drawDLGui(MDirectionalLight* light) {
    if (!light) return;

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  intensity = light->getIntensity();
        SColor color     = light->getColor();
        if (drawLightIntensityAndColor(intensity, color)) {
            light->setIntensity(intensity);
            light->setColor(color);
        }
        auto* mgr = MLightSystemManager::getInstance();
        ImGui::Checkbox("Cast Shadow",    &mgr->directionalShadowEnabled);
        ImGui::Checkbox("Smooth Shadows", &mgr->smoothShadows);
    }
}