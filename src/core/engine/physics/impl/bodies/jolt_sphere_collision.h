//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_SPHERE_COLLISION_H
#define JOLT_SPHERE_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/sphere/sphere_collision_body.h"
#include "core/object/object.h"
#include "macros/jolt_base_collision_body_macros.h"

// Mirrors the MJoltBoxCollision pattern. The base provides all shared Jolt logic;
// only the sphere-specific setRadius lives here.
class MJoltSphereCollision : public MObject, public ISphereCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltSphereCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltSphereCollision(JPH::BodyID         bodyId,
                         JPH::BodyInterface& bodyInterface,
                         JPH::PhysicsSystem& physicsSystem);

    ~MJoltSphereCollision() override = default;

    float setRadius(const float& radius) override;
};

#endif //JOLT_SPHERE_COLLISION_H