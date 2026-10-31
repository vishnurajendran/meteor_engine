//
// collision_body_entity_inspectordrawer.h
//

#pragma once
#ifndef COLLISION_BODY_ENTITY_INSPECTORDRAWER_H
#define COLLISION_BODY_ENTITY_INSPECTORDRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

// Draws the "Physics Body" collapsing section that is common to every collision
// shape - body type, physics layer, mass, gravity, damping, sensor toggle.
// Shape drawers inherit from this and call onDrawInspector() first, then add
// their own shape-specific collapsing section below.
class MCollisionBodyEntityInspectorDrawer : public MSpatialEntityInspectorDrawer
{
public:
    // canDraw is intentionally not registered here - this drawer is only
    // used as a base class, never registered directly.
    bool canDraw(MSpatialEntity* entity) override { return false; }

    // Draws the transform section (via MSpatialEntityInspectorDrawer) followed
    // by the Physics Body section. Shape drawers call this from their own
    // onDrawInspector before appending the shape section.
    void onDrawInspector(MSpatialEntity* target) override;

protected:
    // Shared field rows, so shape drawers that build their own Physics Body
    // section (e.g. mesh) draw them identically.

    // "Physics Layer" combo (or raw int fallback when the layers asset is missing).
    static void drawPhysicsLayerRow(class MCollisionBodyEntity* body);

    // "Collision Events" / "Trigger Events" checkboxes.
    static void drawContactEventRows(class MCollisionBodyEntity* body);

    // "Linear Drag" / "Angular Drag".
    static void drawDampingRows(class MCollisionBodyEntity* body);

    // Read-only "CoM (world)" row.
    static void drawCenterOfMassRow(const SVector3& com);
};

#endif // COLLISION_BODY_ENTITY_INSPECTORDRAWER_H