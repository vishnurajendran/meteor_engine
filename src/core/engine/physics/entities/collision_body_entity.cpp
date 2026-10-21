//
// collision_body_entity.cpp
//

#include "collision_body_entity.h"

#include <glm/gtc/quaternion.hpp>

#include "core/application/application.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/scripting/scripting_call_symbols.h"

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

// In onAwake, not onStart, so the body exists before any script's onStart
// looks for it - e.g. a parent's CarController finding its physics child.
void MCollisionBodyEntity::onAwake()
{
    MSpatialEntity::onAwake();
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

    // While simulating, the pose comes from onPhysicsStepped /
    // onPhysicsInterpolate, which run before any onUpdate.
    auto appInst = MApplication::getAppInstance();
    if (appInst && appInst->isSimulating())
        return;

    body->physicsTick(deltaTime);
    body->setPositionAndRotation(getWorldPosition(), getWorldRotation());
}

void MCollisionBodyEntity::onPhysicsStepped(float fixedDeltaTime)
{
    MSpatialEntity::onPhysicsStepped(fixedDeltaTime);
    if (!initialized) return;

    auto* body = getBasePhysicsBody();
    if (!body) return;

    // Reads the new pose and velocities and applies axis locks - once per
    // step now, rather than once per frame.
    body->physicsTick(fixedDeltaTime);

    prevBodyPosition = currBodyPosition;
    prevBodyRotation = currBodyRotation;
    currBodyPosition = body->getBodySyncPosition();
    currBodyRotation = body->getBodySyncRotation();
    if (!hasBodySnapshot)
    {
        // first step - nothing earlier to blend from
        prevBodyPosition = currBodyPosition;
        prevBodyRotation = currBodyRotation;
        hasBodySnapshot  = true;
    }
}

void MCollisionBodyEntity::onPhysicsInterpolate(float alpha)
{
    MSpatialEntity::onPhysicsInterpolate(alpha);
    if (!initialized || !hasBodySnapshot) return;
    if (!getBasePhysicsBody()) return;

    setWorldPosition(glm::mix(prevBodyPosition, currBodyPosition, alpha));
    setWorldRotation(glm::slerp(prevBodyRotation, currBodyRotation, alpha));
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

    // a new body starts from the entity's pose - don't blend from the old one
    hasBodySnapshot = false;

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
    // Event toggles: no body involved - the contact listener reads the
    // atomics. Seeded from the fields (already loaded from the scene) here.
    collisionEventsEnabled.store(receiveCollisionEvents.get());
    triggerEventsEnabled.store(receiveTriggerEvents.get());
    receiveCollisionEvents.setOnChangeCallback([this](auto v) { collisionEventsEnabled.store(v); });
    receiveTriggerEvents.setOnChangeCallback([this](auto v)   { triggerEventsEnabled.store(v); });

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

// Each contact goes to C++ listeners (the delegates), then to this entity's
// own script as a Unity-style message, if it defines the function.

void MCollisionBodyEntity::onStart()
{
    MSpatialEntity::onStart();   // starts the script
    warnAboutDisabledScriptMessages();
}

void MCollisionBodyEntity::warnAboutDisabledScriptMessages() const
{
    const IScriptInstance* script = getScriptInstance();
    if (!script || !script->isValid())
        return;

    using namespace ScriptinCallSymbols;
    const auto definesAny = [script](std::initializer_list<SString> names)
    {
        for (const auto& name : names)
            if (script->hasFunc(name))
                return true;
        return false;
    };

    if (!receiveCollisionEvents.get() && definesAny({ SCRIPT_FUNC_COLLISION_ENTER, SCRIPT_FUNC_COLLISION_STAY, SCRIPT_FUNC_COLLISION_EXIT }))
        MWARN(SString::format("'{0}': its script handles collision messages, but 'Receive Collision Events' is off - they will not be called.", getName()));

    if (!receiveTriggerEvents.get() && definesAny({ SCRIPT_FUNC_TRIGGER_ENTER, SCRIPT_FUNC_TRIGGER_STAY, SCRIPT_FUNC_TRIGGER_EXIT }))
        MWARN(SString::format("'{0}': its script handles trigger messages, but 'Receive Trigger Events' is off - they will not be called.", getName()));
}

IScriptInstance* MCollisionBodyEntity::getScriptForMessages() const
{
    if (!hasStarted() || hasExited())
        return nullptr;
    IScriptInstance* script = getScriptInstance();
    return script && script->isValid() ? script : nullptr;
}

void MCollisionBodyEntity::dispatchCollisionStart(const SCollisionData& data)
{
    onCollisionStart.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendCollisionMessage(ScriptinCallSymbols::SCRIPT_FUNC_COLLISION_ENTER, data);
}

void MCollisionBodyEntity::dispatchCollisionStay(const SCollisionData& data)
{
    onCollisionStay.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendCollisionMessage(ScriptinCallSymbols::SCRIPT_FUNC_COLLISION_STAY, data);
}

void MCollisionBodyEntity::dispatchCollisionEnd(const SCollisionData& data)
{
    onCollisionEnd.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendCollisionMessage(ScriptinCallSymbols::SCRIPT_FUNC_COLLISION_EXIT, data);
}

void MCollisionBodyEntity::dispatchTriggerStart(const SOverlapData& data)
{
    onTriggerStart.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendTriggerMessage(ScriptinCallSymbols::SCRIPT_FUNC_TRIGGER_ENTER, data);
}

void MCollisionBodyEntity::dispatchTriggerStay(const SOverlapData& data)
{
    onTriggerStay.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendTriggerMessage(ScriptinCallSymbols::SCRIPT_FUNC_TRIGGER_STAY, data);
}

void MCollisionBodyEntity::dispatchTriggerEnd(const SOverlapData& data)
{
    onTriggerEnd.broadcast(data);
    if (auto* script = getScriptForMessages())
        script->sendTriggerMessage(ScriptinCallSymbols::SCRIPT_FUNC_TRIGGER_EXIT, data);
}