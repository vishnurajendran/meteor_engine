//
// Created by ssj5v on 22-05-2026.
//

#ifndef COLLISION_DATA_H
#define COLLISION_DATA_H
#include "core/utils/glmhelper.h"

class MSpatialEntity;

// Populated for collision start and stay events. For end events only otherEntity
// is valid - Jolt does not provide manifold data in OnContactRemoved.
struct SCollisionData
{
    MSpatialEntity* otherEntity   = nullptr; // null if the other body has no registered entity
    SVector3        contactPoint  = {};       // world-space contact point on this body's surface
    SVector3        contactNormal = {};       // points from the other body toward this body
    float           penetration   = 0.0f;

    // NOTE: contact impulse is not available at callback time in Jolt — it is
    // computed after the constraint solver runs, which is after all callbacks fire.
};

// Populated for sensor (trigger) overlap events. Same lifetime rules as SCollisionData.
struct SOverlapData
{
    MSpatialEntity* otherEntity  = nullptr;
    SVector3        overlapPoint = {};  // world-space point on the sensor body's surface
};

#endif //COLLISION_DATA_H