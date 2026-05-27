//
// mesh_collision_body_inspectordrawer.cpp
//

#include "mesh_collision_body_inspectordrawer.h"
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
    auto* body = dynamic_cast<MMeshCollisionBody*>(target);

    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    // ---- Physics Body -------------------------------------------------------

    if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##mesh_phys", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // Dynamic is not supported by MeshShape — show all three options
            // but disable Dynamic and display a tooltip explaining why.
            static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Body Type:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            const int currentTypeIdx = static_cast<int>(body->bodyType.get());
            if (ImGui::BeginCombo("##bodytype", kBodyTypeNames[currentTypeIdx]))
            {
                for (int i = 0; i < 3; ++i)
                {
                    const bool isDynamic = (i == static_cast<int>(ECollisionBodyType::DynamicBody));
                    ImGui::BeginDisabled(isDynamic);
                    if (ImGui::Selectable(kBodyTypeNames[i], currentTypeIdx == i))
                        body->bodyType.set(static_cast<ECollisionBodyType>(i));
                    ImGui::EndDisabled();
                    if (isDynamic && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Jolt MeshShape does not support Dynamic bodies");
                }
                ImGui::EndCombo();
            }

            // Mass is irrelevant for static/kinematic mesh bodies — always disabled.
            ImGui::BeginDisabled(true);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mass (kg):");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float mass = body->mass.get();
            ImGui::DragFloat("##mass", &mass, 0.1f, 0.001f, FLT_MAX, "%.3f");

            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Is Sensor:");
            ImGui::TableSetColumnIndex(1);
            bool sensor = body->isSensor.get();
            if (ImGui::Checkbox("##sensor", &sensor))
                body->isSensor.set(sensor);

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

            ImGui::EndTable();
        }
    }

    // ---- Mesh Shape ---------------------------------------------------------

    if (ImGui::CollapsingHeader("Mesh Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##mesh_shape", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mesh Asset:");
            ImGui::TableSetColumnIndex(1);
            // TAssetRef::isEmpty() and getPath() — adjust to your TAssetRef API
            // if the method names differ.
            const auto& assetRef = body->meshAsset.get();
            if (assetRef.isEmpty())
                ImGui::TextDisabled("(auto from child)");
            else
                ImGui::TextDisabled("%s", assetRef.getPath().c_str());

            if (body->getPhysicsBody())
            {
                const SVector3 com = body->getPhysicsBody()->getCenterOfMass();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("CoM (world):");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("%.2f  %.2f  %.2f", com.x, com.y, com.z);
            }
            else
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("(pending mesh data...)");
            }

            ImGui::EndTable();
        }
    }
}