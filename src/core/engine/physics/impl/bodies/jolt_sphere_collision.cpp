//
// Created by ssj5v on 22-05-2026.
//

#include "jolt_sphere_collision.h"

#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include "core/utils/logger.h"

IMPLEMENT_BASE_COLLISION_BODY(MJoltSphereCollision)

MJoltSphereCollision::MJoltSphereCollision(JPH::BodyID         bodyId,
                                     JPH::BodyInterface& bodyInterface,
                                     JPH::PhysicsSystem& physicsSystem)
    : joltBodyInterface(bodyInterface) , joltPhysicsSystem(physicsSystem), joltBodyID(bodyId)
{

}


float MJoltSphereCollision::setRadius(const float& radius)
{
    if (!isValidBody())
    {
        MERROR("MJoltSphereCollision::setRadius — called on an invalid body");
        return radius;
    }

    // Jolt shapes are immutable after creation - resizing requires swapping to a
    // new shape entirely. This allocates a new shape object on the Jolt heap and
    // may briefly pause any threads waiting to read this body's broadphase entry.
    JPH::SphereShapeSettings settings(radius);
    JPH::ShapeSettings::ShapeResult result = settings.Create();

    if (!result.IsValid())
    {
        MERROR("MJoltSphereCollision::setRadius — failed to create SphereShape");
        return radius;
    }

    // EActivation::DontActivate — leaves sleeping bodies asleep after the swap.
    joltBodyInterface.SetShape(joltBodyID, result.Get(), /*inUpdateMassProperties=*/true,
                               JPH::EActivation::DontActivate);

    return radius;
}