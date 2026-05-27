//
// cylinder_collision_body_inspectordrawer.h
//

#pragma once
#ifndef CYLINDER_COLLISION_BODY_INSPECTOR_H
#define CYLINDER_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MCylinderCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MCylinderCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Cylinder Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)       override;

protected:
    void onDrawInspector(MSpatialEntity* target)  override;

private:
    MCylinderCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // CYLINDER_COLLISION_BODY_INSPECTOR_H