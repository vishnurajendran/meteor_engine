//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_SPHERE_COLLISION_H
#define JOLT_SPHERE_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/bodies/jolt_base_collision_body.h"
#include "core/engine/physics/interface/bodies/sphere/sphere_collision_body.h"

// Mirrors the MJoltBoxCollision pattern. The base provides all shared Jolt logic;
// only the sphere-specific setRadius lives here.
class MJoltSphereCollision : public MJoltBaseCollisionBody, public ISphereCollisionBody
{
public:
    MJoltSphereCollision(JPH::BodyID         bodyId,
                         JPH::BodyInterface& bodyInterface,
                         JPH::PhysicsSystem& physicsSystem);

    ~MJoltSphereCollision() override = default;

    float setRadius(const float& radius) override;
};

#endif //JOLT_SPHERE_COLLISION_H