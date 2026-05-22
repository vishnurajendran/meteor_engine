//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_BOX_COLLISION_H
#define JOLT_BOX_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/bodies/jolt_base_collision_body.h"
#include "core/engine/physics/interface/bodies/box/box_collision_body.h"

class MJoltBoxCollision : public MJoltBaseCollisionBody, public IBoxCollisionBody
{
public:
    MJoltBoxCollision(JPH::BodyID         bodyId,
                      JPH::BodyInterface& bodyInterface,
                      JPH::PhysicsSystem& physicsSystem);

    ~MJoltBoxCollision() override = default;

    void setBounds(const AABB& aabb) override;
};

#endif //JOLT_BOX_COLLISION_H