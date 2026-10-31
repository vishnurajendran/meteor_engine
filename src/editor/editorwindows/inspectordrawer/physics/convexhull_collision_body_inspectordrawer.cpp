//
// convexhull_collision_body_inspectordrawer.cpp
//

#include "convexhull_collision_body_inspectordrawer.h"

#include "imgui.h"
#include "editor/editorwindows/inspectordrawer/controls/inspector_field.h"
#include "core/engine/physics/entities/convexhull_collision_body_entity.h"
#include "core/memory/mnew.h"

bool MConvexHullCollisionBodyInspectorDrawer::registered = []()
{
    registerDrawer(mnew<MConvexHullCollisionBodyInspectorDrawer>());
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