//
// Created by ssj5v on 01-05-2025.
//
#include "aabb.h"

SVector3 AABB::getCentre() const
{
    return (min + max) * 0.5f;
}

bool AABB::inside(const SVector3& point) const
{
    return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) &&
        (point.z >= min.z && point.z <= max.z);
}

bool AABB::intersect(const AABB& a, const AABB& b)
{
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
              (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
              (a.min.z <= b.max.z && a.max.z >= b.min.z);
}
