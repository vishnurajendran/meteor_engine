//
// capsule_collision_body_inspectordrawer.h
//

#pragma once
#ifndef CAPSULE_COLLISION_BODY_INSPECTOR_H
#define CAPSULE_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MCapsuleCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MCapsuleCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Capsule Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)       override;

protected:
    void onDrawInspector(MSpatialEntity* target)  override;

private:
    MCapsuleCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // CAPSULE_COLLISION_BODY_INSPECTOR_H