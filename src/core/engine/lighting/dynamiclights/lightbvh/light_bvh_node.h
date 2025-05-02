//
// Created by ssj5v on 29-04-2025.
//

#ifndef LIGHT_BVH_NODE_H
#define LIGHT_BVH_NODE_H
#include "../../../../utils/aabb.h"

struct SLightBVHNode
{
    AABB bounds;
    SLightBVHNode* left;
    SLightBVHNode* right;
    int lightIndex=-1;
};

#endif //LIGHT_BVH_NODE_H
