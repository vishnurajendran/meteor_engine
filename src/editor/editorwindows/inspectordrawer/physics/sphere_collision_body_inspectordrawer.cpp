//
// sphere_collision_body_inspectordrawer.cpp
//

#include "sphere_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "core/engine/physics/entities/sphere_collision_body_entity.h"

bool MSphereCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(new MSphereCollisionBodyInspectorDrawer());
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

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##sph_shape", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Radius:");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    float r = body->radius.get();
    if (ImGui::DragFloat("##radius", &r, 0.005f, 0.001f, FLT_MAX, "%.3f"))
        body->radius.set(glm::max(r, 0.001f));

    ImGui::EndTable();
}