//
// mesh_collision_body_inspectordrawer.h
//
#pragma once
#ifndef MESH_COLLISION_BODY_INSPECTORDRAWER_H
#define MESH_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MMeshCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // MESH_COLLISION_BODY_INSPECTORDRAWER_H