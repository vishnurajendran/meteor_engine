//
// collision_body_entity_inspectordrawer.cpp
//

#include "collision_body_inspectordrawer.h"

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/engine_statics.h"
#include "core/engine/physics/entities/collision_body_entity.h"

void MCollisionBodyEntityInspectorDrawer::onDrawInspector(MSpatialEntity* target)
{
    // Draw the transform section from the spatial base drawer first.
    MSpatialEntityInspectorDrawer::onDrawInspector(target);

    auto* body = dynamic_cast<MCollisionBodyEntity*>(target);
    if (!body) return;

    if (!ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // ---- Body Type ---------------------------------------------------------
    static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };

    START_FIELD_DRAW("Body Type")
        int typeIdx = static_cast<int>(body->bodyType.get());
        if (ImGui::Combo("##v", &typeIdx, kBodyTypeNames, 3))
            body->bodyType.set(static_cast<ECollisionBodyType>(typeIdx));
    END_FIELD_DRAW()

    // ---- Physics Layer -----------------------------------------------------
    drawPhysicsLayerRow(body);

    // ---- Mass --------------------------------------------------------------
    ImGui::BeginDisabled(body->bodyType.get() == ECollisionBodyType::StaticBody);
    START_FIELD_DRAW("Mass (kg)")
        float mass = body->mass.get();
        if (MInspectorField::DragFloat("##v", &mass, 0.1f, 0.001f, FLT_MAX, "%.3f"))
            body->mass.set(glm::max(mass, 0.001f));
    END_FIELD_DRAW()
    ImGui::EndDisabled();

    // ---- Gravity -----------------------------------------------------------
    START_FIELD_DRAW("Gravity")
        bool gravity = body->affectedByGravity.get();
        if (ImGui::Checkbox("##v", &gravity))
            body->affectedByGravity.set(gravity);
    END_FIELD_DRAW()

    ImGui::BeginDisabled(!body->affectedByGravity.get());
    START_FIELD_DRAW("Gravity Scale")
        float gs = body->gravityScale.get();
        if (MInspectorField::DragFloat("##v", &gs, 0.01f, 0.0f, FLT_MAX, "%.3f"))
            body->gravityScale.set(glm::max(gs, 0.0f));
    END_FIELD_DRAW()
    ImGui::EndDisabled();

    // ---- Sensor ------------------------------------------------------------
    START_FIELD_DRAW("Is Sensor")
        bool sensor = body->isSensor.get();
        if (ImGui::Checkbox("##v", &sensor))
            body->isSensor.set(sensor);
    END_FIELD_DRAW()

    // ---- Contact events ----------------------------------------------------
    drawContactEventRows(body);

    // ---- Damping -----------------------------------------------------------
    drawDampingRows(body);

    // ---- CoM (read-only, shown when body exists) ---------------------------
    if (auto* pb = body->getBasePhysicsBody())
        drawCenterOfMassRow(pb->getCenterOfMass());
}

void MCollisionBodyEntityInspectorDrawer::drawContactEventRows(MCollisionBodyEntity* body)
{
    START_FIELD_DRAW("Collision Events")
        bool collisionEvents = body->receiveCollisionEvents.get();
        if (ImGui::Checkbox("##v", &collisionEvents))
            body->receiveCollisionEvents.set(collisionEvents);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Send onCollisionEnter / Stay / Exit to this entity's script\n"
                              "and fire its C++ collision events.\n"
                              "Off by default - each contact costs work every physics step.");
    END_FIELD_DRAW()

    START_FIELD_DRAW("Trigger Events")
        bool triggerEvents = body->receiveTriggerEvents.get();
        if (ImGui::Checkbox("##v", &triggerEvents))
            body->receiveTriggerEvents.set(triggerEvents);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Send onTriggerEnter / Stay / Exit to this entity's script\n"
                              "and fire its C++ trigger events.\n"
                              "Off by default - each overlap costs work every physics step.");
    END_FIELD_DRAW()
}

void MCollisionBodyEntityInspectorDrawer::drawDampingRows(MCollisionBodyEntity* body)
{
    START_FIELD_DRAW("Linear Drag")
        float ld = body->linearDamping.get();
        if (MInspectorField::DragFloat("##v", &ld, 0.01f, 0.0f, FLT_MAX, "%.3f"))
            body->linearDamping.set(glm::max(ld, 0.0f));
    END_FIELD_DRAW()

    START_FIELD_DRAW("Angular Drag")
        float ad = body->angularDamping.get();
        if (MInspectorField::DragFloat("##v", &ad, 0.01f, 0.0f, FLT_MAX, "%.3f"))
            body->angularDamping.set(glm::max(ad, 0.0f));
    END_FIELD_DRAW()
}

void MCollisionBodyEntityInspectorDrawer::drawCenterOfMassRow(const SVector3& com)
{
    ImGui::BeginDisabled();
    START_FIELD_DRAW("CoM (world)")
        ImGui::Text("%.2f  %.2f  %.2f", com.x, com.y, com.z);
    END_FIELD_DRAW()
    ImGui::EndDisabled();
}

void MCollisionBodyEntityInspectorDrawer::drawPhysicsLayerRow(MCollisionBodyEntity* body)
{
    START_FIELD_DRAW("Physics Layer")
        const auto* layers = MEngineStatics::getPhysicsLayers();
        int layerIdx = glm::clamp(body->physicsLayer.get(), 0, 31);

        if (layers)
        {
            // c_str() pointers are valid for the frame - the asset lives for the
            // engine lifetime and the SString storage is stable between frames.
            const char* layerNames[32];
            for (int i = 0; i < 32; ++i)
                layerNames[i] = layers->getLayerName(static_cast<unsigned int>(i)).c_str();

            if (ImGui::Combo("##v", &layerIdx, layerNames, 32))
                body->physicsLayer.set(layerIdx);
        }
        else
        {
            // Asset not loaded - raw int fallback so the field is still editable.
            if (MInspectorField::DragInt("##v", &layerIdx, 0.25f, 0, 31))
                body->physicsLayer.set(glm::clamp(layerIdx, 0, 31));
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Physics layers asset not loaded - showing raw index");
        }
    END_FIELD_DRAW()
}
