//
// convex_hull_body_settings.h
//

#ifndef CONVEX_HULL_BODY_SETTINGS_H
#define CONVEX_HULL_BODY_SETTINGS_H

#include <vector>
#include "core/engine/physics/data/default_body_settings.h"
#include "core/utils/glmhelper.h"

// Points are in the body's local space (world scale applied by the entity).
// Jolt computes the convex hull from this point cloud — supply a simplified
// cloud rather than full high-poly vertex data to keep hull computation fast.
struct SConvexHullBodySettings
{
    SVector3           position          = {};
    SQuaternion        rotation          = {};
    ECollisionBodyType bodyType          = ECollisionBodyType::DynamicBody;
    float              mass              = 10.0f;
    bool               affectedByGravity = true;

    std::vector<SVector3> points;
};

#endif // CONVEX_HULL_BODY_SETTINGS_H