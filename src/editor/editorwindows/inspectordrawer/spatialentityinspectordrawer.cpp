//
// Created by Vishnu Rajendran on 2024-09-28.
//

#include "spatialentityinspectordrawer.h"
#include "core/engine/entities/spatial/spatial.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

void MSpatialEntityInspectorDrawer::onDrawInspector(MSpatialEntity *target) {
    if(target == nullptr)
        return;

    ImGui::BeginChild("NameBox", ImVec2(0, 120), true, ImGuiChildFlags_Border);
    auto size = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
    ImGui::Text("Entity Name");
    std::string name = target->getName();
    ImGui::SetNextItemWidth(size.x - 40);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
    if(ImGui::InputText("##EntityNameField", &name,ImGuiInputTextFlags_EnterReturnsTrue)) {
        target->setName(name);
    }
    ImGui::EndChild();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    ImGui::BeginChild("TransformBox", ImVec2(0, 200), true, ImGuiChildFlags_Border);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
    ImGui::Text("Transform");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
    auto pos = target->getRelativePosition();
    auto rot = quaternionToEuler(target->getRelativeRotation());
    auto scale = target->getRelativeScale();

    if(drawXYZComponent("Position: ", pos)){
        target->setRelativePosition(pos);
    }
    if(drawXYZComponent("Rotation:", rot)){
        auto newRot = SQuaternion (SVector3(glm::radians(rot.x),glm::radians(rot.y), glm::radians(rot.z)));
        target->setRelativeRotation(eulerToQuaternion(rot));
    }
    if(drawXYZComponent("Scale:      ", scale)){
        target->setRelativeScale(scale);
    }
    ImGui::EndChild();
}

void MSpatialEntityInspectorDrawer::drawColoredBoxOverLabel(const char *label, ImVec4 boxColor, float boxWidth) {
// Get current window's draw list
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImU32 boxColor32 = ImGui::ColorConvertFloat4ToU32(boxColor);
    ImVec2 textSize = ImGui::CalcTextSize(label);
    drawList->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + textSize.y + 5), boxColor32);
    ImGui::TextUnformatted(label);
}

bool MSpatialEntityInspectorDrawer::drawXYZComponent(const SString& label, SVector3& value) {
    auto size = ImGui::GetContentRegionAvail();
    auto paddingX = 40;
    auto boxWidth = 50;
    auto staticSizeX = (20 * 7) +  (boxWidth * 3);
    auto fieldSize = (size.x - staticSizeX - (paddingX*2) - 10)/3;

    ImGui::BeginGroup();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX/2);
    ImGui::Text(label.c_str());
    ImGui::SameLine();
    drawColoredBoxOverLabel(" X:", ImVec4(1,0,0,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30);
    ImGui::SetNextItemWidth(fieldSize);
    bool res1 = ImGui::DragFloat(STR("##"+label.str()+"X").c_str(), &value.x);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    drawColoredBoxOverLabel(" Y:", ImVec4(0,1,0,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30);
    ImGui::SetNextItemWidth(fieldSize);
    bool res2 = ImGui::DragFloat(STR("##"+label.str()+"Y").c_str(), &value.y);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
    drawColoredBoxOverLabel(" Z:", ImVec4(0,0,1,0.5f), boxWidth);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30);
    ImGui::SetNextItemWidth(fieldSize);
    bool res3 = ImGui::DragFloat(STR("##"+label.str()+"Z").c_str(), &value.z);
    ImGui::EndGroup();

    return res1 || res2 || res3;
}
