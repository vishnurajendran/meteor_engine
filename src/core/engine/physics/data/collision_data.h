//
// Created by ssj5v on 22-05-2026.
//

#ifndef COLLISION_DATA_H
#define COLLISION_DATA_H
#include "core/engine/scripting/script_binding_macros.h"
#include "core/utils/glmhelper.h"

class MSpatialEntity;

// Populated for collision start and stay events. For end events only otherEntity
// is valid - Jolt does not provide manifold data in OnContactRemoved.
// Bound to Lua: scripts receive a copy in onCollisionEnter/Stay/Exit.
SCRIPT_BIND_STRUCT()
struct SCollisionData
{
    SCRIPT_BIND_PROP()
    MSpatialEntity* thisEntity    = nullptr;  // should never be null lol
    SCRIPT_BIND_PROP()
    MSpatialEntity* otherEntity   = nullptr;  // null if the other body has no registered entity
    SCRIPT_BIND_PROP()
    SVector3        contactPoint  = {};       // world-space contact point on this body's surface
    SCRIPT_BIND_PROP()
    SVector3        contactNormal = {};       // points from the other body toward this body
    SCRIPT_BIND_PROP()
    float           penetration   = 0.0f;

    // NOTE: contact impulse is not available at callback time in Jolt — it is
    // computed after the constraint solver runs, which is after all callbacks fire.
};

// Populated for sensor (trigger) overlap events. Same lifetime rules as SCollisionData.
// Bound to Lua: scripts receive a copy in onTriggerEnter/Stay/Exit.
SCRIPT_BIND_STRUCT()
struct SOverlapData
{
    SCRIPT_BIND_PROP()
    MSpatialEntity* thisEntity   = nullptr;
    SCRIPT_BIND_PROP()
    MSpatialEntity* otherEntity  = nullptr;
    SCRIPT_BIND_PROP()
    SVector3        overlapPoint = {};  // world-space point on the sensor body's surface
};

#endif //COLLISION_DATA_H