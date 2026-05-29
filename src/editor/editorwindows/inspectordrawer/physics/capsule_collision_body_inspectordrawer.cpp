//
// capsule_collision_body_inspectordrawer.cpp
//

#include "capsule_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "core/engine/physics/entities/capsule_collision_body_entity.h"

bool MCapsuleCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(new MCapsuleCollisionBodyInspectorDrawer());
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

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##cap_shape", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Half Height:");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Cylindrical segment only.\nTotal: %.3f",
                          (body->halfHeight.get() + body->radius.get()) * 2.0f);
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    float hh = body->halfHeight.get();
    if (ImGui::DragFloat("##halfheight", &hh, 0.005f, 0.001f, FLT_MAX, "%.3f"))
        body->halfHeight.set(glm::max(hh, 0.001f));

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Radius:");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    float r = body->radius.get();
    if (ImGui::DragFloat("##radius", &r, 0.005f, 0.001f, FLT_MAX, "%.3f"))
        body->radius.set(glm::max(r, 0.001f));

    static constexpr const char* kAxisNames[] = { "X", "Y", "Z" };
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Axis:");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    int axisIdx = static_cast<int>(body->axis.get());
    if (ImGui::Combo("##axis", &axisIdx, kAxisNames, 3))
        body->axis.set(static_cast<EShapeAxis>(axisIdx));

    ImGui::EndTable();
}