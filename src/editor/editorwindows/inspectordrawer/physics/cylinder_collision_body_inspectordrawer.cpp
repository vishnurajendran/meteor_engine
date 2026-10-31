//
// cylinder_collision_body_inspectordrawer.cpp
//

#include "cylinder_collision_body_inspectordrawer.h"

#include <cstdio>

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/cylinder_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MCylinderCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MCylinderCollisionBodyInspectorDrawer>());
    return true;
}();

bool MCylinderCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MCylinderCollisionBody*>(entity) != nullptr;
}

void MCylinderCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MCollisionBodyEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MCylinderCollisionBody*>(target);

    // ---- Cylinder Shape ----------------------------------------------------

    if (!ImGui::CollapsingHeader("Cylinder Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    char tip[48];
    std::snprintf(tip, sizeof(tip), "Full height: %.3f", body->halfHeight.get() * 2.0f);

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