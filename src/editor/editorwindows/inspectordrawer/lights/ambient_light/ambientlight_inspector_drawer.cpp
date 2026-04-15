//
// Created by ssj5v on 24-04-2025.
//

#include "ambientlight_inspector_drawer.h"
#include "core/engine/lighting/ambient/ambient_light.h"

const bool MAmbientLightInspectorDrawer::registered = []() {
    registerDrawer(new MAmbientLightInspectorDrawer());
    return true;
}();

void MAmbientLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawALGui(dynamic_cast<MAmbientLightEntity*>(target));
}

bool MAmbientLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MAmbientLightEntity*>(entity) != nullptr;
}

void MAmbientLightInspectorDrawer::drawALGui(MAmbientLightEntity* light) {
    if (!light)
        return;

    if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  intensity = light->getIntensity();
        SColor color     = light->getColor();

        if (drawLightIntensityAndColor(intensity, color)) {
            light->setIntensity(intensity);
            light->setColor(color);
        }
    }
}