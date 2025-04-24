//
// Created by ssj5v on 24-04-2025.
//

#include "ambientlight_inspector_drawer.h"

#include "core/engine/lighting/ambient/ambient_light.h"
#include "core/utils/color.h"
#include "core/utils/dpihelper.h"

const bool MAmbientLightInspectorDrawer::registered = []()
{
    registerDrawer(new MAmbientLightInspectorDrawer());
    return true;
}();

void MAmbientLightInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    //Draw TRF
    MSpatialEntityInspectorDrawer::onDrawInspector(target);
    //Draw Camera Controls
    drawALGui(dynamic_cast<MAmbientLightEntity*>(target));
}

bool MAmbientLightInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MAmbientLightEntity*>(entity) != nullptr;
}

void MAmbientLightInspectorDrawer::drawALGui(MAmbientLightEntity* light)
{
    if (light == nullptr)
        return;

    auto dpi = DPIHelper::GetDPIScaleFactor();
    ImGui::BeginChild("##Directional Light", ImVec2(0, 375*dpi), true, ImGuiChildFlags_Border);
    ImGui::Text("Directional Light");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
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
    ImGui::EndChild();

}
