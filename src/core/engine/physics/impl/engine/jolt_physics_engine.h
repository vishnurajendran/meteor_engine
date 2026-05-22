//
// Created by ssj5v on 21-05-2026.
//

#ifndef JOLT_PHYSICS_ENGINE_H
#define JOLT_PHYSICS_ENGINE_H

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/object/object.h"
#include "core/engine/physics/interface/physics_engine_interface.h"
#include "core/engine/physics/impl/body_activation_listener.h"
#include "core/engine/physics/impl/contact_listener.h"
#include "core/engine/physics/impl/jolt_layers_filters.h"

JPH_SUPPRESS_WARNINGS

using namespace JPH;
using namespace literals;

class MJoltPhysicsEngine : public MObject, public IPhysicsEngineSubsystem
{
    DEFINE_OBJECT_SUBCLASS(MJoltPhysicsEngine)
public:
    MJoltPhysicsEngine() = default;
    ~MJoltPhysicsEngine() override = default;

    void init()    override;
    void cleanup() override;
    bool canTick() override { return true; };
    void tick(float fixedDeltaTime) override;

    void releaseAllBodies();

    IBoxCollisionBody*    createBoxCollider(const SBoxPhysicsBodySettings& settings) override;
    void                  releaseBoxCollider(IBoxCollisionBody* body)                 override;

    ISphereCollisionBody* createSphereCollisionBody(const SSphereBodySettings& settings) override;
    void                  releaseSphereCollisionBody(ISphereCollisionBody* body)          override;

    void registerCallbackReceiver(ICollisionBody* body,
                                  IPhysicsCallbackReceiver* receiver) override;
    void unregisterCallbackReceiver(ICollisionBody* body)             override;

    bool createPhysicsBody(const BodyCreationSettings& bodyCreationSetting,
                           EActivation activation, BodyID& container);

private:
    void dispatchPendingEvents();

private:
    TempAllocatorImpl   tempAlloc = TempAllocatorImpl(10 * 1024 * 1024);
    JobSystemThreadPool jobSystem = JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers,
                                                       std::thread::hardware_concurrency() - 1);
    PhysicsSystem  physicsSystem  = {};
    BodyInterface* bodyInterface  = nullptr;

    const uint maxPhysicsBodies      = 1024;
    const uint numBodyMutexes        = 0;
    const uint maxBodyPairs          = 1024;
    const uint maxContactConstraints = 1024;

    MBPLayerInterfaceImpl              broadPhaseLayerInterface        = {};
    MObjectVsBroadPhaseLayerFilterImpl objVsBroadPhaseLayerFilter      = {};
    MObjectLayerPairFilterImpl         objVsObjLayerFilter             = {};
    MBodyActivationListener            bodyActivationListener          = {};
    MContactListener                   contactListener                 = {};

    std::unordered_set<BodyID> bodies;

    // Maps Jolt body index → IPhysicsCallbackReceiver*.
    // Written on the main thread (register/unregister); read on Jolt worker
    // threads (contact listener lookups) — protected by a shared_mutex.
    std::unordered_map<uint32_t, IPhysicsCallbackReceiver*> receiverRegistry;
    std::shared_mutex                                        receiverRegistryMutex;
};

#endif //JOLT_PHYSICS_ENGINE_H