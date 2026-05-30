//
// jolt_physics_engine.h
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
    void tick(float fixedDeltaTime) override;
    bool canTick() override { return allowTick; }

    void releaseAllBodies();

    // ---- Factory -----------------------------------------------------------

    IBoxCollisionBody*    createBoxCollider(const SBoxPhysicsBodySettings& settings) override;
    void                  releaseBoxCollider(IBoxCollisionBody* body)                 override;

    ISphereCollisionBody* createSphereCollisionBody(const SSphereBodySettings& settings) override;
    void                  releaseSphereCollisionBody(ISphereCollisionBody* body)          override;

    ICylinderCollisionBody* createCylinderCollisionBody(const SCylinderBodySettings& settings) override;
    void                    releaseCylinderCollisionBody(ICylinderCollisionBody* body)          override;

    ICapsuleCollisionBody* createCapsuleCollisionBody(const SCapsuleBodySettings& settings) override;
    void                   releaseCapsuleCollisionBody(ICapsuleCollisionBody* body)          override;

    IMeshCollisionBody* createMeshCollisionBody(const SMeshBodySettings& settings) override;
    void                releaseMeshCollisionBody(IMeshCollisionBody* body)         override;

    IConvexHullCollisionBody* createConvexHullCollisionBody(const SConvexHullBodySettings& settings) override;
    void                      releaseConvexHullCollisionBody(IConvexHullCollisionBody* body)          override;

    // ---- Registry ----------------------------------------------------------

    void registerCallbackReceiver(ICollisionBody* body,
                                  IPhysicsCallbackReceiver* receiver,
                                  unsigned int physicsLayer = 0) override;

    void unregisterCallbackReceiver(ICollisionBody* body) override;

    // ---- Raycast -----------------------------------------------------------

    bool rayCast(const SRay&         ray,
                 const SLayerFilter& layerFilter,
                 SRayCastHitResult&  outHit) override;

private:
    bool createPhysicsBody(const BodyCreationSettings& bodyCreationSetting, EActivation activation, BodyID& container);
    void dispatchPendingEvents();

private:
    TempAllocatorImpl*                                      tempAlloc                       = nullptr;
    JobSystemThreadPool*                                    jobSystem                       = nullptr;
    PhysicsSystem                                           physicsSystem                   = {};
    BodyInterface*                                          bodyInterface                   = nullptr;
    MBPLayerInterfaceImpl                                   broadPhaseLayerInterface        = {};
    MObjectVsBroadPhaseLayerFilterImpl                      objVsBroadPhaseLayerFilter      = {};
    MObjectLayerPairFilterImpl                              objVsObjLayerFilter             = {};
    MBodyActivationListener                                 bodyActivationListener          = {};
    MContactListener                                        contactListener                 = {};

    std::unordered_set<BodyID>                              bodies;

    // receiverRegistry maps bodyIndex -> callback receiver for contact events.
    // Accessed by Jolt worker threads (contact listener) and the main thread
    // (registration, raycast) -- protected by receiverRegistryMutex.
    std::unordered_map<uint32_t, IPhysicsCallbackReceiver*> receiverRegistry;

    // layerRegistry maps bodyIndex -> physicsLayer (0-31).
    // Updated alongside receiverRegistry; protected by the same mutex.
    std::unordered_map<uint32_t, unsigned int>              layerRegistry;

    std::shared_mutex                                       receiverRegistryMutex;
    bool                                                    allowTick                       = false;
};

#endif // JOLT_PHYSICS_ENGINE_H