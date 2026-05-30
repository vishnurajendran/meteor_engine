//
// collision_body_entity_inspectordrawer.cpp
//

#include "collision_body_inspectordrawer.h"

#include "imgui.h"
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

    constexpr float LW = 140.0f;
    if (!ImGui::BeginTable("##phys_body_common", 2, ImGuiTableFlags_None))
        return;

    ImGui::TableSetupColumn("l", ImGuiTableColumnFlags_WidthFixed,   LW);
    ImGui::TableSetupColumn("w", ImGuiTableColumnFlags_WidthStretch);

    // ---- Body Type ---------------------------------------------------------
    static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Body Type:");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    int typeIdx = static_cast<int>(body->bodyType.get());
    if (ImGui::Combo("##bodytype", &typeIdx, kBodyTypeNames, 3))
        body->bodyType.set(static_cast<ECollisionBodyType>(typeIdx));

    // ---- Physics Layer -----------------------------------------------------
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Physics Layer:");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    drawPhysicsLayerRow(body);

    // ---- Mass --------------------------------------------------------------
    ImGui::BeginDisabled(body->bodyType.get() == ECollisionBodyType::StaticBody);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Mass (kg):");
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN);
    float mass = body->mass.get();
    if (ImGui::DragFloat("##mass", &mass, 0.1f, 0.001f, FLT_MAX, "%.3f"))
        body->mass.set(glm::max(mass, 0.001f));

    ImGui::EndDisabled();

    // ---- Gravity -----------------------------------------------------------
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

    // ---- Sensor ------------------------------------------------------------
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Is Sensor:");
    ImGui::TableSetColumnIndex(1);
    bool sensor = body->isSensor.get();
    if (ImGui::Checkbox("##sensor", &sensor))
        body->isSensor.set(sensor);

    // ---- Damping -----------------------------------------------------------
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

    // ---- CoM (read-only, shown when body exists) ---------------------------
    if (auto* pb = body->getBasePhysicsBody())
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

void MCollisionBodyEntityInspectorDrawer::drawPhysicsLayerRow(MCollisionBodyEntity* body)
{
    const auto* layers = MEngineStatics::getPhysicsLayers();
    int layerIdx = glm::clamp(body->physicsLayer.get(), 0, 31);

    if (layers)
    {
        // c_str() pointers are valid for the frame — the asset lives for the
        // engine lifetime and the SString storage is stable between frames.
        const char* layerNames[32];
        for (int i = 0; i < 32; ++i)
            layerNames[i] = layers->getLayerName(static_cast<unsigned int>(i)).c_str();

        if (ImGui::Combo("##physicslayer", &layerIdx, layerNames, 32))
            body->physicsLayer.set(layerIdx);
    }
    else
    {
        // Asset not loaded — raw int fallback so the field is still editable.
        if (ImGui::DragInt("##physicslayer", &layerIdx, 0.25f, 0, 31))
            body->physicsLayer.set(glm::clamp(layerIdx, 0, 31));
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Physics layers asset not loaded — showing raw index");
    }
}