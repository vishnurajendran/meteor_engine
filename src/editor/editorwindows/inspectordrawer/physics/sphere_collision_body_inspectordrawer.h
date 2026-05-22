//
// Created by ssj5v on 22-05-2026.
//

#pragma once
#ifndef SPHERE_COLLISION_BODY_INSPECTOR_H
#define SPHERE_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MSphereCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MSphereCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Sphere Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)       override;

protected:
    void onDrawInspector(MSpatialEntity* target)  override;

private:
    MSphereCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // SPHERE_COLLISION_BODY_INSPECTOR_H