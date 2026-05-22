//
// Created by ssj5v on 22-05-2026.
//

#include "sphere_collision_body_entity.h"

#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"

IMPLEMENT_SPATIAL_CLASS(MSphereCollisionBody)

// ---------------------------------------------------------------------------
// Scene creation helpers
// ---------------------------------------------------------------------------

MSphereCollisionBody* MSphereCollisionBody::create(ECollisionBodyType type,
                                                   float              radius,
                                                   float              mass)
{
    auto* entity = new MSphereCollisionBody();
    entity->bodyType.set(type);
    entity->radius.set(radius);
    entity->mass.set(mass);
    return entity;
}

MSphereCollisionBody* MSphereCollisionBody::createDynamic(float radius, float mass)
{
    auto* entity = create(ECollisionBodyType::DynamicBody, radius, mass);
    entity->affectedByGravity.set(true);
    return entity;
}

MSphereCollisionBody* MSphereCollisionBody::createStatic(float radius)
{
    return create(ECollisionBodyType::StaticBody, radius, 0.0f);
}

MSphereCollisionBody* MSphereCollisionBody::createTrigger(float radius)
{
    auto* entity = create(ECollisionBodyType::StaticBody, radius, 0.0f);
    entity->isSensor.set(true);
    return entity;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MSphereCollisionBody::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MSphereCollisionBody::onCreate — physics engine subsystem not found");
        return;
    }

    // Determine absolute uniform multiplier from scale vector components
    const auto& scale = getRelativeScale();
    float maxScale = std::max({scale.x, scale.y, scale.z});

    SSphereBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.radius            = radius.get() * maxScale;

    physicsBody = physicsEngine->createSphereCollisionBody(settings);
    if (!physicsBody)
    {
        MERROR("MSphereCollisionBody::onCreate — failed to create sphere collider");
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
    radius.setOnChangeCallback([this](auto)             { syncRadius(); });

    initialized = true;
    setCanTick(true);
}

void MSphereCollisionBody::syncRadius()
{
    if (!physicsBody) return;
    const auto& scale = getRelativeScale();
    float maxScale = std::max({scale.x, scale.y, scale.z});
    physicsBody->setRadius(radius.get() * maxScale);
}

void MSphereCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody)
    {
        syncRadius();
    }
}

// Ensure the debug gizmo renders at the actual scaled sizing too!
void MSphereCollisionBody::onDrawGizmo(SVector2 res)
{
    const SColor color = isSensor.get()
                         ? SColor(0.2f, 0.8f, 1.0f, 1.0f)
                         : SColor(0.2f, 1.0f, 0.2f, 1.0f);
    const auto& scale = getRelativeScale();
    float maxScale = std::max({scale.x, scale.y, scale.z});

    MGizmos::drawWireSphere(getWorldPosition(), radius.get() * maxScale, color, 1.0f);
}

void MSphereCollisionBody::onStart()
{
    MSpatialEntity::onStart();
}

void MSphereCollisionBody::onUpdate(float deltaTime)
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

void MSphereCollisionBody::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    physicsEngine->unregisterCallbackReceiver(physicsBody);
    physicsEngine->releaseSphereCollisionBody(physicsBody);

    physicsBody   = nullptr;
    physicsEngine = nullptr;
    initialized   = false;
}

void MSphereCollisionBody::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MSphereCollisionBody::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MSphereCollisionBody::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MSphereCollisionBody::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MSphereCollisionBody::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MSphereCollisionBody::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }

void MSphereCollisionBody::syncFieldsToBody()
{
    physicsBody->enableGravity(affectedByGravity.get());
    physicsBody->setGravity(gravityScale.get());
    physicsBody->setIsSensor(isSensor.get());
    physicsBody->setLinearDamping(linearDamping.get());
    physicsBody->setAngularDamping(angularDamping.get());
}