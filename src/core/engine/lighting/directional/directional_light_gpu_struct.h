//
// Created by ssj5v on 26-01-2025.
//

#ifndef DIRECTIONAL_LIGHT_DATA_H
#define DIRECTIONAL_LIGHT_DATA_H
#include "core/utils/glmhelper.h"

struct SDirectionalLightData
{
    SVector3 lightDirection;
    SVector3 lightColor;
    float lightIntensity;
};

#endif //DIRECTIONAL_LIGHT_DATA_H
