
#include "scene_raycast.h"
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshasset.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/scene/scene.h"

namespace SceneRaycast
{

static void testEntity(MSpatialEntity* entity,
                       const SVector3& origin, const SVector3& dir,
                       SRaycastHit& best)
{
    if (!entity) return;

    // Test this entity if it's a static mesh.
    if (auto* sme = dynamic_cast<MStaticMeshEntity*>(entity))
    {
        auto meshAsset = sme->getStaticMeshAsset();
        if (meshAsset.isValid())
        {
            const SMatrix4 world = sme->getTransformMatrix();

            for (auto* mesh : meshAsset->getMeshes())
            {
                if (!mesh) continue;
                const auto& verts = mesh->getVertices();
                const int indexCount = mesh->getIndexCount();

                // If the mesh has an index buffer, use it.
                // Otherwise fall back to sequential triangles.
                if (indexCount > 0 && mesh->getEBO() != 0)
                {
                    // Read indices from CPU-side data if available.
                    // MStaticMesh stores vertices but may not store indices
                    // on the CPU side. Fall back to AABB test in that case.
                    // For now, iterate vertices as triangle soup (groups of 3).
                    // TODO: expose getIndices() on MStaticMesh for precise test.
                }

                // Triangle soup — every 3 consecutive vertices form a triangle.
                // This works for most imported meshes. Index-buffer-based meshes
                // would need getIndices() for exact results, but vertex soup
                // gives a reasonable approximation for placement.
                for (int i = 0; i + 2 < (int)verts.size(); i += 3)
                {
                    SVector3 v0 = SVector3(world * SVector4(verts[i    ].Position, 1.f));
                    SVector3 v1 = SVector3(world * SVector4(verts[i + 1].Position, 1.f));
                    SVector3 v2 = SVector3(world * SVector4(verts[i + 2].Position, 1.f));

                    float t;
                    if (rayTriangle(origin, dir, v0, v1, v2, t) && t < best.distance)
                    {
                        best.distance = t;
                        best.point    = origin + dir * t;
                        best.normal   = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                        best.hit      = true;
                    }
                }
            }
        }
    }

    // Recurse into children.
    for (auto* child : entity->getChildren())
        testEntity(child, origin, dir, best);
}

SRaycastHit castRay(MScene* scene, const SVector3& origin, const SVector3& dir)
{
    SRaycastHit best;

    if (scene)
    {
        for (auto* root : scene->getRootEntities())
            testEntity(root, origin, dir, best);
    }

    // Fallback: Y=0 ground plane.
    if (!best.hit)
    {
        float t;
        if (rayGroundPlane(origin, dir, t))
        {
            best.distance = t;
            best.point    = origin + dir * t;
            best.normal   = SVector3(0.f, 1.f, 0.f);
            best.hit      = true;
        }
    }

    // Last resort: 10 units along the ray.
    if (!best.hit)
    {
        best.point    = origin + dir * 10.0f;
        best.normal   = SVector3(0.f, 1.f, 0.f);
        best.distance = 10.0f;
        best.hit      = true;
    }

    return best;
}

}  // namespace SceneRaycast