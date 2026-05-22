//
// Created by ssj5v on 21-05-2026.
//

#ifndef SPHERE_BODY_SETTINGS_H
#define SPHERE_BODY_SETTINGS_H
#include "core/engine/physics/interface/default_body_settings.h"

struct SSphereBodySettings : public SBasePhysicsBodySettings
{
    float radius = 1.0f;
};

#endif //SPHERE_BODY_SETTINGS_H
