//
// Created by ssj5v on 22-05-2026.
//

#include "box_collision_body_inspectordrawer.h"

#include "../../../../core/engine/physics/data/default_body_settings.h"
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
    auto* body = dynamic_cast<MBoxCollisionBody*>(target);

    // Draw transform and base spatial properties.
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    // ---- Physics Body -------------------------------------------------------

    if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##box_phys", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // -- Body Type --
            // ECollisionBodyType is { StaticBody=0, DynamicBody=1, KinematicBody=2 }
            // so a direct int cast is safe as long as the enum order stays fixed.
            static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Body Type:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            int typeIdx = static_cast<int>(body->bodyType.get());
            if (ImGui::Combo("##bodytype", &typeIdx, kBodyTypeNames, 3))
                body->bodyType.set(static_cast<ECollisionBodyType>(typeIdx));

            // -- Mass (greyed out for static bodies -- they have no MotionProperties) --
            ImGui::BeginDisabled(body->bodyType.get() == ECollisionBodyType::StaticBody);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mass (kg):");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float mass = body->mass.get();
            if (ImGui::DragFloat("##mass", &mass, 0.1f, 0.001f, FLT_MAX, "%.3f"))
                body->mass.set(glm::max(mass, 0.001f)); // Jolt requires positive mass

            ImGui::EndDisabled();

            // -- Affected By Gravity --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Gravity:");
            ImGui::TableSetColumnIndex(1);
            bool gravity = body->affectedByGravity.get();
            if (ImGui::Checkbox("##gravity", &gravity))
                body->affectedByGravity.set(gravity);

            // -- Gravity Scale (greyed out when gravity is off) --
            ImGui::BeginDisabled(!body->affectedByGravity.get());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Gravity Scale:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float gs = body->gravityScale.get();
            if (ImGui::DragFloat("##gravscale", &gs, 0.01f, 0.0f, FLT_MAX, "%.3f"))
                body->gravityScale.set(glm::max(gs, 0.0f));

            ImGui::EndDisabled();

            // -- Is Sensor --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Is Sensor:");
            ImGui::TableSetColumnIndex(1);
            bool sensor = body->isSensor.get();
            if (ImGui::Checkbox("##sensor", &sensor))
                body->isSensor.set(sensor);

            // -- Linear Damping --
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Linear Drag:");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float ld = body->linearDamping.get();
            if (ImGui::DragFloat("##lindamp", &ld, 0.01f, 0.0f, FLT_MAX, "%.3f"))
                body->linearDamping.set(glm::max(ld, 0.0f));

            // -- Angular Damping --
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

    // ---- Box Shape ----------------------------------------------------------

    if (ImGui::CollapsingHeader("Box Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        constexpr float LW = 140.f;
        if (ImGui::BeginTable("##box_shape", 2, ImGuiTableFlags_None))
        {
            ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
            ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

            // Half-extents — minimum 0.001 because Jolt rejects degenerate shapes.
            // Full box size displayed as a tooltip so it's clear what the value means.
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Half Extent X:");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Full width: %.3f", body->halfExtentX.get() * 2.0f);
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float hx = body->halfExtentX.get();
            if (ImGui::DragFloat("##hx", &hx, 0.005f, 0.001f, FLT_MAX, "%.3f"))
                body->halfExtentX.set(glm::max(hx, 0.001f));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Half Extent Y:");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Full height: %.3f", body->halfExtentY.get() * 2.0f);
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float hy = body->halfExtentY.get();
            if (ImGui::DragFloat("##hy", &hy, 0.005f, 0.001f, FLT_MAX, "%.3f"))
                body->halfExtentY.set(glm::max(hy, 0.001f));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Half Extent Z:");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Full depth: %.3f", body->halfExtentZ.get() * 2.0f);
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
            float hz = body->halfExtentZ.get();
            if (ImGui::DragFloat("##hz", &hz, 0.005f, 0.001f, FLT_MAX, "%.3f"))
                body->halfExtentZ.set(glm::max(hz, 0.001f));

            // Info row — center of mass relative to body origin.
            if (body->getPhysicsBody())
            {
                const SVector3 com = body->getPhysicsBody()->getCenterOfMass();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("CoM (world):");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("%.2f  %.2f  %.2f", com.x, com.y, com.z);
            }

            ImGui::EndTable();
        }
    }
}