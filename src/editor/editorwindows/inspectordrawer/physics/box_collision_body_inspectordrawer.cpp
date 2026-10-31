//
// box_collision_body_inspectordrawer.cpp
//

#include "box_collision_body_inspectordrawer.h"

#include <cstdio>

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/box_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MBoxCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MBoxCollisionBodyInspectorDrawer>());
    return true;
}();

bool MBoxCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MBoxCollisionBody*>(entity) != nullptr;
}

void MBoxCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    // Draws transform + Physics Body sections.
    MCollisionBodyEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MBoxCollisionBody*>(target);

    // ---- Box Shape ---------------------------------------------------------

    if (!ImGui::CollapsingHeader("Box Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Half extents with tooltip showing the full dimension
    auto dragHalf = [&](const char* label, Field<float>& field)
    {
        char tip[48];
        std::snprintf(tip, sizeof(tip), "Full size: %.3f", field.get() * 2.0f);
        START_FIELD_DRAW(label, tip)
            float v = field.get();
            if (MInspectorField::DragFloat("##v", &v, 0.005f, 0.001f, FLT_MAX, "%.3f"))
                field.set(glm::max(v, 0.001f));
        END_FIELD_DRAW()
    };

    dragHalf("Half Extent X", body->halfExtentX);
    dragHalf("Half Extent Y", body->halfExtentY);
    dragHalf("Half Extent Z", body->halfExtentZ);
}