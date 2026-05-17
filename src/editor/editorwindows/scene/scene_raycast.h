//
// scene_raycast.h
//
// CPU-side raycasting against scene mesh geometry.
// Used by the editor for drag-to-place and click-to-select-on-surface.
//
// Uses Möller–Trumbore for ray-triangle intersection.
// Iterates all MStaticMeshEntity instances, transforms vertices to world
// space, and tests every triangle. Acceptable for single-shot editor
// operations (drop, click); not suitable for per-frame queries.
//

#pragma once
#ifndef SCENE_RAYCAST_H
#define SCENE_RAYCAST_H

#include "core/utils/glmhelper.h"
#include <limits>

struct SRaycastHit
{
    SVector3 point    = SVector3(0.f);
    SVector3 normal   = SVector3(0.f, 1.f, 0.f);
    float    distance = std::numeric_limits<float>::max();
    bool     hit      = false;
};

class MScene;

namespace SceneRaycast
{
    // ── Primitive tests ───────────────────────────────────────────────────

    // Möller–Trumbore ray-triangle intersection.
    // Returns true if the ray (origin + t*dir) hits the triangle v0-v1-v2.
    // On hit, outT is set to the distance along the ray.
    inline bool rayTriangle(const SVector3& origin, const SVector3& dir,
                            const SVector3& v0, const SVector3& v1, const SVector3& v2,
                            float& outT)
    {
        SVector3 e1 = v1 - v0;
        SVector3 e2 = v2 - v0;
        SVector3 h  = glm::cross(dir, e2);
        float    a  = glm::dot(e1, h);
        if (std::abs(a) < 1e-7f) return false;

        float    f = 1.0f / a;
        SVector3 s = origin - v0;
        float    u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return false;

        SVector3 q = glm::cross(s, e1);
        float    v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) return false;

        outT = f * glm::dot(e2, q);
        return outT > 1e-6f;
    }

    // Ray vs Y=0 ground plane. Returns true if the ray hits the plane.
    inline bool rayGroundPlane(const SVector3& origin, const SVector3& dir,
                               float& outT)
    {
        if (std::abs(dir.y) < 1e-7f) return false;
        outT = -origin.y / dir.y;
        return outT > 0.0f;
    }

    // ── Scene-level raycast ───────────────────────────────────────────────

    // Casts a ray against all MStaticMeshEntity triangle meshes in the scene.
    // Falls back to the Y=0 ground plane if no mesh is hit.
    // Falls back to a point 10 units along the ray if nothing at all is hit.
    SRaycastHit castRay(MScene* scene, const SVector3& origin, const SVector3& dir);

}  // namespace SceneRaycast

#endif // SCENE_RAYCAST_H