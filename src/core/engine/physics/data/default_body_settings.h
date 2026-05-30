//
// Created by ssj5v on 21-05-2026.
//

#ifndef DEFAULT_BODY_SETTINGS_H
#define DEFAULT_BODY_SETTINGS_H
#include "../../../utils/glmhelper.h"

enum ECollisionBodyType
{
    StaticBody=0,
    DynamicBody,
    KinematicBody
};

struct SBasePhysicsBodySettings
{
    SVector3    position = SVector3(0, 0, 0);
    SQuaternion rotation = SQuaternion(1, 0, 0, 0);

    ECollisionBodyType bodyType        = ECollisionBodyType::StaticBody;
    float   mass                       = 10.0f;
    bool    affectedByGravity          = false;
    bool    movementConstraints[3]     = { true, true, true }; // true = axis is free
    bool    rotationConstraints[3]     = { true, true, true }; // true = axis is free
    bool    isSensor                   = false;

    // Restitution: 0 = perfectly inelastic (dead stop), 1 = perfectly elastic (full bounce).
    // Jolt combines restitution from both bodies using: max(a.restitution, b.restitution),
    // so setting either the sphere OR the floor to a non-zero value produces bouncing.
    float   restitution                = 0.3f;

    // Friction: 0 = frictionless, 1 = high friction.
    // Jolt combines friction from both bodies using: sqrt(a.friction * b.friction).
    float   friction                   = 0.6f;
};

#endif //DEFAULT_BODY_SETTINGS_H