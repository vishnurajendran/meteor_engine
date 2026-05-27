//
// convex_hull_collision_body_inspectordrawer.cpp
//

#include "convexhull_collision_body_inspectordrawer.h"
#include "core/engine/physics/entities/convexhull_collision_body_entity.h"
#include "core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h"


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
    auto* body = dynamic_cast<MConvexHullCollisionBody*>(target);

    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    // ---- Physics Body -------------------------------------------------------

    if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##cvx_phys", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // ConvexHullShape supports all body types including Dynamic.
            static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Body Type:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int typeIdx = static_cast<int>(body->bodyType.get());
            if (ImGui::Combo("##bodytype", &typeIdx, kBodyTypeNames, 3))
                body->bodyType.set(static_cast<ECollisionBodyType>(typeIdx));

            ImGui::BeginDisabled(body->bodyType.get() == ECollisionBodyType::StaticBody);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mass (kg):");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float mass = body->mass.get();
            if (ImGui::DragFloat("##mass", &mass, 0.1f, 0.001f, FLT_MAX, "%.3f"))
                body->mass.set(glm::max(mass, 0.001f));

            ImGui::EndDisabled();

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Gravity:");
            ImGui::TableSetColumnIndex(1);
            bool gravity = body->affectedByGravity.get();
            if (ImGui::Checkbox("##gravity", &gravity))
                body->affectedByGravity.set(gravity);

            ImGui::BeginDisabled(!body->affectedByGravity.get());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Gravity Scale:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float gs = body->gravityScale.get();
            if (ImGui::DragFloat("##gravscale", &gs, 0.01f, 0.0f, FLT_MAX, "%.3f"))
                body->gravityScale.set(glm::max(gs, 0.0f));

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

    // ---- Convex Hull Shape --------------------------------------------------

    if (ImGui::CollapsingHeader("Convex Hull Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##cvx_shape", 2, ImGuiTableFlags_None))
        {
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