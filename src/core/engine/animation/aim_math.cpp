#include "aim_math.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float EPS = 1e-6f;

    // Orthonormal basis (aim, up, side) as rotation columns. False if degenerate.
    bool makeBasis(const SVector3& aim, const SVector3& up, SMatrix3& out)
    {
        const float aimLen = glm::length(aim);
        if (aimLen < EPS) return false;
        const SVector3 a = aim / aimLen;

        SVector3 side = glm::cross(a, up);
        if (glm::length(side) < EPS)
        {
            // up is parallel to aim: pick any perpendicular.
            const SVector3 fallback = std::abs(a.y) < 0.9f ? SVector3(0, 1, 0) : SVector3(1, 0, 0);
            side = glm::cross(a, fallback);
        }
        side = glm::normalize(side);
        const SVector3 u = glm::cross(side, a);
        out = SMatrix3(a, u, side);
        return true;
    }
}

namespace MAimMath
{
    SQuaternion aimRotation(const SVector3& aimAxis, const SVector3& upAxis,
                            const SVector3& direction, const SVector3& worldUp)
    {
        SMatrix3 from, to;
        if (!makeBasis(aimAxis, upAxis, from) || !makeBasis(direction, worldUp, to))
            return glm::identity<SQuaternion>();
        // to = R * from  ->  R = to * from^T (orthonormal)
        return glm::normalize(glm::quat_cast(to * glm::transpose(from)));
    }

    float angleBetween(const SQuaternion& a, const SQuaternion& b)
    {
        const float d = std::min(1.0f, std::abs(glm::dot(glm::normalize(a), glm::normalize(b))));
        return 2.0f * std::acos(d);
    }

    SQuaternion rotateTowards(const SQuaternion& from, const SQuaternion& to, float maxRadians)
    {
        const float angle = angleBetween(from, to);
        if (angle <= maxRadians || angle < EPS)
            return to;
        return glm::normalize(glm::slerp(from, to, maxRadians / angle));
    }

    SQuaternion clampAngle(const SQuaternion& reference, const SQuaternion& rotation, float maxRadians)
    {
        if (maxRadians <= 0.0f)
            return rotation;
        return rotateTowards(reference, rotation, maxRadians);
    }
}
