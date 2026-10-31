//
// Created by ssj5v on 13-10-2024.
//

#include "cameraentityinspectordrawer.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/memory/mnew.h"

const bool MCameraEntityInspectorDrawer::registered = []() {
    registerDrawer(mnew<MCameraEntityInspectorDrawer>());
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
    auto nearClip = cameraTarget->getClipPlanes().x;
    auto farClip = cameraTarget->getClipPlanes().y;

    if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    START_FIELD_DRAW("Fov")
        if (MInspectorField::DragFloat("##v", &fov, 0.1f, 0.1f, 180.0f))
            cameraTarget->setFov(fov);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Orthographic")
        if (ImGui::Checkbox("##v", &isOrtho))
            cameraTarget->setOrthographic(isOrtho);
    END_FIELD_DRAW()

    START_FIELD_DRAW("Near Clip")
        if (ImGui::InputFloat("##v", &nearClip, 0.1f, 100.0f)) {
            if (nearClip <= 0.01f)
                nearClip = 0.01f;
            cameraTarget->setClipPlanes(nearClip, farClip);
        }
    END_FIELD_DRAW()

    START_FIELD_DRAW("Far Clip")
        if (ImGui::InputFloat("##v", &farClip, 0.1f, 100.0f))
            cameraTarget->setClipPlanes(nearClip, farClip);
    END_FIELD_DRAW()
}