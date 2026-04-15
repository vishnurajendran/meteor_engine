//
// Created by ssj5v on 01-05-2025.
//

#include "spot_light_inspector_drawer.h"
#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"

bool MSpotLightInspectorDrawer::registered = []() {
    registerDrawer(new MSpotLightInspectorDrawer());
    return true;
}();

void MSpotLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawSLGui(target);
}

bool MSpotLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MSpotLight*>(entity) != nullptr;
}

void MSpotLightInspectorDrawer::drawSLGui(MSpatialEntity* target) {
    // BUG FIX: original code never null-checked this cast result before dereferencing
    auto* light = dynamic_cast<MSpotLight*>(target);
    if (!light)
        return;

    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  range     = light->getRange();
        float  intensity = light->getIntensity();
        float  angle     = light->getSpotAngle();
        SColor color     = light->getColor();
        bool   changed   = false;

        ImGui::Text("Range:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        // BUG FIX: original had 3x PushItemWidth with zero matching PopItemWidth calls,
        // corrupting the ImGui item-width stack for everything drawn afterward.
        if (ImGui::DragFloat("##Range", &range, 0.01f, 0.0f, FLT_MAX, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::Text("Spot Angle:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##SpotAngle", &angle, 0.1f, 0.0f, 180.0f, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        if (drawLightIntensityAndColor(intensity, color))
            changed = true;

        if (changed) {
            light->setRange(glm::max(range, 0.0f));
            light->setIntensity(glm::max(intensity, 0.0f));
            light->setSpotAngle(glm::clamp(angle, 0.0f, 180.0f));
            light->setColor(color);
        }
    }
}