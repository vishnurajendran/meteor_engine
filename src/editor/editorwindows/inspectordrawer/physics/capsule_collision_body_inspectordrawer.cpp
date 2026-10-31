//
// capsule_collision_body_inspectordrawer.cpp
//

#include "capsule_collision_body_inspectordrawer.h"

#include <cstdio>

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/capsule_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MCapsuleCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MCapsuleCollisionBodyInspectorDrawer>());
    return true;
}();

bool MCapsuleCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MCapsuleCollisionBody*>(entity) != nullptr;
}

void MCapsuleCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MCollisionBodyEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MCapsuleCollisionBody*>(target);

    // ---- Capsule Shape -----------------------------------------------------

    if (!ImGui::CollapsingHeader("Capsule Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    char tip[64];
    std::snprintf(tip, sizeof(tip), "Cylindrical segment only.\nTotal: %.3f",
                  (body->halfHeight.get() + body->radius.get()) * 2.0f);

    START_FIELD_DRAW("Half Height", tip)
        float hh = body->halfHeight.get();
        if (MInspectorField::DragFloat("##v", &hh, 0.005f, 0.001f, FLT_MAX, "%.3f"))
            body->halfHeight.set(glm::max(hh, 0.001f));
    END_FIELD_DRAW()

    START_FIELD_DRAW("Radius")
        float r = body->radius.get();
        if (MInspectorField::DragFloat("##v", &r, 0.005f, 0.001f, FLT_MAX, "%.3f"))
            body->radius.set(glm::max(r, 0.001f));
    END_FIELD_DRAW()

    static constexpr const char* kAxisNames[] = { "X", "Y", "Z" };
    START_FIELD_DRAW("Axis")
        int axisIdx = static_cast<int>(body->axis.get());
        if (ImGui::Combo("##v", &axisIdx, kAxisNames, 3))
            body->axis.set(static_cast<EShapeAxis>(axisIdx));
    END_FIELD_DRAW()
}