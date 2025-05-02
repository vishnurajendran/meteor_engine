//
// Created by ssj5v on 30-04-2025.
//

#ifndef DYNAMIC_LIGHT_SHADER_CONTAINER_H
#define DYNAMIC_LIGHT_SHADER_CONTAINER_H
#include "dynamiclights/dynamic_light_data.h"

static constexpr int MAX_DYN_LIGHTS = 8;
struct alignas(16) SDynamicLightShaderContainer{
    SDynamicLightDataStruct data[MAX_DYN_LIGHTS];
    int lightCount;
    int _padding[3];
};

#endif //DYNAMIC_LIGHT_SHADER_CONTAINER_H
