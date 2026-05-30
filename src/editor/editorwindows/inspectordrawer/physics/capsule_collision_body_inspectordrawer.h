//
// capsule_collision_body_inspectordrawer.h
//
#pragma once
#ifndef CAPSULE_COLLISION_BODY_INSPECTORDRAWER_H
#define CAPSULE_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MCapsuleCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // CAPSULE_COLLISION_BODY_INSPECTORDRAWER_H