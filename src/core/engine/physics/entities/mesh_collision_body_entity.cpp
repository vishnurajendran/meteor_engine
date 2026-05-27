//
// mesh_collision_body_entity.cpp
//

#include "mesh_collision_body_entity.h"

#include "core/application/application.h"
#include "core/engine/3d/staticmesh/staticmesh.h"
#include "core/engine/3d/staticmesh/staticmeshentity.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"


IMPLEMENT_SPATIAL_CLASS(MMeshCollisionBody)

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MMeshCollisionBody::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MMeshCollisionBody::onCreate — physics engine subsystem not found");
        return;
    }

    initialized = true;
    setCanTick(true);
}

void MMeshCollisionBody::onStart()
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

void MMeshCollisionBody::onUpdate(float deltaTime)
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

void MMeshCollisionBody::onFixedUpdate(float /*fixedDeltaTime*/)
{
    if (!initialized || physicsBody) return;

    if (auto* child = findMeshEntityChild())
    {
        if (auto* resolved = child->meshAsset.get().resolve())
            tryBuildBody(resolved);
    }
}

void MMeshCollisionBody::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    if (physicsBody)
    {
        physicsEngine->unregisterCallbackReceiver(physicsBody);
        physicsEngine->releaseMeshCollisionBody(physicsBody);
        physicsBody = nullptr;
    }

    physicsEngine = nullptr;
    initialized   = false;
}

void MMeshCollisionBody::onDrawGizmo(SVector2 /*res*/)
{
    // Resolve the source mesh asset — explicit field takes priority over child.
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

    // Transform a local vertex position to world space. Scale is baked in
    // component-wise before rotating — this matches the physics body build.
    auto toWorld = [&](const SVector3& localPos) -> SVector3
    {
        return worldPos + worldRot * (localPos * worldScale);
    };

    // Draw every triangle edge from every submesh. Shared edges are drawn
    // twice — acceptable for a gizmo. For high-poly meshes (>~500 triangles)
    // this will produce many GL draw calls per frame; consider using a
    // batched line buffer if editor framerate is impacted.
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

void MMeshCollisionBody::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MMeshCollisionBody::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MMeshCollisionBody::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MMeshCollisionBody::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MMeshCollisionBody::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MMeshCollisionBody::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MMeshCollisionBody::tryBuildBody(MStaticMeshAsset* asset)
{
    if (!asset)
    {
        MERROR("MMeshCollisionBody::tryBuildBody — null asset");
        return;
    }

    std::vector<SVector3> vertices;
    std::vector<uint32_t> indices;

    for (const MStaticMesh* mesh : asset->getMeshes())
    {
        const uint32_t base = static_cast<uint32_t>(vertices.size());
        for (const SVertex& v : mesh->getVertices())
            vertices.push_back(v.Position);
        for (unsigned int idx : mesh->getIndices())
            indices.push_back(base + static_cast<uint32_t>(idx));
    }

    if (vertices.empty() || indices.empty())
    {
        MERROR("MMeshCollisionBody::tryBuildBody — mesh asset contains no geometry");
        return;
    }

    const SVector3 worldScale = getWorldScale();
    for (SVector3& v : vertices)
    {
        v.x *= worldScale.x;
        v.y *= worldScale.y;
        v.z *= worldScale.z;
    }

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
    {
        MERROR("MMeshCollisionBody::tryBuildBody — failed to create mesh collision body");
        return;
    }

    physicsEngine->registerCallbackReceiver(physicsBody, this);
    syncFieldsToBody();
}

MStaticMeshEntity* MMeshCollisionBody::findMeshEntityChild() const
{
    for (MObject* child : const_cast<MMeshCollisionBody*>(this)->getChildren())
    {
        if (auto* meshEnt = dynamic_cast<MStaticMeshEntity*>(child))
            return meshEnt;
    }
    return nullptr;
}

void MMeshCollisionBody::setupFieldCallbacks()
{
    mass.setOnChangeCallback([this](auto v)             { if (physicsBody) physicsBody->setMass(v); });
    affectedByGravity.setOnChangeCallback([this](auto v){ if (physicsBody) physicsBody->enableGravity(v); });
    gravityScale.setOnChangeCallback([this](auto v)     { if (physicsBody) physicsBody->setGravity(v); });
    isSensor.setOnChangeCallback([this](auto v)         { if (physicsBody) physicsBody->setIsSensor(v); });
    linearDamping.setOnChangeCallback([this](auto v)    { if (physicsBody) physicsBody->setLinearDamping(v); });
    angularDamping.setOnChangeCallback([this](auto v)   { if (physicsBody) physicsBody->setAngularDamping(v); });

    bodyType.setOnChangeCallback([this](auto v)
    {
        if (v == ECollisionBodyType::DynamicBody)
        {
            MWARN("MMeshCollisionBody::bodyType — MeshShape does not support Dynamic bodies; reverting to Static");
            bodyType.set(ECollisionBodyType::StaticBody);
            return;
        }

        if (!physicsEngine || !physicsBody) return;

        physicsEngine->unregisterCallbackReceiver(physicsBody);
        physicsEngine->releaseMeshCollisionBody(physicsBody);
        physicsBody = nullptr;

        auto* resolved = meshAsset.get().isEmpty()
                         ? (findMeshEntityChild() ? findMeshEntityChild()->meshAsset.get().resolve() : nullptr)
                         : meshAsset.get().resolve();

        if (resolved)
            tryBuildBody(resolved);
    });

    meshAsset.setOnChangeCallback([this](auto ref)
    {
        if (physicsBody)
        {
            physicsEngine->unregisterCallbackReceiver(physicsBody);
            physicsEngine->releaseMeshCollisionBody(physicsBody);
            physicsBody = nullptr;
        }

        auto* resolved = ref.resolve();
        if (resolved)
            tryBuildBody(resolved);
    });
}

void MMeshCollisionBody::syncFieldsToBody()
{
    physicsBody->enableGravity(affectedByGravity.get());
    physicsBody->setGravity(gravityScale.get());
    physicsBody->setIsSensor(isSensor.get());
    physicsBody->setLinearDamping(linearDamping.get());
    physicsBody->setAngularDamping(angularDamping.get());
}