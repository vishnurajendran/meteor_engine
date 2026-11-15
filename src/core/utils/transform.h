//
// transform.h
//
// STransform - a position / rotation / scale triple. Used for bone poses,
// where blending works on the parts rather than on matrices.
//
#pragma once
#ifndef METEOR_TRANSFORM_H
#define METEOR_TRANSFORM_H

#include "glmhelper.h"
#include <glm/gtc/quaternion.hpp>
#include "core/engine/scripting/script_binding_macros.h"

SCRIPT_BIND_STRUCT()
struct STransform
{
    SCRIPT_BIND_PROP()
    SVector3 position = SVector3(0.0f);
    SCRIPT_BIND_PROP()
    SQuaternion rotation = SQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
    SCRIPT_BIND_PROP()
    SVector3 scale = SVector3(1.0f);

    // T * R * S
    [[nodiscard]] SMatrix4 toMatrix() const
    {
        SMatrix4 m = glm::mat4_cast(rotation);
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        m[3] = SVector4(position, 1.0f);
        return m;
    }

    // Assumes no shear (true for anything built from T * R * S).
    static STransform fromMatrix(const SMatrix4& m)
    {
        STransform t;
        t.position = SVector3(m[3]);
        SVector3 c0(m[0]), c1(m[1]), c2(m[2]);
        t.scale = SVector3(glm::length(c0), glm::length(c1), glm::length(c2));
        if (t.scale.x > 0.0f) c0 /= t.scale.x;
        if (t.scale.y > 0.0f) c1 /= t.scale.y;
        if (t.scale.z > 0.0f) c2 /= t.scale.z;
        // A mirrored basis: fold the flip into one axis so the rotation is proper.
        if (glm::dot(glm::cross(c0, c1), c2) < 0.0f)
        {
            t.scale.x = -t.scale.x;
            c0 = -c0;
        }
        t.rotation = glm::normalize(glm::quat_cast(SMatrix3(c0, c1, c2)));
        return t;
    }
};

#endif // METEOR_TRANSFORM_H
