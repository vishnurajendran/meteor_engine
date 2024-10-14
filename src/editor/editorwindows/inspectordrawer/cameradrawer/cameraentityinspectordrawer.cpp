//
// Created by ssj5v on 13-10-2024.
//

#include "cameraentityinspectordrawer.h"
#include "core/engine/camera/camera.h"

const bool MCameraEntityInspectorDrawer::registered = []() {
    registerDrawer(new MCameraEntityInspectorDrawer());
    return true;
}();

void MCameraEntityInspectorDrawer::onDrawInspector(MSpatialEntity *target) {
    //Draw TRF
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    //Draw Camera Controls
    drawCameraGUI(dynamic_cast<MCameraEntity *>(target));
}

bool MCameraEntityInspectorDrawer::canDraw(MSpatialEntity* entity) {
    return dynamic_cast<MCameraEntity*>(entity) != nullptr;
}

void MCameraEntityInspectorDrawer::drawCameraGUI(MCameraEntity* cameraTarget) {
    if(!cameraTarget) return;

    auto fov = cameraTarget->getFov();
    auto isOrtho = cameraTarget->getOrthographic();
    auto nearClip = cameraTarget->getClipPlanes().first;
    auto farClip = cameraTarget->getClipPlanes().second;

    auto dpi = DPIHelper::GetDPIScaleFactor();
    ImGui::BeginChild("##Camera", ImVec2(0, 150*dpi), true, ImGuiChildFlags_Border);
    ImGui::Text("Camera");

    ImGui::Text("Fov");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if(ImGui::DragFloat("##Fov", &fov, 0.1f, 0.1f, 180.0f)) {
        cameraTarget->setFov(fov);
    }

    ImGui::Text("Orthographic");
    ImGui::SameLine();
    if(ImGui::Checkbox("##Orthographic", &isOrtho)) {
        cameraTarget->setOrthographic(isOrtho);
    }

    ImGui::Text("Near Clip");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if(ImGui::InputFloat("##NearClip", &nearClip, 0.1f, 100.0f)) {
        if(nearClip <= 0.01f) {
            nearClip = 0.01f;
        }
        cameraTarget->setClipPlanes(nearClip, farClip);
    }

    ImGui::Text("Far Clip");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if(ImGui::InputFloat("##FarClip",&farClip, 0.1f, 100.0f)) {
        cameraTarget->setClipPlanes(nearClip, farClip);
    }

    ImGui::EndChild();
}
