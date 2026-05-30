//
// collision_body_entity_inspectordrawer.h
//

#pragma once
#ifndef COLLISION_BODY_ENTITY_INSPECTORDRAWER_H
#define COLLISION_BODY_ENTITY_INSPECTORDRAWER_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

// Draws the "Physics Body" collapsing section that is common to every collision
// shape — body type, physics layer, mass, gravity, damping, sensor toggle.
// Shape drawers inherit from this and call onDrawInspector() first, then add
// their own shape-specific collapsing section below.
class MCollisionBodyEntityInspectorDrawer : public MSpatialEntityInspectorDrawer
{
public:
    // canDraw is intentionally not registered here — this drawer is only
    // used as a base class, never registered directly.
    bool canDraw(MSpatialEntity* entity) override { return false; }

    // Draws the transform section (via MSpatialEntityInspectorDrawer) followed
    // by the Physics Body section. Shape drawers call this from their own
    // onDrawInspector before appending the shape section.
    void onDrawInspector(MSpatialEntity* target) override;

protected:
    // Draws a standard physics layer combo (or raw int fallback) into the
    // currently open ImGui table. Extracted so subclasses can call it if they
    // ever need to redraw the layer row independently.
    static void drawPhysicsLayerRow(class MCollisionBodyEntity* body);
};

#endif // COLLISION_BODY_ENTITY_INSPECTORDRAWER_H