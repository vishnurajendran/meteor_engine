//
// jolt_capsule_collision.h
//

#ifndef JOLT_CAPSULE_COLLISION_H
#define JOLT_CAPSULE_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/capsule/capsule_collision_body.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "macros/jolt_base_collision_body_macros.h"

class MJoltCapsuleCollision : public MObject, public ICapsuleCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltCapsuleCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltCapsuleCollision(JPH::BodyID         bodyId,
                          JPH::BodyInterface& bodyInterface,
                          JPH::PhysicsSystem& physicsSystem,
                          float               halfHeight,
                          float               radius,
                          EShapeAxis          axis);

    ~MJoltCapsuleCollision() override = default;

    void setHalfHeight(float halfHeight) override;
    void setRadius(float radius)         override;
    void setAxis(EShapeAxis axis)        override;

private:
    void rebuildShape();

    float      currentHalfHeight = 0.5f;
    float      currentRadius     = 0.5f;
    EShapeAxis currentAxis       = EShapeAxis::Y;
};

#endif // JOLT_CAPSULE_COLLISION_H