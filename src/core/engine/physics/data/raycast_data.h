//
// raycast_data.h
//

#ifndef RAYCAST_DATA_H
#define RAYCAST_DATA_H

#include "core/utils/glmhelper.h"

class MSpatialEntity;

// A ray defined by a world-space origin, a normalised direction, and a maximum
// distance. Jolt receives direction * distance as its displacement vector, so
// direction MUST be normalised - passing a non-unit vector will silently scale
// the perceived hit fraction.
struct SRay
{
    SVector3 origin    = {};
    SVector3 direction = { 0.0f, -1.0f, 0.0f }; // normalised; defaults downward
    float    distance  = 100.0f;
};

// Populated by IPhysicsEngineSubsystem::rayCast on a successful hit.
struct SRayCastHitResult
{
    // World-space position where the ray intersected the surface.
    SVector3 hitPoint  = {};

    // Outward surface normal at the hit point, in world space.
    // Defaults to up if the normal cannot be determined (degenerate geometry).
    SVector3 hitNormal = { 0.0f, 1.0f, 0.0f };

    // World-space distance from the ray origin to the hit point.
    // Equivalent to SRay::distance * Jolt's internal hit fraction.
    float distance = 0.0f;

    // The spatial entity owning the hit physics body. nullptr if the body has
    // no registered callback receiver (e.g. a body created outside the entity
    // system).
    MSpatialEntity* hitEntity = nullptr;
};

#endif // RAYCAST_DATA_H