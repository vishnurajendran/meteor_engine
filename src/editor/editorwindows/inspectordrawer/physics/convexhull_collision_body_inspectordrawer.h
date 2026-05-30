//
// convexhull_collision_body_inspectordrawer.h
//
#pragma once
#ifndef CONVEXHULL_COLLISION_BODY_INSPECTORDRAWER_H
#define CONVEXHULL_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MConvexHullCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // CONVEXHULL_COLLISION_BODY_INSPECTORDRAWER_H