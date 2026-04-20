#include "spot_light_inspector_drawer.h"
#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"

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
    auto* light = dynamic_cast<MSpotLight*>(target);
    if (!light) return;

    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  range       = light->getRange();
        float  intensity   = light->getIntensity();
        float  angle       = light->getSpotAngle();
        SColor color       = light->getColor();
        bool   castsShadow = light->getCastsShadow();
        bool   changed     = false;

        ImGui::Text("Range:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##Range", &range, 0.01f, 0.0f, FLT_MAX, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        ImGui::Text("Spot Angle:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##SpotAngle", &angle, 0.1f, 0.0f, 180.0f, "%.2f"))
            changed = true;
        ImGui::PopItemWidth();

        if (ImGui::Checkbox("Cast Shadow", &castsShadow))
            light->setCastsShadow(castsShadow);

        bool smoothShadow = light->getSmoothShadow();
        if (ImGui::Checkbox("Smooth Shadows", &smoothShadow))
            light->setSmoothShadow(smoothShadow);

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