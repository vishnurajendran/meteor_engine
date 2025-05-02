//
// Created by ssj5v on 29-04-2025.
//

#ifndef AABB_H
#define AABB_H
#include "glmhelper.h"

struct AABB
{
    SVector3 min;
    SVector3 max;

    [[nodiscard]] SVector3 getCentre() const;
    [[nodiscard]] bool inside(const SVector3& point) const;
    [[nodiscard]] static bool intersect(const AABB& a, const AABB& b);
};
#endif //AABB_H
