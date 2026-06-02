//
// physics_engine_interface.h
//

#ifndef PHYSICS_ENGINE_INTERFACE_H
#define PHYSICS_ENGINE_INTERFACE_H

#include "../data/box_body_settings.h"
#include "core/engine/physics/data/sphere_body_settings.h"
#include "core/engine/physics/data/cylinder_body_settings.h"
#include "core/engine/physics/data/capsule_body_settings.h"
#include "core/engine/physics/data/mesh_body_settings.h"
#include "core/engine/physics/data/convexhull_body_settings.h"
#include "core/engine/physics/data/raycast_data.h"
#include "core/engine/physics/data/layer_filter.h"
#include "core/engine/subsystem/subsystem_interface.h"


class IPhysicsCallbackReceiver;
class ISphereCollisionBody;
class IBoxCollisionBody;
class ICylinderCollisionBody;
class ICapsuleCollisionBody;
class IMeshCollisionBody;
class IConvexHullCollisionBody;
class ICollisionBody;

class IPhysicsEngineSubsystem : public IEngineSubSystem
{
public:
    ~IPhysicsEngineSubsystem() override = default;

    virtual void init()    override = 0;
    virtual void cleanup() override = 0;
    virtual void tick(float fixedDeltaTime) = 0;

    // ---- Factory -----------------------------------------------------------

    virtual IBoxCollisionBody*    createBoxCollider(const SBoxPhysicsBodySettings& settings) = 0;
    virtual void                  releaseBoxCollider(IBoxCollisionBody* body)                 = 0;

    virtual ISphereCollisionBody* createSphereCollisionBody(const SSphereBodySettings& settings) = 0;
    virtual void                  releaseSphereCollisionBody(ISphereCollisionBody* body)          = 0;

    virtual ICylinderCollisionBody* createCylinderCollisionBody(const SCylinderBodySettings& settings) = 0;
    virtual void                    releaseCylinderCollisionBody(ICylinderCollisionBody* body)          = 0;

    virtual ICapsuleCollisionBody* createCapsuleCollisionBody(const SCapsuleBodySettings& settings) = 0;
    virtual void                   releaseCapsuleCollisionBody(ICapsuleCollisionBody* body)          = 0;

    // MeshShape is Static/Kinematic only - the factory emits a MWARN and forces
    // Static if DynamicBody is requested via settings.bodyType.
    virtual IMeshCollisionBody* createMeshCollisionBody(const SMeshBodySettings& settings) = 0;
    virtual void                releaseMeshCollisionBody(IMeshCollisionBody* body)         = 0;

    virtual IConvexHullCollisionBody* createConvexHullCollisionBody(const SConvexHullBodySettings& settings) = 0;
    virtual void                      releaseConvexHullCollisionBody(IConvexHullCollisionBody* body)          = 0;

    // ---- Callback receiver registry ----------------------------------------

    // physicsLayer is the 0–31 layer index this body belongs to. It is stored
    // in the engine's layer registry so rayCast can filter hits by layer without
    // needing to touch Jolt's internal object layer system.
    virtual void registerCallbackReceiver(ICollisionBody* body,
                                          IPhysicsCallbackReceiver* receiver,
                                          unsigned int physicsLayer = 0) = 0;

    virtual void unregisterCallbackReceiver(ICollisionBody* body) = 0;

    // ---- Raycast -----------------------------------------------------------

    // Casts a ray through the physics world and populates `outHit` with the
    // closest hit that belongs to a layer included in `layerFilter`.
    //
    // Returns true if any layer-matching body was hit; false otherwise.
    // outHit is only valid when the return value is true.
    //
    // Threading: must be called from the main thread. Do not call during
    // physicsSystem.Update() (i.e. not from inside a Jolt contact callback).
    virtual bool rayCast(const SRay&         ray,
                         const SLayerFilter& layerFilter,
                         SRayCastHitResult&  outHit) = 0;
};

#endif // PHYSICS_ENGINE_INTERFACE_H