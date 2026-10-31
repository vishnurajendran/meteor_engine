//
// sphere_collision_body_inspectordrawer.cpp
//

#include "sphere_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/sphere_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MSphereCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MSphereCollisionBodyInspectorDrawer>());
    return true;
}();

bool MSphereCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MSphereCollisionBody*>(entity) != nullptr;
}

void MSphereCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MCollisionBodyEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MSphereCollisionBody*>(target);

    // ---- Sphere Shape ------------------------------------------------------

    if (!ImGui::CollapsingHeader("Sphere Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    START_FIELD_DRAW("Radius")
        float r = body->radius.get();
        if (MInspectorField::DragFloat("##v", &r, 0.005f, 0.001f, FLT_MAX, "%.3f"))
            body->radius.set(glm::max(r, 0.001f));
    END_FIELD_DRAW()
}