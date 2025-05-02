//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "spatialentityinspectordrawer.h"
#include "core/engine/entities/spatial/spatial.h"

#include "imgui.h"
#include "imgui-SFML.h"
#include "misc/cpp/imgui_stdlib.h"

MSpatialEntityInspectorDrawer::MSpatialEntityInspectorDrawer() {
    trfTexture.loadFromFile("meteor_assets/icons/transform.png");
    auto dpi = DPIHelper::GetDPIScaleFactor();
    trfSize = {trfTexture.getSize().x*dpi, trfTexture.getSize().y*dpi};
}

void MSpatialEntityInspectorDrawer::onDrawInspector(MSpatialEntity *target) {
    if(target == nullptr)
        return;

    if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool enabled = target->getEnabled();
        if (ImGui::Checkbox("Enabled", &enabled))
            target->setEnabled(enabled);

        auto name = target->getName();
        if(drawTextField("Entity Name", name)) {
            target->setName(name);
        }
    }

    drawTransformField(target);
}

bool MSpatialEntityInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return true;
}

void MSpatialEntityInspectorDrawer::drawColoredBoxOverLabel(const char *label, ImVec4 boxColor, float boxWidth) {
// Get current window's draw list
    auto dpi = DPIHelper::GetDPIScaleFactor();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 boxColor32 = ImGui::ColorConvertFloat4ToU32(boxColor);
    ImVec2 textSize = ImGui::CalcTextSize(label);
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + textSize.y), boxColor32);
    ImGui::TextUnformatted(label);
}

bool MSpatialEntityInspectorDrawer::drawTextField(const SString &label, SString &text) {
    auto dpi = DPIHelper::GetDPIScaleFactor();
    ImGui::PushID(&label);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10*dpi);
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    std::string name = text;
    ImGui::SetNextItemWidth(size.x - 20*dpi);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10*dpi);
    bool res = ImGui::InputText("##EntityNameField", &name,ImGuiInputTextFlags_EnterReturnsTrue);
    if(res)
        text = name;
    ImGui::PopID();
    return res;
}

void MSpatialEntityInspectorDrawer::drawTransformField(MSpatialEntity *target) {
    auto dpi = DPIHelper::GetDPIScaleFactor();
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto pos = target->getRelativePosition();
        auto rotQuat = target->getRelativeRotation();
        auto rotEuler = quaternionToEuler(target->getRelativeRotation());
        auto oldRotEuler = quaternionToEuler(target->getRelativeRotation());
        auto scale = target->getRelativeScale();

        if(drawXYZComponent("Position: ", pos)){
            target->setRelativePosition(pos);
        }
        if(drawXYZComponent("Rotation:", rotEuler)){
            auto deltaEuler = rotEuler - oldRotEuler;
            glm::quat deltaX = glm::angleAxis(glm::radians(deltaEuler.x), glm::vec3(1, 0, 0));
            glm::quat deltaY = glm::angleAxis(glm::radians(deltaEuler.y), glm::vec3(0, 1, 0));
            glm::quat deltaZ = glm::angleAxis(glm::radians(deltaEuler.z), glm::vec3(0, 0, 1));

            // Apply the rotations in the correct order (e.g., yaw-pitch-roll)
            rotQuat *= (deltaX * deltaY * deltaZ);
            target->setRelativeRotation(rotQuat);
        }
        if(drawXYZComponent("Scale:      ", scale)){
            target->setRelativeScale(scale);
        }
    }
}

bool MSpatialEntityInspectorDrawer::drawXYZComponent(const SString& label, SVector3& value) {

    auto dpi = DPIHelper::GetDPIScaleFactor();
    auto size = ImGui::GetContentRegionAvail();
    auto paddingX = 20 * dpi;
    auto boxWidth = 30 * dpi;
    auto spaceBetwenBoxAndField = 10 * dpi;
    auto spaceBetweenFields = 5 * dpi;
    auto adjustment = 18 * dpi;

    auto staticSizeX = (2 * paddingX) + (boxWidth*3) + (3 * spaceBetwenBoxAndField) + (2 * spaceBetweenFields);
    auto fieldSize = (size.x - staticSizeX)/3 - adjustment;
    ImGui::BeginGroup();

    // X comp
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    drawColoredBoxOverLabel(" X:", ImVec4(1,0,0,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBetwenBoxAndField);
    ImGui::SetNextItemWidth(fieldSize);
    bool res1 = ImGui::DragFloat(STR("##"+label.str()+"X").c_str(), &value.x, 0.1f);

    // Y Comp
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBetweenFields);
    drawColoredBoxOverLabel(" Y:", ImVec4(0,1,0,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBetwenBoxAndField);
    ImGui::SetNextItemWidth(fieldSize);
    bool res2 = ImGui::DragFloat(STR("##"+label.str()+"Y").c_str(), &value.y, 0.1f);

    // Z Comp
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBetweenFields);
    drawColoredBoxOverLabel(" Z:", ImVec4(0,0,1,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + spaceBetwenBoxAndField);
    ImGui::SetNextItemWidth(fieldSize);
    bool res3 = ImGui::DragFloat(STR("##"+label.str()+"Z").c_str(), &value.z, 0.1f);

    ImGui::EndGroup();
    return res1 || res2 || res3;
}
