//
// collision_body_entity.cpp
//

#include "collision_body_entity.h"

#include "core/application/application.h"
#include "core/engine/subsystem/subsystem_registry.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MCollisionBodyEntity::onCreate()
{
    MSpatialEntity::onCreate();

    physicsEngine = MEngineSubsystemRegistry::getSubsystem<IPhysicsEngineSubsystem>();
    if (!physicsEngine)
    {
        MERROR("MCollisionBodyEntity::onCreate - physics engine subsystem not found");
        return;
    }

    initialized = true;
    setCanTick(true);
}

void MCollisionBodyEntity::onStart()
{
    MSpatialEntity::onStart();
    if (!initialized) return;

    // Always wire common field callbacks so they are ready whenever a body
    // eventually appears (including after deferred creation or re-enable).
    setupCommonCallbacks();

    // Let the subclass wire its shape-specific callbacks and, if needed,
    // replace the default bodyType onChange set by setupCommonCallbacks.
    setupShapeCallbacks();

    // Skip body creation if starting disabled — onEnable will handle it.
    if (!isEnabledInHierarchy()) return;

    // Let the subclass create its body. This may not produce a body immediately
    // (mesh/convex hull waiting for an asset) - that is expected.
    createCollisionBody();

    // Register + sync common fields only if a body was created.
    if (getBasePhysicsBody())
        onBodyCreated();
}

void MCollisionBodyEntity::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (!initialized) return;

    auto* body = getBasePhysicsBody();
    if (!body) return;

    body->physicsTick(deltaTime);

    auto appInst = MApplication::getAppInstance();
    if (appInst && appInst->isPlaying())
    {
        setWorldPosition(body->getBodySyncPosition());
        setWorldRotation(body->getBodySyncRotation());
        return;
    }

    body->setPositionAndRotation(getWorldPosition(), getWorldRotation());
}

void MCollisionBodyEntity::onExit()
{
    MSpatialEntity::onExit();
    if (!initialized) return;

    auto* body = getBasePhysicsBody();
    if (body)
    {
        physicsEngine->unregisterCallbackReceiver(body);
        releaseBody(); // subclass releases and nulls typed pointer
    }

    physicsEngine = nullptr;
    initialized   = false;
}

void MCollisionBodyEntity::onEnable()
{
    MSpatialEntity::onEnable();
    if (!initialized) return;

    if (!getBasePhysicsBody())
    {
        createCollisionBody();
        if (getBasePhysicsBody())
            onBodyCreated();
    }
}

void MCollisionBodyEntity::onDisable()
{
    MSpatialEntity::onDisable();
    if (!initialized) return;

    auto* body = getBasePhysicsBody();
    if (body)
    {
        physicsEngine->unregisterCallbackReceiver(body);
        releaseBody();
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MCollisionBodyEntity::onBodyCreated()
{
    auto* body = getBasePhysicsBody();
    if (!body || !physicsEngine) return;

    physicsEngine->registerCallbackReceiver(body, this,
        static_cast<unsigned int>(glm::clamp(physicsLayer.get(), 0, 31)));

    syncFieldsToBody();
}

void MCollisionBodyEntity::recreateBody()
{
    auto* body = getBasePhysicsBody();
    if (!physicsEngine) return;

    if (body) physicsEngine->unregisterCallbackReceiver(body);
    releaseBody(); // subclass sets typed pointer to nullptr

    createCollisionBody();

    if (getBasePhysicsBody())
        onBodyCreated();
}

void MCollisionBodyEntity::syncFieldsToBody()
{
    auto* body = getBasePhysicsBody();
    if (!body) return;

    body->enableGravity(affectedByGravity.get());
    body->setGravity(gravityScale.get());
    body->setIsSensor(isSensor.get());
    body->setLinearDamping(linearDamping.get());
    body->setAngularDamping(angularDamping.get());
}

void MCollisionBodyEntity::setupCommonCallbacks()
{
    mass.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->setMass(v); });

    affectedByGravity.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->enableGravity(v); });

    gravityScale.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->setGravity(v); });

    isSensor.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->setIsSensor(v); });

    linearDamping.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->setLinearDamping(v); });

    angularDamping.setOnChangeCallback([this](auto v)
        { auto* b = getBasePhysicsBody(); if (b) b->setAngularDamping(v); });

    // Re-registers with the new layer - does not recreate the Jolt body.
    physicsLayer.setOnChangeCallback([this](auto v)
    {
        const int clamped = glm::clamp(v, 0, 31);
        if (clamped != v) physicsLayer.set(clamped);

        auto* body = getBasePhysicsBody();
        if (physicsEngine && body)
            physicsEngine->registerCallbackReceiver(body, this,
                static_cast<unsigned int>(clamped));
    });

    // Default bodyType onChange - release and recreate via pure virtuals.
    // Shapes with constraints (e.g. mesh blocks Dynamic) override this in
    // setupShapeCallbacks(), which runs after this method and overwrites it.
    bodyType.setOnChangeCallback([this](auto)
    {
        recreateBody();
    });
}

// ---------------------------------------------------------------------------
// IPhysicsCallbackReceiver
// ---------------------------------------------------------------------------

void MCollisionBodyEntity::dispatchCollisionStart(const SCollisionData& data) { if (onCollisionStartCb) onCollisionStartCb(data); }
void MCollisionBodyEntity::dispatchCollisionStay (const SCollisionData& data) { if (onCollisionStayCb)  onCollisionStayCb(data);  }
void MCollisionBodyEntity::dispatchCollisionEnd  (const SCollisionData& data) { if (onCollisionEndCb)   onCollisionEndCb(data);   }
void MCollisionBodyEntity::dispatchTriggerStart  (const SOverlapData&   data) { if (onTriggerStartCb)   onTriggerStartCb(data);   }
void MCollisionBodyEntity::dispatchTriggerStay   (const SOverlapData&   data) { if (onTriggerStayCb)    onTriggerStayCb(data);    }
void MCollisionBodyEntity::dispatchTriggerEnd    (const SOverlapData&   data) { if (onTriggerEndCb)     onTriggerEndCb(data);     }