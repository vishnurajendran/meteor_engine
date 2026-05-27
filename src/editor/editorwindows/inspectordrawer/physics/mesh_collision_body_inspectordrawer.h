//
// mesh_collision_body_inspectordrawer.h
//

#pragma once
#ifndef MESH_COLLISION_BODY_INSPECTOR_H
#define MESH_COLLISION_BODY_INSPECTOR_H

#include "editor/editorwindows/inspectordrawer/spatialentityinspectordrawer.h"

class MMeshCollisionBodyInspectorDrawer : public MSpatialEntityInspectorDrawer
{
    DEFINE_OBJECT_SUBCLASS(MMeshCollisionBodyInspectorDrawer)
public:
    SString getInspectorName() const override { return "Mesh Collision Body"; }
    bool    canDraw(MSpatialEntity* entity)       override;

protected:
    void onDrawInspector(MSpatialEntity* target)  override;

private:
    MMeshCollisionBodyInspectorDrawer() = default;

    static bool registered;
};

#endif // MESH_COLLISION_BODY_INSPECTOR_H