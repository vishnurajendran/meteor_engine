//
// convex_hull_collision_body_entity.cpp
//

#include "convexhull_collision_body_entity.h"

#include "core/application/application.h"
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/physics/interface/bodies/convexhull/convexhull_collsion_body.h"
#include "core/engine/subsystem/subsystem_registry.h"


IMPLEMENT_SPATIAL_CLASS(MConvexHullCollisionBody)

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MConvexHullCollisionBody::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MConvexHullCollisionBody::onCreate — physics engine subsystem not found");
        return;
    }

    initialized = true;
    setCanTick(true);
}

void MConvexHullCollisionBody::onStart()
{
    MSpatialEntity::onStart();

    if (!meshAsset.get().isEmpty())
    {
        auto* resolved = meshAsset.get().resolve();
        if (resolved)
        {
            tryBuildBody(resolved);
            setupFieldCallbacks();
            return;
        }
    }

    if (auto* child = findMeshEntityChild())
    {
        if (auto* resolved = child->meshAsset.get().resolve())
            tryBuildBody(resolved);
    }

    setupFieldCallbacks();
}

void MConvexHullCollisionBody::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (!initialized || !physicsBody) return;

    physicsBody->physicsTick(deltaTime);

    auto appInst = MApplication::getAppInstance();
    if (appInst && appInst->isPlaying())
    {
        setWorldPosition(physicsBody->getBodySyncPosition());
        setWorldRotation(physicsBody->getBodySyncRotation());
        return;
    }

    physicsBody->setPositionAndRotation(getWorldPosition(), getWorldRotation());
}

void MConvexHullCollisionBody::onFixedUpdate(float /*fixedDeltaTime*/)
{
    if (!initialized || physicsBody) return;

    if (auto* child = findMeshEntityChild())
    {
        if (auto* resolved = child->meshAsset.get().resolve())
            tryBuildBody(resolved);
    }
}

void MConvexHullCollisionBody::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    if (physicsBody)
    {
        physicsEngine->unregisterCallbackReceiver(physicsBody);
        physicsEngine->releaseConvexHullCollisionBody(physicsBody);
        physicsBody = nullptr;
    }

    physicsEngine = nullptr;
    initialized   = false;
}

void MConvexHullCollisionBody::onDrawGizmo(SVector2 /*res*/)
{
    // Resolve the source mesh asset. The gizmo draws the input mesh wireframe
    // as an approximation of the convex hull shape — the actual Jolt hull may
    // differ slightly at very concave features, but is otherwise a close match.
    MStaticMeshAsset* asset = nullptr;
    if (!meshAsset.get().isEmpty())
        asset = meshAsset.get().resolve();
    if (!asset)
        if (auto* child = findMeshEntityChild())
            asset = child->meshAsset.get().resolve();
    if (!asset) return;

    const SColor color = isSensor.get()
                         ? SColor(0.2f, 0.8f, 1.0f, 1.0f)
                         : SColor(0.2f, 1.0f, 0.2f, 1.0f);

    const SVector3    worldPos   = getWorldPosition();
    const SQuaternion worldRot   = getWorldRotation();
    const SVector3    worldScale = getWorldScale();

    auto toWorld = [&](const SVector3& localPos) -> SVector3
    {
        return worldPos + worldRot * (localPos * worldScale);
    };

    // Draw each triangle edge. Shared edges are drawn twice — acceptable for
    // a gizmo. See the same note in MMeshCollisionBody::onDrawGizmo regarding
    // performance on high-poly meshes.
    for (const MStaticMesh* mesh : asset->getMeshes())
    {
        const auto& verts   = mesh->getVertices();
        const auto& indices = mesh->getIndices();

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const SVector3 v0 = toWorld(verts[indices[i    ]].Position);
            const SVector3 v1 = toWorld(verts[indices[i + 1]].Position);
            const SVector3 v2 = toWorld(verts[indices[i + 2]].Position);

            MGizmos::drawLine(v0, v1, color, 1.0f, false);
            MGizmos::drawLine(v1, v2, color, 1.0f, false);
            MGizmos::drawLine(v2, v0, color, 1.0f, false);
        }
    }
}

// ---------------------------------------------------------------------------
// IPhysicsCallbackReceiver
// ---------------------------------------------------------------------------

void MConvexHullCollisionBody::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MConvexHullCollisionBody::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MConvexHullCollisionBody::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MConvexHullCollisionBody::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MConvexHullCollisionBody::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MConvexHullCollisionBody::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MConvexHullCollisionBody::tryBuildBody(MStaticMeshAsset* asset)
{
    if (!asset)
    {
        MERROR("MConvexHullCollisionBody::tryBuildBody — null asset");
        return;
    }

    std::vector<SVector3> points;
    for (const MStaticMesh* mesh : asset->getMeshes())
        for (const SVertex& v : mesh->getVertices())
            points.push_back(v.Position);

    if (points.size() < 4)
    {
        MERROR("MConvexHullCollisionBody::tryBuildBody — mesh has fewer than 4 vertices; cannot form a convex hull");
        return;
    }

    const SVector3 worldScale = getWorldScale();
    for (SVector3& p : points)
    {
        p.x *= worldScale.x;
        p.y *= worldScale.y;
        p.z *= worldScale.z;
    }

    SConvexHullBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.points            = std::move(points);

    physicsBody = physicsEngine->createConvexHullCollisionBody(settings);
    if (!physicsBody)
    {
        MERROR("MConvexHullCollisionBody::tryBuildBody — failed to create convex hull collision body");
        return;
    }

    physicsEngine->registerCallbackReceiver(physicsBody, this);
    syncFieldsToBody();
}

MStaticMeshEntity* MConvexHullCollisionBody::findMeshEntityChild() const
{
    for (MObject* child : const_cast<MConvexHullCollisionBody*>(this)->getChildren())
    {
        if (auto* meshEnt = dynamic_cast<MStaticMeshEntity*>(child))
            return meshEnt;
    }
    return nullptr;
}

void MConvexHullCollisionBody::setupFieldCallbacks()
{
    mass.setOnChangeCallback([this](auto v)             { if (physicsBody) physicsBody->setMass(v); });
    affectedByGravity.setOnChangeCallback([this](auto v){ if (physicsBody) physicsBody->enableGravity(v); });
    gravityScale.setOnChangeCallback([this](auto v)     { if (physicsBody) physicsBody->setGravity(v); });
    isSensor.setOnChangeCallback([this](auto v)         { if (physicsBody) physicsBody->setIsSensor(v); });
    linearDamping.setOnChangeCallback([this](auto v)    { if (physicsBody) physicsBody->setLinearDamping(v); });
    angularDamping.setOnChangeCallback([this](auto v)   { if (physicsBody) physicsBody->setAngularDamping(v); });

    bodyType.setOnChangeCallback([this](auto v)
    {
        if (!physicsEngine || !physicsBody) return;

        physicsEngine->unregisterCallbackReceiver(physicsBody);
        physicsEngine->releaseConvexHullCollisionBody(physicsBody);
        physicsBody = nullptr;

        auto* resolved = meshAsset.get().isEmpty()
                         ? (findMeshEntityChild() ? findMeshEntityChild()->meshAsset.get().resolve() : nullptr)
                         : meshAsset.get().resolve();

        if (resolved)
        {
            bodyType.set(v);
            tryBuildBody(resolved);
        }
    });

    meshAsset.setOnChangeCallback([this](auto ref)
    {
        if (physicsBody)
        {
            physicsEngine->unregisterCallbackReceiver(physicsBody);
            physicsEngine->releaseConvexHullCollisionBody(physicsBody);
            physicsBody = nullptr;
        }

        auto* resolved = ref.resolve();
        if (resolved)
            tryBuildBody(resolved);
    });
}

void MConvexHullCollisionBody::syncFieldsToBody()
{
    physicsBody->enableGravity(affectedByGravity.get());
    physicsBody->setGravity(gravityScale.get());
    physicsBody->setIsSensor(isSensor.get());
    physicsBody->setLinearDamping(linearDamping.get());
    physicsBody->setAngularDamping(angularDamping.get());
}