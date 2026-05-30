//
// box_collision_body_inspectordrawer.h
//
#pragma once
#ifndef BOX_COLLISION_BODY_INSPECTORDRAWER_H
#define BOX_COLLISION_BODY_INSPECTORDRAWER_H

#include "collision_body_inspectordrawer.h"

class MBoxCollisionBodyInspectorDrawer : public MCollisionBodyEntityInspectorDrawer
{
public:
    bool canDraw(MSpatialEntity* entity) override;
    void onDrawInspector(MSpatialEntity* target) override;
private:
    static bool registered;
};

#endif // BOX_COLLISION_BODY_INSPECTORDRAWER_H