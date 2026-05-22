//
// Created by ssj5v on 22-05-2026.
//

#include "jolt_box_collision.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include "core/utils/logger.h"

MJoltBoxCollision::MJoltBoxCollision(JPH::BodyID         bodyId,
                                     JPH::BodyInterface& bodyInterface,
                                     JPH::PhysicsSystem& physicsSystem)
    : MJoltBaseCollisionBody(bodyInterface, physicsSystem, bodyId)
{
}

void MJoltBoxCollision::setBounds(const AABB& aabb)
{
    if (!isValidBody())
    {
        MERROR("MJoltBoxCollision::setBounds — called on an invalid body");
        return;
    }

    // Jolt expects half-extents, not full dimensions.
    const float halfX = (aabb.max.x - aabb.min.x) * 0.5f;
    const float halfY = (aabb.max.y - aabb.min.y) * 0.5f;
    const float halfZ = (aabb.max.z - aabb.min.z) * 0.5f;

    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfX, halfY, halfZ));
    JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();

    if (!result.IsValid())
    {
        MERROR("MJoltBoxCollision::setBounds — failed to create BoxShape");
        return;
    }

    // EActivation::DontActivate — leaves sleeping bodies asleep after the shape swap.
    // Pass EActivation::Activate if a bounds change should always wake the body.
    joltBodyInterface.SetShape(joltBodyID, result.Get(), /*inUpdateMassProperties=*/true,
                               JPH::EActivation::DontActivate);
}