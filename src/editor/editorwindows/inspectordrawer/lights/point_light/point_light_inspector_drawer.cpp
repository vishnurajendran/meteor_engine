//
// Created by ssj5v on 01-05-2025.
//

#include "point_light_inspector_drawer.h"
#include "core/engine/lighting/dynamiclights/point_light/point_light.h"

bool MPointLightInspectorDrawer::registered = []()
{
    registerDrawer(new MPointLightInspectorDrawer());
    return true;
}();

void MPointLightInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawPLGui(target);
}
bool MPointLightInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MPointLight*>(entity) != nullptr;
}

void MPointLightInspectorDrawer::drawPLGui(MSpatialEntity* target)
{
    auto light = dynamic_cast<MPointLight*>(target);
    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float intensity = light->getIntensity();
        float range = light->getRange();
        ImGui::Text("Range:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Range:", &range, 0.01f, 0,0, "%.2f");
        if (range <= 0)
            range = 0;
        light->setRange(range);

        ImGui::Text("Intensity:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Intensity:", &intensity, 0.01f, 0,0, "%.2f");
        if (intensity <= 0.0f)
            intensity = 0;
        light->setIntensity(intensity);

        ImGui::Text("Color");
        auto color = light->getColor();
        float cols[4] = {color.r, color.g, color.b, color.a};
        ImGui::PushItemWidth(200.0f);
        ImGui::ColorPicker4("##Picker", cols, ImGuiColorEditFlags_Float);
        light->setColor(SColor(cols[0], cols[1], cols[2], cols[3]));
    }
}
