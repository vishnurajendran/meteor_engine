//
// Created by ssj5v on 21-05-2026.
//

#ifndef SPHERE_COLLISION_BODY_H
#define SPHERE_COLLISION_BODY_H
#include "core/engine/physics/interface/bodies/collision_body_interface.h"

class ISphereCollisionBody : public virtual ICollisionBody
{
public:
    ISphereCollisionBody()  = default;
    ~ISphereCollisionBody() override = default;

    virtual float setRadius(const float& radius) = 0;
};

#endif //SPHERE_COLLISION_BODY_H