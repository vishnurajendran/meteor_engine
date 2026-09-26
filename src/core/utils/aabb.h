//
// Created by ssj5v on 29-04-2025.
//

#ifndef AABB_H
#define AABB_H
#include "glmhelper.h"
#include "tools/lua_binding_generator/stubs/script_binding_macros.h"

SCRIPT_BIND_STRUCT()
struct AABB
{
    SCRIPT_BIND_PROP()
    SVector3 min;
    SCRIPT_BIND_PROP()
    SVector3 max;

    SCRIPT_BIND_FUNC()
    SVector3 getCentre() const;
    SCRIPT_BIND_FUNC()
    bool inside(const SVector3& point) const;
    SCRIPT_BIND_FUNC()
    static bool intersect(const AABB& a, const AABB& b);
};
#endif //AABB_H
