//
// Created by ssj5v on 21-05-2026.
//

#ifndef PHYSICS_BODY_SETTINGS_H
#define PHYSICS_BODY_SETTINGS_H
#include "core/engine/physics/interface/default_body_settings.h"
#include "core/utils/aabb.h"

struct SBoxPhysicsBodySettings : public SBasePhysicsBodySettings
{
    AABB bounds {
        .min = {-0.5f, -0.5f, -0.5f},
        .max = {0.5f,0.5f,0.5f}
    };
};

#endif //PHYSICS_BODY_SETTINGS_H
