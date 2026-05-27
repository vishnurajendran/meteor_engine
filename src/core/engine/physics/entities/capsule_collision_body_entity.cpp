//
// capsule_collision_body_entity.cpp
//

#include "capsule_collision_body_entity.h"

#include <algorithm>
#include "core/application/application.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/engine/subsystem/subsystem_registry.h"

IMPLEMENT_SPATIAL_CLASS(MCapsuleCollisionBody)

// ---------------------------------------------------------------------------
// Scene creation helpers
// ---------------------------------------------------------------------------

MCapsuleCollisionBody* MCapsuleCollisionBody::create(ECollisionBodyType type,
                                                     float              halfHeight,
                                                     float              radius,
                                                     float              mass)
{
    auto* entity = new MCapsuleCollisionBody();
    entity->bodyType.set(type);
    entity->halfHeight.set(halfHeight);
    entity->radius.set(radius);
    entity->mass.set(mass);
    return entity;
}

MCapsuleCollisionBody* MCapsuleCollisionBody::createDynamic(float halfHeight, float radius, float mass)
{
    auto* entity = create(ECollisionBodyType::DynamicBody, halfHeight, radius, mass);
    entity->affectedByGravity.set(true);
    return entity;
}

MCapsuleCollisionBody* MCapsuleCollisionBody::createStatic(float halfHeight, float radius)
{
    return create(ECollisionBodyType::StaticBody, halfHeight, radius, 0.0f);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MCapsuleCollisionBody::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MCapsuleCollisionBody::onCreate — physics engine subsystem not found");
        return;
    }

    initialized = true;
    setCanTick(true);
}

void MCapsuleCollisionBody::updateTransforms()
{
    MSpatialEntity::updateTransforms();
    if (initialized && physicsBody)
        syncShape();
}

void MCapsuleCollisionBody::onStart()
{
    MSpatialEntity::onStart();
    createCollisionBody();
}

void MCapsuleCollisionBody::createCollisionBody()
{
    updateTransforms();

    const auto& scale = getWorldScale();
    float scaleR, scaleH;
    switch (axis.get())
    {
    case EShapeAxis::X: scaleR = std::max(scale.y, scale.z); scaleH = scale.x; break;
    case EShapeAxis::Z: scaleR = std::max(scale.x, scale.y); scaleH = scale.z; break;
    default:            scaleR = std::max(scale.x, scale.z); scaleH = scale.y; break;
    }

    SCapsuleBodySettings settings;
    settings.position          = getWorldPosition();
    settings.rotation          = getWorldRotation();
    settings.bodyType          = bodyType.get();
    settings.mass              = mass.get();
    settings.affectedByGravity = affectedByGravity.get();
    settings.halfHeight        = halfHeight.get() * scaleH;
    settings.radius            = radius.get()     * scaleR;
    settings.axis              = axis.get();

    physicsBody = physicsEngine->createCapsuleCollisionBody(settings);
    if (!physicsBody)
    {
        MERROR("MCapsuleCollisionBody::createCollisionBody — failed to create capsule collider");
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

    halfHeight.setOnChangeCallback([this](auto) { syncShape(); });
    radius.setOnChangeCallback([this](auto)     { syncShape(); });

    axis.setOnChangeCallback([this](auto v)
    {
        if (!physicsBody) return;
        physicsBody->setAxis(v);
        syncShape();
    });

    bodyType.setOnChangeCallback([this](auto v)
    {
        if (!physicsEngine || !physicsBody) return;

        physicsEngine->unregisterCallbackReceiver(physicsBody);
        physicsEngine->releaseCapsuleCollisionBody(physicsBody);
        physicsBody = nullptr;

        const auto& scale = getWorldScale();
        float scaleR, scaleH;
        switch (axis.get())
        {
        case EShapeAxis::X: scaleR = std::max(scale.y, scale.z); scaleH = scale.x; break;
        case EShapeAxis::Z: scaleR = std::max(scale.x, scale.y); scaleH = scale.z; break;
        default:            scaleR = std::max(scale.x, scale.z); scaleH = scale.y; break;
        }

        SCapsuleBodySettings settings;
        settings.position          = getWorldPosition();
        settings.rotation          = getWorldRotation();
        settings.bodyType          = v;
        settings.mass              = mass.get();
        settings.affectedByGravity = affectedByGravity.get();
        settings.halfHeight        = halfHeight.get() * scaleH;
        settings.radius            = radius.get()     * scaleR;
        settings.axis              = axis.get();

        physicsBody = physicsEngine->createCapsuleCollisionBody(settings);
        if (!physicsBody)
        {
            MERROR("MCapsuleCollisionBody::bodyType onChange — failed to recreate capsule collider");
            return;
        }
        physicsEngine->registerCallbackReceiver(physicsBody, this);
        syncFieldsToBody();
    });
}

void MCapsuleCollisionBody::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (!initialized) return;

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

void MCapsuleCollisionBody::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    physicsEngine->unregisterCallbackReceiver(physicsBody);
    physicsEngine->releaseCapsuleCollisionBody(physicsBody);

    physicsBody   = nullptr;
    physicsEngine = nullptr;
    initialized   = false;
}

void MCapsuleCollisionBody::onDrawGizmo(SVector2 res)
{
    const auto& scale = getRelativeScale();
    float scaleR, scaleH;
    switch (axis.get())
    {
    case EShapeAxis::X: scaleR = std::max(scale.y, scale.z); scaleH = scale.x; break;
    case EShapeAxis::Z: scaleR = std::max(scale.x, scale.y); scaleH = scale.z; break;
    default:            scaleR = std::max(scale.x, scale.z); scaleH = scale.y; break;
    }

    const float scaledR = radius.get()     * scaleR;
    const float scaledH = halfHeight.get() * scaleH;

    SVector3 localAxis;
    switch (axis.get())
    {
    case EShapeAxis::X: localAxis = {1.0f, 0.0f, 0.0f}; break;
    case EShapeAxis::Z: localAxis = {0.0f, 0.0f, 1.0f}; break;
    default:            localAxis = {0.0f, 1.0f, 0.0f}; break;
    }
    const SVector3 worldAxis = getWorldRotation() * localAxis;

    const SColor color = isSensor.get()
                         ? SColor(0.2f, 0.8f, 1.0f, 1.0f)
                         : SColor(0.2f, 1.0f, 0.2f, 1.0f);

    MGizmos::drawWireCapsule(getWorldPosition(), scaledH, scaledR, worldAxis, color, 1.0f);
}

// ---------------------------------------------------------------------------
// IPhysicsCallbackReceiver
// ---------------------------------------------------------------------------

void MCapsuleCollisionBody::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MCapsuleCollisionBody::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MCapsuleCollisionBody::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MCapsuleCollisionBody::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MCapsuleCollisionBody::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MCapsuleCollisionBody::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MCapsuleCollisionBody::syncShape()
{
    if (!physicsBody) return;

    const auto& scale = getRelativeScale();
    float scaleR, scaleH;
    switch (axis.get())
    {
    case EShapeAxis::X: scaleR = std::max(scale.y, scale.z); scaleH = scale.x; break;
    case EShapeAxis::Z: scaleR = std::max(scale.x, scale.y); scaleH = scale.z; break;
    default:            scaleR = std::max(scale.x, scale.z); scaleH = scale.y; break;
    }

    physicsBody->setRadius(radius.get()         * scaleR);
    physicsBody->setHalfHeight(halfHeight.get() * scaleH);
}

void MCapsuleCollisionBody::syncFieldsToBody()
{
    physicsBody->enableGravity(affectedByGravity.get());
    physicsBody->setGravity(gravityScale.get());
    physicsBody->setIsSensor(isSensor.get());
    physicsBody->setLinearDamping(linearDamping.get());
    physicsBody->setAngularDamping(angularDamping.get());
}