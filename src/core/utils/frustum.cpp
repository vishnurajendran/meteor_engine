//
// frustum.cpp
//
#include "frustum.h"
#include <cmath>

void SFrustum::extractFromVP(const SMatrix4& vp)
{
    // Gribb-Hartmann plane extraction.
    // glm is column-major: vp[col][row].

    // left:   row3 + row0
    planes[0] = SVector4(
        vp[0][3] + vp[0][0],
        vp[1][3] + vp[1][0],
        vp[2][3] + vp[2][0],
        vp[3][3] + vp[3][0]);

    // right:  row3 - row0
    planes[1] = SVector4(
        vp[0][3] - vp[0][0],
        vp[1][3] - vp[1][0],
        vp[2][3] - vp[2][0],
        vp[3][3] - vp[3][0]);

    // bottom: row3 + row1
    planes[2] = SVector4(
        vp[0][3] + vp[0][1],
        vp[1][3] + vp[1][1],
        vp[2][3] + vp[2][1],
        vp[3][3] + vp[3][1]);

    // top:    row3 - row1
    planes[3] = SVector4(
        vp[0][3] - vp[0][1],
        vp[1][3] - vp[1][1],
        vp[2][3] - vp[2][1],
        vp[3][3] - vp[3][1]);

    // near:   row3 + row2
    planes[4] = SVector4(
        vp[0][3] + vp[0][2],
        vp[1][3] + vp[1][2],
        vp[2][3] + vp[2][2],
        vp[3][3] + vp[3][2]);

    // far:    row3 - row2
    planes[5] = SVector4(
        vp[0][3] - vp[0][2],
        vp[1][3] - vp[1][2],
        vp[2][3] - vp[2][2],
        vp[3][3] - vp[3][2]);

    // Normalize so signed-distance tests return world-space units.
    for (auto& p : planes)
    {
        float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        if (len > 0.0f)
            p /= len;
    }
}

bool SFrustum::testAABB(const AABB& box) const
{
    for (const auto& p : planes)
    {
        // p-vertex: the AABB corner furthest along the plane normal.
        SVector3 pv;
        pv.x = (p.x >= 0.0f) ? box.max.x : box.min.x;
        pv.y = (p.y >= 0.0f) ? box.max.y : box.min.y;
        pv.z = (p.z >= 0.0f) ? box.max.z : box.min.z;

        if (p.x * pv.x + p.y * pv.y + p.z * pv.z + p.w < 0.0f)
            return false; // entirely outside this plane
    }

    return true;
}