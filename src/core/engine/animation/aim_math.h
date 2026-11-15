//
// aim_math.h
//
// Standalone aiming helpers, shared by MBoneControlEntity's lookAt and
// (later) IK solvers.
//
#pragma once
#ifndef AIM_MATH_H
#define AIM_MATH_H

#include "core/utils/glmhelper.h"
#include <glm/gtc/quaternion.hpp>

namespace MAimMath
{
    // The rotation that turns a frame whose aim axis is `aimAxis` and up axis
    // is `upAxis` (both in that frame's rest orientation) so the aim axis
    // points along `direction` while the up axis stays as close to
    // `worldUp` as it can. All in one space. Identity for degenerate input.
    SQuaternion aimRotation(const SVector3& aimAxis, const SVector3& upAxis,
                            const SVector3& direction, const SVector3& worldUp);

    // Rotates `from` towards `to` by at most maxRadians.
    SQuaternion rotateTowards(const SQuaternion& from, const SQuaternion& to, float maxRadians);

    // Limits `rotation` to within maxRadians of `reference` (0 = no limit).
    SQuaternion clampAngle(const SQuaternion& reference, const SQuaternion& rotation, float maxRadians);

    // Angle between two rotations, radians.
    float angleBetween(const SQuaternion& a, const SQuaternion& b);
}

#endif // AIM_MATH_H
