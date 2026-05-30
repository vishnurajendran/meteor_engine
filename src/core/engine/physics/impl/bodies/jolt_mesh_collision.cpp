//
// jolt_mesh_collision.cpp
//

#include "jolt_mesh_collision.h"
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include "core/utils/logger.h"

IMPLEMENT_BASE_COLLISION_BODY(MJoltMeshCollision)

MJoltMeshCollision::MJoltMeshCollision(JPH::BodyID         bodyId,
                                       JPH::BodyInterface& bodyInterface,
                                       JPH::PhysicsSystem& physicsSystem)
    : INIT_BASE_COLLISION_BODY(bodyInterface, physicsSystem, bodyId)
{
}

bool MJoltMeshCollision::setMeshData(const std::vector<SVector3>& vertices,
                                     const std::vector<uint32_t>& indices)
{
    if (!isValidBody())
    {
        MERROR("MJoltMeshCollision::setMeshData — called on an invalid body");
        return false;
    }

    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0)
    {
        MERROR("MJoltMeshCollision::setMeshData — invalid mesh data (empty or non-triangulated)");
        return false;
    }

    // Jolt MeshShape takes an unindexed triangle list — convert from indexed.
    // Performance note: this is a one-time BVH build cost, not per-frame.
    JPH::TriangleList triangles;
    triangles.reserve(indices.size() / 3);

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        const SVector3& v0 = vertices[indices[i]];
        const SVector3& v1 = vertices[indices[i + 1]];
        const SVector3& v2 = vertices[indices[i + 2]];
        triangles.emplace_back(
            JPH::Float3(v0.x, v0.y, v0.z),
            JPH::Float3(v1.x, v1.y, v1.z),
            JPH::Float3(v2.x, v2.y, v2.z)
        );
    }

    JPH::MeshShapeSettings meshSettings(std::move(triangles));
    JPH::ShapeSettings::ShapeResult result = meshSettings.Create();

    if (!result.IsValid())
    {
        MERROR("MJoltMeshCollision::setMeshData — failed to create MeshShape");
        return false;
    }

    // DontActivate — static/kinematic bodies should not be woken by a shape swap.
    joltBodyInterface.SetShape(joltBodyID, result.Get(), true, JPH::EActivation::DontActivate);
    return true;
}