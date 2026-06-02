//
// jolt_cylinder_collision.cpp
//

#include "jolt_cylinder_collision.h"

#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include "core/utils/logger.h"

IMPLEMENT_BASE_COLLISION_BODY(MJoltCylinderCollision)

// Returns the quaternion that rotates Jolt's native Y-axis cylinder to align
// with the requested axis. X uses a -90 degree rotation around Z (so Y maps to
// +X). Z uses a +90 degree rotation around X (so Y maps to +Z).
static JPH::Quat cylinderAxisRotation(EShapeAxis axis)
{
    switch (axis)
    {
    case EShapeAxis::X: return JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), -0.5f * JPH::JPH_PI);
    case EShapeAxis::Z: return JPH::Quat::sRotation(JPH::Vec3::sAxisX(),  0.5f * JPH::JPH_PI);
    default:            return JPH::Quat::sIdentity();
    }
}

MJoltCylinderCollision::MJoltCylinderCollision(JPH::BodyID         bodyId,
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

void MJoltCylinderCollision::setHalfHeight(float halfHeight)
{
    currentHalfHeight = halfHeight;
    rebuildShape();
}

void MJoltCylinderCollision::setRadius(float radius)
{
    currentRadius = radius;
    rebuildShape();
}

void MJoltCylinderCollision::setAxis(EShapeAxis axis)
{
    currentAxis = axis;
    rebuildShape();
}

void MJoltCylinderCollision::rebuildShape()
{
    if (!isValidBody())
    {
        MERROR("MJoltCylinderCollision::rebuildShape - called on an invalid body");
        return;
    }

    JPH::CylinderShapeSettings cylinderSettings(currentHalfHeight, currentRadius);
    JPH::ShapeSettings::ShapeResult cylinderResult = cylinderSettings.Create();
    if (!cylinderResult.IsValid())
    {
        MERROR("MJoltCylinderCollision::rebuildShape — failed to create CylinderShape");
        return;
    }

    JPH::ShapeSettings::ShapeResult finalResult;

    if (currentAxis == EShapeAxis::Y)
    {
        finalResult = cylinderResult;
    }
    else
    {
        // Wrap in a RotatedTranslatedShape to reorient without any runtime cost.
        JPH::RotatedTranslatedShapeSettings rts(
            JPH::Vec3::sZero(),
            cylinderAxisRotation(currentAxis),
            cylinderResult.Get()
        );
        finalResult = rts.Create();
    }

    if (!finalResult.IsValid())
    {
        MERROR("MJoltCylinderCollision::rebuildShape — failed to create rotated CylinderShape");
        return;
    }

    // EActivation::DontActivate — leaves sleeping bodies asleep after the shape swap.
    joltBodyInterface.SetShape(joltBodyID, finalResult.Get(), true, JPH::EActivation::DontActivate);
}