//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_BOX_COLLISION_H
#define JOLT_BOX_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/box/box_collision_body.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "macros/jolt_base_collision_body_macros.h"

class MJoltBoxCollision : public MObject, public IBoxCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltBoxCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltBoxCollision(JPH::BodyID         bodyId,
                      JPH::BodyInterface& bodyInterface,
                      JPH::PhysicsSystem& physicsSystem);

    ~MJoltBoxCollision() override = default;

    void setBounds(const AABB& aabb) override;
};
#endif //JOLT_BOX_COLLISION_H