//
// Created by ssj5v on 21-05-2026.
//

#ifndef DEFAULT_BODY_SETTINGS_H
#define DEFAULT_BODY_SETTINGS_H
#include "core/utils/glmhelper.h"

enum ECollisionBodyType
{
    StaticBody,
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
};

#endif //DEFAULT_BODY_SETTINGS_H