//
// Created by ssj5v on 30-04-2025.
//

#ifndef DYNAMIC_LIGHT_SHADER_CONTAINER_H
#define DYNAMIC_LIGHT_SHADER_CONTAINER_H
#include "../../graphics/core/render-pipeline/stages/lighting/dynamic_light_data.h"

// Dynamic lights uploaded per frame for the whole view (fullscreen lighting).
// Must match MAX_DYN_LIGHTS in meteor_lighting_utils.glsl.
static constexpr int MAX_DYN_LIGHTS = 32;
struct alignas(16) SDynamicLightShaderContainer{
    SDynamicLightDataStruct data[MAX_DYN_LIGHTS];
    int lightCount;
    int _padding[3];
};

#endif //DYNAMIC_LIGHT_SHADER_CONTAINER_H
