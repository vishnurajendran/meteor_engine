//
// sphere_collision_body_inspectordrawer.h
//
#pragma once
#ifndef SPHERE_COLLISION_BODY_INSPECTORDRAWER_H
#define SPHERE_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MSphereCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // SPHERE_COLLISION_BODY_INSPECTORDRAWER_H