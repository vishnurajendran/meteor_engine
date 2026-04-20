#include "point_light_inspector_drawer.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"

bool MPointLightInspectorDrawer::registered = []() {
    registerDrawer(new MPointLightInspectorDrawer());
    return true;
}();

void MPointLightInspectorDrawer::onDrawInspector(MSpatialEntity* target) {
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawPLGui(target);
}

bool MPointLightInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MPointLight*>(entity) != nullptr;
}

void MPointLightInspectorDrawer::drawPLGui(MSpatialEntity* target) {
    auto* light = dynamic_cast<MPointLight*>(target);
    if (!light) return;

    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        float  range       = light->getRange();
        float  intensity   = light->getIntensity();
        SColor color       = light->getColor();
        bool   castsShadow = light->getCastsShadow();
        bool   changed     = false;

        ImGui::Text("Range:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##Range", &range, 0.01f, 0.0f, FLT_MAX, "%.2f"))
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
            light->setColor(color);
        }
    }
}