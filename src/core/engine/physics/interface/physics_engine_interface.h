//
// physics_engine_interface.h
//

#ifndef PHYSICS_ENGINE_INTERFACE_H
#define PHYSICS_ENGINE_INTERFACE_H

#include "../data/box_body_settings.h"
#include "core/engine/physics/data/capsule_body_settings.h"
#include "core/engine/physics/data/convexhull_body_settings.h"
#include "core/engine/physics/data/cylinder_body_settings.h"
#include "core/engine/physics/data/mesh_body_settings.h"
#include "core/engine/physics/data/sphere_body_settings.h"
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

    // MeshShape is Static/Kinematic only — the factory emits a MWARN and forces
    // Static if DynamicBody is requested via settings.bodyType.
    virtual IMeshCollisionBody* createMeshCollisionBody(const SMeshBodySettings& settings) = 0;
    virtual void                releaseMeshCollisionBody(IMeshCollisionBody* body)         = 0;

    virtual IConvexHullCollisionBody* createConvexHullCollisionBody(const SConvexHullBodySettings& settings) = 0;
    virtual void                      releaseConvexHullCollisionBody(IConvexHullCollisionBody* body)          = 0;

    // ---- Callback receiver registry ----------------------------------------
    // Entities call these in onCreate / onExit. The engine maps the body's
    // internal index to the receiver so the contact listener can dispatch
    // events without knowing about spatial entities.
    virtual void registerCallbackReceiver(ICollisionBody* body,
                                          IPhysicsCallbackReceiver* receiver) = 0;
    virtual void unregisterCallbackReceiver(ICollisionBody* body)             = 0;
};

#endif //PHYSICS_ENGINE_INTERFACE_H