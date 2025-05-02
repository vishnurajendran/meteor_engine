//
// Created by ssj5v on 01-05-2025.
//

#include "spot_light_inspector_drawer.h"

#include "core/engine/lighting/dynamiclights/spot_light/spot_light.h"

bool MSpotLightInspectorDrawer::registered = []()
{
    registerDrawer(new MSpotLightInspectorDrawer());
    return true;
}();

void MSpotLightInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    drawSLGui(target);
}

bool MSpotLightInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MSpotLight*>(entity) != nullptr;
}

void MSpotLightInspectorDrawer::drawSLGui(MSpatialEntity* target)
{
    auto light = dynamic_cast<MSpotLight*>(target);
    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float intensity = light->getIntensity();
        float range = light->getRange();
        float angle = light->getSpotAngle();

        ImGui::Text("Range:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Range:", &range, 0.01f, 0,0, "%.2f");

        ImGui::Text("Intensity:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Intensity:", &intensity, 0.01f, 0,0, "%.2f");

        ImGui::Text("Spot Angle:");
        ImGui::SameLine();
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Spot Angle:", &angle, 0.01f, 0,0, "%.2f");

        ImGui::Text("Color");
        auto color = light->getColor();
        float cols[4] = {color.r, color.g, color.b, color.a};
        ImGui::PushItemWidth(200.0f);
        ImGui::ColorPicker4("##Picker", cols, ImGuiColorEditFlags_Float);

        if (range <= 0)
            range = 0;
        light->setRange(range);

        if (intensity <= 0.0f)
            intensity = 0;
        light->setIntensity(intensity);

        angle = glm::clamp(angle, 0.0f, 180.0f);
        light->setSpotAngle(angle);

        light->setColor(SColor(cols[0], cols[1], cols[2], cols[3]));
    }
}
