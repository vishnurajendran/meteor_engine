//
// cylinder_collision_body_inspectordrawer.h
//
#pragma once
#ifndef CYLINDER_COLLISION_BODY_INSPECTORDRAWER_H
#define CYLINDER_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MCylinderCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // CYLINDER_COLLISION_BODY_INSPECTORDRAWER_H