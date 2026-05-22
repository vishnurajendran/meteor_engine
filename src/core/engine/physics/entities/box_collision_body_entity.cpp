//
// Created by ssj5v on 22-05-2026.
//

#include "box_collision_body_entity.h"

#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"

IMPLEMENT_SPATIAL_CLASS(MBoxCollisionBody)

// ---------------------------------------------------------------------------
// Scene creation helpers
// ---------------------------------------------------------------------------

MBoxCollisionBody* MBoxCollisionBody::create(ECollisionBodyType type,
                                             SVector3           halfExtents,
                                             float              mass)
{
    // Fields are set before the entity reaches onCreate, so the physics body
    // is constructed with these values rather than the DECLARE_FIELD defaults.
    auto* entity = new MBoxCollisionBody();
    entity->bodyType.set(type);
    entity->halfExtentX.set(halfExtents.x);
    entity->halfExtentY.set(halfExtents.y);
    entity->halfExtentZ.set(halfExtents.z);
    entity->mass.set(mass);
    return entity;
}

MBoxCollisionBody* MBoxCollisionBody::createDynamic(SVector3 halfExtents, float mass)
{
    auto* entity = create(ECollisionBodyType::DynamicBody, halfExtents, mass);
    entity->affectedByGravity.set(true);
    return entity;
}

MBoxCollisionBody* MBoxCollisionBody::createStatic(SVector3 halfExtents)
{
    return create(ECollisionBodyType::StaticBody, halfExtents, 0.0f);
}

MBoxCollisionBody* MBoxCollisionBody::createTrigger(SVector3 halfExtents)
{
    auto* entity = create(ECollisionBodyType::StaticBody, halfExtents, 0.0f);
    entity->isSensor.set(true);
    return entity;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MBoxCollisionBody::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MBoxCollisionBody::onCreate — physics engine subsystem not found");
        return;
    }

    // Factor in relative scale immediately upon creation
    const auto& scale = getRelativeScale();

    SBoxPhysicsBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.bounds            = {
        .min = { -halfExtentX.get() * scale.x, -halfExtentY.get() * scale.y, -halfExtentZ.get() * scale.z },
        .max = {  halfExtentX.get() * scale.x,  halfExtentY.get() * scale.y,  halfExtentZ.get() * scale.z }
    };

    physicsBody = physicsEngine->createBoxCollider(settings);
    if (!physicsBody)
    {
        MERROR("MBoxCollisionBody::onCreate — failed to create box collider");
        return;
    }

    physicsEngine->registerCallbackReceiver(physicsBody, this);

    syncFieldsToBody();

    mass.setOnChangeCallback([this](auto v)             { if (physicsBody) physicsBody->setMass(v); });
    affectedByGravity.setOnChangeCallback([this](auto v){ if (physicsBody) physicsBody->enableGravity(v); });
    gravityScale.setOnChangeCallback([this](auto v)     { if (physicsBody) physicsBody->setGravity(v); });
    isSensor.setOnChangeCallback([this](auto v)         { if (physicsBody) physicsBody->setIsSensor(v); });
    linearDamping.setOnChangeCallback([this](auto v)    { if (physicsBody) physicsBody->setLinearDamping(v); });
    angularDamping.setOnChangeCallback([this](auto v)   { if (physicsBody) physicsBody->setAngularDamping(v); });
    bodyType.setOnChangeCallback([this](auto v)         { if (physicsBody) physicsBody->setBodyType(v); });

    halfExtentX.setOnChangeCallback([this](auto) { syncBounds(); });
    halfExtentY.setOnChangeCallback([this](auto) { syncBounds(); });
    halfExtentZ.setOnChangeCallback([this](auto) { syncBounds(); });

    initialized = true;
    setCanTick(true);
}

// Implement the updateTransforms cascade to catch scale modifications
void MBoxCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody)
    {
        syncBounds();
    }
}

void MBoxCollisionBody::onStart()
{
    MSpatialEntity::onStart();
}

void MBoxCollisionBody::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (!initialized) return;

    physicsBody->physicsTick(deltaTime);

    switch (bodyType.get())
    {
    case ECollisionBodyType::DynamicBody:
        setWorldPosition(physicsBody->getBodySyncPosition());
        setWorldRotation(physicsBody->getBodySyncRotation());
        break;
    case ECollisionBodyType::KinematicBody:
        physicsBody->setPositionAndRotation(getWorldPosition(), getWorldRotation());
        break;
    case ECollisionBodyType::StaticBody:
        break;
    }
}

void MBoxCollisionBody::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    physicsEngine->unregisterCallbackReceiver(physicsBody);
    physicsEngine->releaseBoxCollider(physicsBody);

    physicsBody   = nullptr;
    physicsEngine = nullptr;
    initialized   = false;
}

void MBoxCollisionBody::onDrawGizmo(SVector2 res)
{
    const auto& scale = getRelativeScale();
    const SVector3 halfExt = { halfExtentX.get() * scale.x, halfExtentY.get() * scale.y, halfExtentZ.get() * scale.z };
    const SColor   color   = isSensor.get()
                             ? SColor(0.2f, 0.8f, 1.0f, 1.0f)
                             : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    MGizmos::drawWireCube(getWorldPosition(), halfExt, color, 1.0f);
}

// ---------------------------------------------------------------------------
// IPhysicsCallbackReceiver
// ---------------------------------------------------------------------------

void MBoxCollisionBody::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MBoxCollisionBody::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MBoxCollisionBody::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MBoxCollisionBody::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MBoxCollisionBody::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MBoxCollisionBody::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MBoxCollisionBody::syncBounds()
{
    if (!physicsBody) return;
    const auto& scale = getRelativeScale();
    physicsBody->setBounds({
        .min = { -halfExtentX.get() * scale.x, -halfExtentY.get() * scale.y, -halfExtentZ.get() * scale.z },
        .max = {  halfExtentX.get() * scale.x,  halfExtentY.get() * scale.y,  halfExtentZ.get() * scale.z }
    });
}

void MBoxCollisionBody::syncFieldsToBody()
{
    physicsBody->enableGravity(affectedByGravity.get());
    physicsBody->setGravity(gravityScale.get());
    physicsBody->setIsSensor(isSensor.get());
    physicsBody->setLinearDamping(linearDamping.get());
    physicsBody->setAngularDamping(angularDamping.get());
}