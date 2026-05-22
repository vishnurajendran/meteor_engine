//
// Created by ssj5v on 22-05-2026.
//

#pragma once
#ifndef BOX_COLLISION_BODY_INSPECTOR_H
#define BOX_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MBoxCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MBoxCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Box Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)           override;

protected:
    void onDrawInspector(MSpatialEntity* target)      override;

private:
    MBoxCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // BOX_COLLISION_BODY_INSPECTOR_H