//
// Created by ssj5v on 24-04-2025.
//

#include "directionalight_inspector_drawer.h"
#include "core/engine/lighting/directional/directional_light.h"

const bool MDirecionalLightInspectorDrawer::registered = []()
{
    registerDrawer(new MDirecionalLightInspectorDrawer());
    return true;
}();

void MDirecionalLightInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    //Draw TRF
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    //Draw Camera Controls
    drawDLGui(dynamic_cast<MDirectionalLight*>(target));
}

bool MDirecionalLightInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MDirectionalLight*>(entity) != nullptr;
}

inline void MDirecionalLightInspectorDrawer::drawDLGui(MDirectionalLight* light)
{
    if (light == nullptr)
        return;

    if (ImGui::CollapsingHeader("Directional Light",ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Intensity");
        auto intensity = light->getIntensity();

        ImGui::SameLine();
        // Make input float expand to fill remaining width
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Intensity", &intensity, 0.01f);
        ImGui::PopItemWidth();
        light->setIntensity(intensity);

        ImGui::Text("Color");
        auto color = light->getColor();
        float cols[4] = {color.r, color.g, color.b, color.a};
        ImGui::PushItemWidth(200.0f);
        ImGui::ColorPicker4("##Picker", cols, ImGuiColorEditFlags_Float);
        light->setColor(SColor(cols[0], cols[1], cols[2], cols[3]));
    }
}