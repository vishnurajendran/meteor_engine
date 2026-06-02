//
// jolt_convex_hull_collision.cpp
//

#include "jolt_convexhull_collision.h"

#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include "core/utils/logger.h"

IMPLEMENT_BASE_COLLISION_BODY(MJoltConvexHullCollision)

MJoltConvexHullCollision::MJoltConvexHullCollision(JPH::BodyID         bodyId,
                                                    JPH::BodyInterface& bodyInterface,
                                                    JPH::PhysicsSystem& physicsSystem)
    : INIT_BASE_COLLISION_BODY(bodyInterface, physicsSystem, bodyId)
{
}

bool MJoltConvexHullCollision::setPoints(const std::vector<SVector3>& points)
{
    if (!isValidBody())
    {
        MERROR("MJoltConvexHullCollision::setPoints - called on an invalid body");
        return false;
    }

    if (points.size() < 4)
    {
        MERROR("MJoltConvexHullCollision::setPoints — need at least 4 non-coplanar points to form a hull");
        return false;
    }

    // Convert to Jolt Vec3 array — Quickhull runs here (O(n log n)).
    // Performance note: supply a simplified point cloud rather than full
    // high-poly vertex data to keep hull computation and queries fast.
    JPH::Array<JPH::Vec3> jphPoints;
    jphPoints.reserve(points.size());
    for (const SVector3& p : points)
        jphPoints.emplace_back(p.x, p.y, p.z);

    JPH::ConvexHullShapeSettings hullSettings(jphPoints);
    JPH::ShapeSettings::ShapeResult result = hullSettings.Create();

    if (!result.IsValid())
    {
        MERROR("MJoltConvexHullCollision::setPoints — failed to create ConvexHullShape");
        return false;
    }

    joltBodyInterface.SetShape(joltBodyID, result.Get(), true, JPH::EActivation::DontActivate);
    return true;
}