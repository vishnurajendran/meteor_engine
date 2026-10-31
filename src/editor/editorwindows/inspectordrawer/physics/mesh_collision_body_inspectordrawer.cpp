//
// mesh_collision_body_inspectordrawer.cpp
//

#include "mesh_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/mesh_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MMeshCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MMeshCollisionBodyInspectorDrawer>());
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

    // ---- Physics Body (custom - blocks Dynamic) ----------------------------

    if (ImGui::CollapsingHeader("Physics Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Body Type - Dynamic selectable but visually disabled with tooltip
        static constexpr const char* kBodyTypeNames[] = { "Static", "Dynamic", "Kinematic" };
        START_FIELD_DRAW("Body Type")
            const int curType = static_cast<int>(body->bodyType.get());
            if (ImGui::BeginCombo("##v", kBodyTypeNames[curType]))
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
        END_FIELD_DRAW()

        drawPhysicsLayerRow(body);

        START_FIELD_DRAW("Is Sensor")
            bool sensor = body->isSensor.get();
            if (ImGui::Checkbox("##v", &sensor)) body->isSensor.set(sensor);
        END_FIELD_DRAW()

        drawContactEventRows(body);
        drawDampingRows(body);

        if (auto* pb = body->getPhysicsBody())
            drawCenterOfMassRow(pb->getCenterOfMass());
    }

    // ---- Mesh Shape --------------------------------------------------------

    if (!ImGui::CollapsingHeader("Mesh Shape", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    START_FIELD_DRAW("Mesh Asset")
        const auto& assetRef = body->meshAsset.get();
        if (assetRef.isEmpty())
            ImGui::TextDisabled("(auto from child)");
        else
            ImGui::TextDisabled("%s", assetRef.getPath().c_str());

        if (!body->getPhysicsBody())
            ImGui::TextDisabled("(waiting for mesh data...)");
    END_FIELD_DRAW()
}