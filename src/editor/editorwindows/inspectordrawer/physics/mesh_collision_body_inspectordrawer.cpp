//
// mesh_collision_body_inspectordrawer.cpp
//

#include "mesh_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "core/engine/physics/entities/mesh_collision_body_entity.h"

bool MMeshCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(new MMeshCollisionBodyInspectorDrawer());
    return true;
}();

bool MMeshCollisionBodyInspectorDrawer::canDraw(MSpatialEntity* entity)
{
    return dynamic_cast<MMeshCollisionBody*>(entity) != nullptr;
}

void MMeshCollisionBodyInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    // The base drawer draws Body Type as a simple Combo. For mesh we need to
    // gray out Dynamic, so we draw everything ourselves rather than calling
    // the base and then patching over it.
    //
    // We call the spatial base directly to get the transform section, then
    // draw a custom Physics Body section, then our shape section.
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MMeshCollisionBody*>(target);

    // ---- Physics Body (custom — blocks Dynamic) ----------------------------

    if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.0f;
        if (ImGui::BeginTable("##mesh_phys", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // Body Type — Dynamic selectable but visually disabled with tooltip
            static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Body Type:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            const int curType = static_cast<int>(body->bodyType.get());
            if (ImGui::BeginCombo("##bodytype", kBodyTypeNames[curType]))
            {
                for (int i = 0; i < 3; ++i)
                {
                    const bool isDynamic = (i == static_cast<int>(ECollisionBodyType::DynamicBody));
                    ImGui::BeginDisabled(isDynamic);
                    if (ImGui::Selectable(kBodyTypeNames[i], curType == i))
                        body->bodyType.set(static_cast<ECollisionBodyType>(i));
                    ImGui::EndDisabled();
                    if (isDynamic && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Jolt MeshShape does not support Dynamic bodies");
                }
                ImGui::EndCombo();
            }

            // Physics Layer
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Physics Layer:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            drawPhysicsLayerRow(body);

            // Sensor
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Is Sensor:");
            ImGui::TableSetColumnIndex(1);
            bool sensor = body->isSensor.get();
            if (ImGui::Checkbox("##sensor", &sensor)) body->isSensor.set(sensor);

            // Damping
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Linear Drag:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float ld = body->linearDamping.get();
            if (ImGui::DragFloat("##lindamp", &ld, 0.01f, 0.0f, FLT_MAX, "%.3f"))
                body->linearDamping.set(glm::max(ld, 0.0f));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Angular Drag:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float ad = body->angularDamping.get();
            if (ImGui::DragFloat("##angdamp", &ad, 0.01f, 0.0f, FLT_MAX, "%.3f"))
                body->angularDamping.set(glm::max(ad, 0.0f));

            if (auto* pb = body->getPhysicsBody())
            {
                const SVector3 com = pb->getCenterOfMass();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("CoM (world):");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("%.2f  %.2f  %.2f", com.x, com.y, com.z);
            }

            ImGui::EndTable();
        }
    }

    // ---- Mesh Shape --------------------------------------------------------

    if (!ImGui::CollapsingHeader("Mesh Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##mesh_shape", 2, ImGuiTableFlags_None))
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