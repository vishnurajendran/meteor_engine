//
// convexhull_collision_body_inspectordrawer.cpp
//

#include "convexhull_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "core/engine/physics/entities/convexhull_collision_body_entity.h"

bool MConvexHullCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(new MConvexHullCollisionBodyInspectorDrawer());
    return true;
}();

bool MConvexHullCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MConvexHullCollisionBody*>(entity) != nullptr;
}

void MConvexHullCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    MCollisionBodyEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MConvexHullCollisionBody*>(target);

    // ---- Convex Hull Shape -------------------------------------------------

    if (!ImGui::CollapsingHeader("Convex Hull Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##cvx_shape", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Mesh Asset:");
    ImGui::TableSetColumnIndex(1);
    const auto& assetRef = body->meshAsset.get();
    if (assetRef.isEmpty())
        ImGui::TextDisabled("(auto from child)");
    else
        ImGui::TextDisabled("%s", assetRef.getPath().c_str());

    if (!body->getPhysicsBody())
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("(waiting for mesh data...)");
    }

    ImGui::EndTable();
}