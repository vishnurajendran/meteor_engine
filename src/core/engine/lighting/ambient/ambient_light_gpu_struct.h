//
// Created by ssj5v on 24-01-2025.
//

#ifndef AMBIENT_LIGHT_GPU_STRUCT_H
#define AMBIENT_LIGHT_GPU_STRUCT_H

#include "core/utils/glmhelper.h"

struct SAmbientLightData {
    SVector3 color = {0.0f,0.0f,0.0f};
    float intensity = 0.1f;
    int enabled = true;
    float padding[3];
};

#endif //AMBIENT_LIGHT_GPU_STRUCT_H
