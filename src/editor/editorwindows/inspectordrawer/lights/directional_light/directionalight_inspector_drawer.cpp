//
// Created by ssj5v on 24-04-2025.
//

#include "directionalight_inspector_drawer.h"
#include "core/engine/lighting/directional/directional_light.h"

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
    if (!light)
        return;

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  intensity = light->getIntensity();
        SColor color     = light->getColor();

        if (drawLightIntensityAndColor(intensity, color)) {
            light->setIntensity(intensity);
            light->setColor(color);
        }
    }
}