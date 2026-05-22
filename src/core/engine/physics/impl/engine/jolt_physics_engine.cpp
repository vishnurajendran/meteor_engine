//
// Created by ssj5v on 21-05-2026.
//

#include "jolt_physics_engine.h"

#include <Jolt/Jolt.h>
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"


#include "core/engine/physics/impl/bodies/jolt_base_collision_body.h"
#include "core/engine/physics/impl/bodies/jolt_box_collision.h"
#include "core/engine/physics/impl/bodies/jolt_sphere_collision.h"

void MJoltPhysicsEngine::init()
{
    MLOG("MJoltPhysicsEngine:: Initializing Physics engine");
    MLOG("MJoltPhysicsEngine:: Registering Allocator");
    RegisterDefaultAllocator();

    MLOG("MJoltPhysicsEngine:: Creating factory");
    Factory::sInstance = new Factory();
    MLOG("MJoltPhysicsEngine:: Registering types");
    RegisterTypes();

    MLOG("MJoltPhysicsEngine:: Engine Init");
    physicsSystem.Init(maxPhysicsBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints,
                       broadPhaseLayerInterface, objVsBroadPhaseLayerFilter, objVsObjLayerFilter);

    MLOG("MJoltPhysicsEngine:: Setting activation listener");
    physicsSystem.SetBodyActivationListener(&bodyActivationListener);

    MLOG("MJoltPhysicsEngine:: Setting contact listener");
    physicsSystem.SetContactListener(&contactListener);

    MLOG("MJoltPhysicsEngine:: Fetching body interface");
    bodyInterface = &physicsSystem.GetBodyInterface();

    // Wire the contact listener to the receiver registry so it can dispatch events.
    MLOG("MJoltPhysicsEngine:: Init contact listener");
    contactListener.init(&receiverRegistry, &receiverRegistryMutex);
    MLOG("MJoltPhysicsEngine:: Initialized Physics engine");
}

void MJoltPhysicsEngine::cleanup()
{
    MLOG("MJoltPhysicsEngine:: Cleanup");
    releaseAllBodies();
    UnregisterTypes();

    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}

void MJoltPhysicsEngine::releaseAllBodies()
{
    MLOG("MJoltPhysicsEngine:: Releasing all bodies");
    for (const auto& bodyId : bodies)
    {
        bodyInterface->RemoveBody(bodyId);
        bodyInterface->DestroyBody(bodyId);
    }
    bodies.clear();
}

void MJoltPhysicsEngine::tick(float fixedDeltaTime)
{
    physicsSystem.Update(fixedDeltaTime, 1, &tempAlloc, &jobSystem);

    // Contact callbacks were queued on worker threads during Update() — dispatch
    // them now on the main thread so entity callbacks always run here.
    dispatchPendingEvents();
}

void MJoltPhysicsEngine::dispatchPendingEvents()
{
    std::vector<SPendingContactEvent> events;
    contactListener.drainPendingEvents(events);

    for (const auto& e : events)
    {
        if (!e.receiver) continue;

        switch (e.type)
        {
        case SPendingContactEvent::EType::CollisionStart: e.receiver->dispatchCollisionStart(e.collisionData); break;
        case SPendingContactEvent::EType::CollisionStay:  e.receiver->dispatchCollisionStay (e.collisionData); break;
        case SPendingContactEvent::EType::CollisionEnd:   e.receiver->dispatchCollisionEnd  (e.collisionData); break;
        case SPendingContactEvent::EType::TriggerStart:   e.receiver->dispatchTriggerStart  (e.overlapData);   break;
        case SPendingContactEvent::EType::TriggerStay:    e.receiver->dispatchTriggerStay   (e.overlapData);   break;
        case SPendingContactEvent::EType::TriggerEnd:     e.receiver->dispatchTriggerEnd    (e.overlapData);   break;
        }
    }
}

IBoxCollisionBody* MJoltPhysicsEngine::createBoxCollider(const SBoxPhysicsBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating box collider");

    const auto& aabb = settings.bounds;
    const float halfX = (aabb.max.x - aabb.min.x) * 0.5f;
    const float halfY = (aabb.max.y - aabb.min.y) * 0.5f;
    const float halfZ = (aabb.max.z - aabb.min.z) * 0.5f;

    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    BoxShapeSettings     boxShapeSettings(Vec3(halfX, halfY, halfZ));
    BodyCreationSettings bodySettings(&boxShapeSettings, position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltBoxCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseBoxCollider(IBoxCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing box collider");
    const auto* joltBody = dynamic_cast<MJoltBoxCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

ISphereCollisionBody* MJoltPhysicsEngine::createSphereCollisionBody(const SSphereBodySettings& settings)
{
    MLOG("MJoltPhysicsEngine:: Creating sphere collider");
    EMotionType motionType  = EMotionType::Static;
    ObjectLayer objectLayer = Layers::STATIC_BODY;

    switch (settings.bodyType)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    objectLayer = Layers::STATIC_BODY;  break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   objectLayer = Layers::DYNAMIC_BODY; break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; objectLayer = Layers::DYNAMIC_BODY; break;
    }

    const RVec3 position = { settings.position.x, settings.position.y, settings.position.z };
    const Quat  rotation = { settings.rotation.x, settings.rotation.y, settings.rotation.z, settings.rotation.w };

    SphereShapeSettings  sphereShapeSettings(settings.radius);
    BodyCreationSettings bodySettings(&sphereShapeSettings, position, rotation, motionType, objectLayer);

    BodyID bodyId;
    if (createPhysicsBody(bodySettings, EActivation::Activate, bodyId))
        return new MJoltSphereCollision(bodyId, *bodyInterface, physicsSystem);

    return nullptr;
}

void MJoltPhysicsEngine::releaseSphereCollisionBody(ISphereCollisionBody* body)
{
    if (!body || !bodyInterface) return;

    MLOG("MJoltPhysicsEngine:: Releasing sphere collider");
    const auto* joltBody = dynamic_cast<MJoltSphereCollision*>(body);
    if (!joltBody) return;

    const BodyID bodyId = joltBody->getBodyID();
    bodyInterface->RemoveBody(bodyId);
    bodyInterface->DestroyBody(bodyId);
    bodies.erase(bodyId);

    delete joltBody;
}

void MJoltPhysicsEngine::registerCallbackReceiver(ICollisionBody* body,
                                                  IPhysicsCallbackReceiver* receiver)
{
    if (!body || !receiver) return;

    // Cast to the Jolt base to get the body index — this is internal knowledge
    // that belongs in the concrete engine, not the abstract interface.
    const auto* joltBody = dynamic_cast<MJoltBaseCollisionBody*>(body);
    if (!joltBody) return;

    std::unique_lock lock(receiverRegistryMutex);
    receiverRegistry[joltBody->getJoltBodyIndex()] = receiver;
}

void MJoltPhysicsEngine::unregisterCallbackReceiver(ICollisionBody* body)
{
    if (!body) return;

    const auto* joltBody = dynamic_cast<MJoltBaseCollisionBody*>(body);
    if (!joltBody) return;

    std::unique_lock lock(receiverRegistryMutex);
    receiverRegistry.erase(joltBody->getJoltBodyIndex());
}

bool MJoltPhysicsEngine::createPhysicsBody(const BodyCreationSettings& bodyCreationSetting,
                                           const EActivation activation, BodyID& container)
{
    if (!bodyInterface) return false;

    container = bodyInterface->CreateAndAddBody(bodyCreationSetting, activation);

    if (container.IsInvalid())
        return false;

    bodies.insert(container);
    return true;
}