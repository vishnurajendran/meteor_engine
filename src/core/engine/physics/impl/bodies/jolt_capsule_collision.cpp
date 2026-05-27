//
// jolt_capsule_collision.cpp
//

#include "jolt_capsule_collision.h"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include "core/utils/logger.h"

IMPLEMENT_BASE_COLLISION_BODY(MJoltCapsuleCollision)

static JPH::Quat capsuleAxisRotation(EShapeAxis axis)
{
    switch (axis)
    {
    case EShapeAxis::X: return JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -0.5f * JPH::JPH_PI);
    case EShapeAxis::Z: return JPH::Quat::sRotation(JPH::Vec3::sAxisX(),  0.5f * JPH::JPH_PI);
    default:            return JPH::Quat::sIdentity();
    }
}

MJoltCapsuleCollision::MJoltCapsuleCollision(JPH::BodyID         bodyId,
                                             JPH::BodyInterface& bodyInterface,
                                             JPH::PhysicsSystem& physicsSystem,
                                             float               halfHeight,
                                             float               radius,
                                             EShapeAxis          axis)
    : INIT_BASE_COLLISION_BODY(bodyInterface, physicsSystem, bodyId)
    , currentHalfHeight(halfHeight)
    , currentRadius(radius)
    , currentAxis(axis)
{
}

void MJoltCapsuleCollision::setHalfHeight(float halfHeight)
{
    currentHalfHeight = halfHeight;
    rebuildShape();
}

void MJoltCapsuleCollision::setRadius(float radius)
{
    currentRadius = radius;
    rebuildShape();
}

void MJoltCapsuleCollision::setAxis(EShapeAxis axis)
{
    currentAxis = axis;
    rebuildShape();
}

void MJoltCapsuleCollision::rebuildShape()
{
    if (!isValidBody())
    {
        MERROR("MJoltCapsuleCollision::rebuildShape — called on an invalid body");
        return;
    }

    // Jolt's CapsuleShape takes halfHeightOfCylinder and radius.
    // Total capsule extent along the axis = 2 * (halfHeight + radius).
    JPH::CapsuleShapeSettings capsuleSettings(currentHalfHeight, currentRadius);
    JPH::ShapeSettings::ShapeResult capsuleResult = capsuleSettings.Create();
    if (!capsuleResult.IsValid())
    {
        MERROR("MJoltCapsuleCollision::rebuildShape — failed to create CapsuleShape");
        return;
    }

    JPH::ShapeSettings::ShapeResult finalResult;

    if (currentAxis == EShapeAxis::Y)
    {
        finalResult = capsuleResult;
    }
    else
    {
        JPH::RotatedTranslatedShapeSettings rts(
            JPH::Vec3::sZero(),
            capsuleAxisRotation(currentAxis),
            capsuleResult.Get()
        );
        finalResult = rts.Create();
    }

    if (!finalResult.IsValid())
    {
        MERROR("MJoltCapsuleCollision::rebuildShape — failed to create rotated CapsuleShape");
        return;
    }

    joltBodyInterface.SetShape(joltBodyID, finalResult.Get(), true, JPH::EActivation::DontActivate);
}