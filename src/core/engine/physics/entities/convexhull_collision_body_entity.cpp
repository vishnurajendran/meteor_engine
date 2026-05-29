//
// convexhull_collision_body_entity.cpp
//

#include "convexhull_collision_body_entity.h"
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/gizmos/gizmos.h"

IMPLEMENT_SPATIAL_CLASS(MConvexHullCollisionBody)

void MConvexHullCollisionBody::createCollisionBody()
{
    if (!meshAsset.get().isEmpty())
        if (auto* r = meshAsset.get().resolve()) { tryBuildBody(r); return; }
    if (auto* child = findMeshEntityChild())
        if (auto* r = child->meshAsset.get().resolve()) tryBuildBody(r);
}

void MConvexHullCollisionBody::releaseBody()
{
    if (!physicsBody) return;
    physicsEngine->releaseConvexHullCollisionBody(physicsBody);
    physicsBody = nullptr;
}

void MConvexHullCollisionBody::setupShapeCallbacks()
{
    // ConvexHull supports all body types — no Dynamic block needed.
    // The default bodyType onChange from the base is sufficient.
    meshAsset.setOnChangeCallback([this](auto) { recreateBody(); });
}

void MConvexHullCollisionBody::onFixedUpdate(float)
{
    if (!initialized || physicsBody) return;
    if (auto* child = findMeshEntityChild())
    {
        if (auto* r = child->meshAsset.get().resolve())
        {
            tryBuildBody(r);
            if (physicsBody) onBodyCreated();
        }
    }
}

void MConvexHullCollisionBody::onDrawGizmo(SVector2)
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

void MConvexHullCollisionBody::tryBuildBody(MStaticMeshAsset* asset)
{
    if (!asset) { MERROR("MConvexHullCollisionBody::tryBuildBody — null asset"); return; }

    std::vector<SVector3> points;
    for (const MStaticMesh* mesh : asset->getMeshes())
        for (const SVertex& v : mesh->getVertices())
            points.push_back(v.Position);

    if (points.size() < 4)
    { MERROR("MConvexHullCollisionBody::tryBuildBody — fewer than 4 vertices"); return; }

    const SVector3 ws = getWorldScale();
    for (SVector3& p : points) { p.x *= ws.x; p.y *= ws.y; p.z *= ws.z; }

    SConvexHullBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.points            = std::move(points);

    physicsBody = physicsEngine->createConvexHullCollisionBody(settings);
    if (!physicsBody)
        MERROR("MConvexHullCollisionBody::tryBuildBody — failed to create convex hull body");
}

MStaticMeshEntity* MConvexHullCollisionBody::findMeshEntityChild() const
{
    for (MObject* child : const_cast<MConvexHullCollisionBody*>(this)->getChildren())
        if (auto* e = dynamic_cast<MStaticMeshEntity*>(child)) return e;
    return nullptr;
}