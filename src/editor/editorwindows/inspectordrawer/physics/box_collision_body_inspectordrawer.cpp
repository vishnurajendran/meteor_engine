//
// box_collision_body_inspectordrawer.cpp
//

#include "box_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "core/engine/physics/entities/box_collision_body_entity.h"

bool MBoxCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(new MBoxCollisionBodyInspectorDrawer());
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

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##box_shape", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

    // Half extents with tooltip showing the full dimension
    auto dragHalf = [&](const char* label, const char* id, Field<float>& field)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Full size: %.3f", field.get() * 2.0f);
        ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
        float v = field.get();
        if (ImGui::DragFloat(id, &v, 0.005f, 0.001f, FLT_MAX, "%.3f"))
            field.set(glm::max(v, 0.001f));
    };

    dragHalf("Half Extent X:", "##hx", body->halfExtentX);
    dragHalf("Half Extent Y:", "##hy", body->halfExtentY);
    dragHalf("Half Extent Z:", "##hz", body->halfExtentZ);

    ImGui::EndTable();
}