//
// jolt_cylinder_collision.h
//

#ifndef JOLT_CYLINDER_COLLISION_H
#define JOLT_CYLINDER_COLLISION_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "core/engine/physics/impl/jolt_body_interface.h"
#include "core/engine/physics/interface/bodies/cylinder/cylinder_collision_body.h"
#include "core/object/object.h"
#include "core/object/object_class_macros.h"
#include "macros/jolt_base_collision_body_macros.h"

class MJoltCylinderCollision : public MObject, public ICylinderCollisionBody, public IJoltCollisionBodyInterface
{
    DEFINE_OBJECT_SUBCLASS(MJoltCylinderCollision)
    DECLARE_BASE_COLLISION_BODY()
public:
    MJoltCylinderCollision(JPH::BodyID         bodyId,
                           JPH::BodyInterface& bodyInterface,
                           JPH::PhysicsSystem& physicsSystem,
                           float               halfHeight,
                           float               radius,
                           EShapeAxis          axis);

    ~MJoltCylinderCollision() override = default;

    void setHalfHeight(float halfHeight) override;
    void setRadius(float radius)         override;
    void setAxis(EShapeAxis axis)        override;

private:
    // Rebuilds and swaps the cylinder shape using the stored parameters.
    // Called by all three setters above.
    void rebuildShape();

    float      currentHalfHeight = 0.5f;
    float      currentRadius     = 0.5f;
    EShapeAxis currentAxis       = EShapeAxis::Y;
};

#endif // JOLT_CYLINDER_COLLISION_H