//
// mesh_collision_body_entity.cpp
//

#include "mesh_collision_body_entity.h"

#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/gizmos/gizmos.h"

IMPLEMENT_SPATIAL_CLASS(MMeshCollisionBody)

// ---------------------------------------------------------------------------
// Pure virtual implementations
// ---------------------------------------------------------------------------

void MMeshCollisionBody::createCollisionBody()
{
    // Try explicit asset first, then scan for a child MStaticMeshEntity.
    if (!meshAsset.get().isEmpty())
        if (auto* r = meshAsset.get().resolve()) { tryBuildBody(r); return; }
    if (auto* child = findMeshEntityChild())
        if (auto* r = child->meshAsset.get().resolve()) tryBuildBody(r);
    // body may still be null — onFixedUpdate will keep retrying
}

void MMeshCollisionBody::releaseBody()
{
    if (!physicsBody) return;
    physicsEngine->releaseMeshCollisionBody(physicsBody);
    physicsBody = nullptr;
}

void MMeshCollisionBody::setupShapeCallbacks()
{
    // Block Dynamic — MeshShape does not support it.
    bodyType.setOnChangeCallback([this](auto v)
    {
        if (v == ECollisionBodyType::DynamicBody)
        {
            MWARN("MMeshCollisionBody::bodyType — MeshShape does not support Dynamic; reverting to Static");
            bodyType.set(ECollisionBodyType::StaticBody);
            return;
        }
        recreateBody(); // base helper
    });

    meshAsset.setOnChangeCallback([this](auto) { recreateBody(); });
}

// ---------------------------------------------------------------------------
// Deferred retry
// ---------------------------------------------------------------------------

void MMeshCollisionBody::onFixedUpdate(float)
{
    if (!initialized || physicsBody) return;

    if (auto* child = findMeshEntityChild())
    {
        if (auto* r = child->meshAsset.get().resolve())
        {
            tryBuildBody(r);
            if (physicsBody) onBodyCreated(); // base: register + sync
        }
    }
}

// ---------------------------------------------------------------------------
// Gizmo
// ---------------------------------------------------------------------------

void MMeshCollisionBody::onDrawGizmo(SVector2)
{
    MStaticMeshAsset* asset = nullptr;
    if (!meshAsset.get().isEmpty()) asset = meshAsset.get().resolve();
    if (!asset)
        if (auto* child = findMeshEntityChild())
            asset = child->meshAsset.get().resolve();
    if (!asset) return;

    const SColor color = isSensor.get() ? SColor(0.2f, 0.8f, 1.0f, 1.0f) : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    const SVector3 wPos = getWorldPosition();
    const SQuaternion wRot = getWorldRotation();
    const SVector3 wScale = getWorldScale();

    for (const MStaticMesh* mesh : asset->getMeshes())
    {
        const auto& verts   = mesh->getVertices();
        const auto& indices = mesh->getIndices();
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const SVector3 v0 = wPos + wRot * (verts[indices[i    ]].Position * wScale);
            const SVector3 v1 = wPos + wRot * (verts[indices[i + 1]].Position * wScale);
            const SVector3 v2 = wPos + wRot * (verts[indices[i + 2]].Position * wScale);
            MGizmos::drawLine(v0, v1, color, 1.0f, false);
            MGizmos::drawLine(v1, v2, color, 1.0f, false);
            MGizmos::drawLine(v2, v0, color, 1.0f, false);
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MMeshCollisionBody::tryBuildBody(MStaticMeshAsset* asset)
{
    if (!asset) { MERROR("MMeshCollisionBody::tryBuildBody — null asset"); return; }

    std::vector<SVector3> vertices;
    std::vector<uint32_t> indices;

    for (const MStaticMesh* mesh : asset->getMeshes())
    {
        const uint32_t base = static_cast<uint32_t>(vertices.size());
        for (const SVertex& v : mesh->getVertices())  vertices.push_back(v.Position);
        for (unsigned int i  : mesh->getIndices())    indices.push_back(base + i);
    }

    if (vertices.empty() || indices.empty())
    { MERROR("MMeshCollisionBody::tryBuildBody — mesh asset has no geometry"); return; }

    const SVector3 ws = getWorldScale();
    for (SVector3& v : vertices) { v.x *= ws.x; v.y *= ws.y; v.z *= ws.z; }

    SMeshBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.vertices          = std::move(vertices);
    settings.indices           = std::move(indices);

    physicsBody = physicsEngine->createMeshCollisionBody(settings);
    if (!physicsBody)
        MERROR("MMeshCollisionBody::tryBuildBody — failed to create mesh collision body");
    // Caller is responsible for calling onBodyCreated() if physicsBody != nullptr.
}

MStaticMeshEntity* MMeshCollisionBody::findMeshEntityChild() const
{
    for (MObject* child : const_cast<MMeshCollisionBody*>(this)->getChildren())
        if (auto* e = dynamic_cast<MStaticMeshEntity*>(child)) return e;
    return nullptr;
}