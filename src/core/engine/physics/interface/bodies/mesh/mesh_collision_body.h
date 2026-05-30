//
// mesh_collision_body.h
//

#ifndef MESH_COLLISION_BODY_H
#define MESH_COLLISION_BODY_H

#include <vector>
#include "core/engine/physics/interface/bodies/collision_body_interface.h"
#include "core/utils/glmhelper.h"

// IMeshCollisionBody wraps Jolt's MeshShape — a concave triangle mesh intended
// for world geometry (terrain, level walls, stairs).
//
// IMPORTANT: MeshShape does not support Dynamic motion. Only Static and
// Kinematic body types are valid.
//
// Shape complexity note: MeshShape builds a BVH at creation time. For complex
// meshes this is a one-time cost during level load, not per-frame.
class IMeshCollisionBody : public virtual ICollisionBody
{
public:
    IMeshCollisionBody()  = default;
    ~IMeshCollisionBody() override = default;

    // Rebuild and swap the underlying MeshShape from new vertex/index data.
    // Vertices must be in the body's local space. Indices form triangles
    // (every 3 consecutive indices = 1 triangle). Returns false if the data
    // is invalid or Jolt rejects the shape.
    virtual bool setMeshData(const std::vector<SVector3>& vertices,
                             const std::vector<uint32_t>& indices) = 0;
};

#endif // MESH_COLLISION_BODY_H