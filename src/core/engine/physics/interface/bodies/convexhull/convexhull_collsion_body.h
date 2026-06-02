//
// convex_hull_collision_body.h
//

#ifndef CONVEX_HULL_COLLISION_BODY_H
#define CONVEX_HULL_COLLISION_BODY_H

#include <vector>
#include "core/engine/physics/interface/bodies/collision_body_interface.h"
#include "core/utils/glmhelper.h"

// IConvexHullCollisionBody wraps Jolt's ConvexHullShape - a convex approximation
// of arbitrary geometry that supports all body types including Dynamic.
//
// Jolt computes the convex hull from the provided point cloud using Quickhull
// (O(n log n)). For large point sets this is a one-time creation cost.
// Prefer supplying a pre-simplified point cloud rather than a full high-poly mesh
// to keep hull computation and collision queries fast.
class IConvexHullCollisionBody : public virtual ICollisionBody
{
public:
    IConvexHullCollisionBody()  = default;
    ~IConvexHullCollisionBody() override = default;

    // Rebuild and swap the underlying ConvexHullShape from a new point cloud.
    // Points must be in the body's local space. Returns false if Jolt rejects
    // the hull (e.g. fewer than 4 non-coplanar points).
    virtual bool setPoints(const std::vector<SVector3>& points) = 0;
};

#endif // CONVEX_HULL_COLLISION_BODY_H