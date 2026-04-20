//
// Created by ssj5v on 29-04-2025.
//

#ifndef DYNAMIC_LIGHT_DATA_H
#define DYNAMIC_LIGHT_DATA_H
#include "../../../../../utils/glmhelper.h"

struct SDynamicLightDataStruct
{
    // Basic - Needed for Point and Spot-lights
    SVector3 position  = {0,0,0};
    float    range     = 10;

    SVector3 color     = {1,1,1};
    float    intensity = 1.0f;

    // Spot light only data
    SVector3 direction = {0,0,0};
    float    angle     = 45.0f;

    alignas(16) int type        = 0; // 0->none, 1->point, 2->spot

    // Shadow map slot assigned by MLightSystemManager::prepareDynamicLights().
    // -1 = no shadow.  >=0 = index into spotShadowMaps[] or pointShadowMaps[]
    // depending on type.  Fits in the alignas(16) padding of the block above.
    int shadowIndex  = -1;
    int smoothShadow = 0;  // Poisson disk PCF for this light instance
    int _pad1        = 0;
};

#endif //DYNAMIC_LIGHT_DATA_H