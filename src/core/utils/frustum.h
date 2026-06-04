//
// frustum.h
//
// Six-plane view frustum extracted from a view-projection matrix.
// Used by the render pipeline to discard items whose AABB is entirely
// outside the camera frustum before any draw calls are issued.
//
// Plane extraction uses the Gribb-Hartmann method (direct row
// addition/subtraction on the VP matrix).  The AABB test checks the
// "p-vertex" (the corner most aligned with the plane normal) against
// each plane -- if the p-vertex is behind any single plane the entire
// box is outside.
//
// The test is conservative: it can return true for a box that sits
// just outside the frustum corners, but it will never return false
// for a box that is actually visible.
//

#pragma once
#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "glmhelper.h"
#include "aabb.h"

struct SFrustum
{
    SVector4 planes[6]; // left, right, bottom, top, near, far

    // Extract planes from a combined view-projection matrix.
    void extractFromVP(const SMatrix4& vp);

    // Returns true if any part of the AABB is inside or intersects the frustum.
    [[nodiscard]] bool testAABB(const AABB& box) const;
};

#endif // FRUSTUM_H