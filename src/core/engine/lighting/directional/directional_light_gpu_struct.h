//
// Created by ssj5v on 26-01-2025.
//

#ifndef DIRECTIONAL_LIGHT_DATA_H
#define DIRECTIONAL_LIGHT_DATA_H
#include "core/utils/glmhelper.h"

struct SDirectionalLightData
{
    SVector3 lightDirection = {0.0f, 0.0f, 0.0f};
    float lightIntensity = 1.0f;
    SVector3 lightColor = {1.0f, 1.0f, 1.0f};
    int enabled = true;
};

#endif //DIRECTIONAL_LIGHT_DATA_H
