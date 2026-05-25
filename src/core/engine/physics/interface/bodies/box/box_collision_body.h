//
// Created by ssj5v on 21-05-2026.
//

#ifndef BOX_COLLISION_BODY_H
#define BOX_COLLISION_BODY_H
#include "core/engine/physics/interface/bodies/collision_body_interface.h"
#include "core/utils/aabb.h"

class IBoxCollisionBody : public ICollisionBody
{
public:
    IBoxCollisionBody()  = default;
    virtual ~IBoxCollisionBody() = default;

    virtual void setBounds(const AABB& aabb) = 0;
};

#endif //BOX_COLLISION_BODY_H