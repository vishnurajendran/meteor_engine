//
// convex_hull_collision_body_inspectordrawer.h
//

#pragma once
#ifndef CONVEX_HULL_COLLISION_BODY_INSPECTOR_H
#define CONVEX_HULL_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MConvexHullCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MConvexHullCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Convex Hull Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)       override;

protected:
    void onDrawInspector(MSpatialEntity* target)  override;

private:
    MConvexHullCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // CONVEX_HULL_COLLISION_BODY_INSPECTOR_H