//
// Created by ssj5v on 21-05-2026.
//

#ifndef PHYSICS_ENGINE_INTERFACE_H
#define PHYSICS_ENGINE_INTERFACE_H

#include "../data/box_body_settings.h"
#include "core/engine/physics/data/sphere_body_settings.h"
#include "core/engine/subsystem/subsystem_interface.h"


class IPhysicsCallbackReceiver;
class ISphereCollisionBody;
class IBoxCollisionBody;
class ICollisionBody;

class IPhysicsEngineSubsystem : public IEngineSubSystem
{
public:
    ~IPhysicsEngineSubsystem() override = default;

    virtual void init()    override = 0;
    virtual void cleanup() override = 0;
    virtual void tick(float fixedDeltaTime) = 0;

    // Factory
    virtual IBoxCollisionBody*    createBoxCollider(const SBoxPhysicsBodySettings& settings) = 0;
    virtual void                  releaseBoxCollider(IBoxCollisionBody* body)                 = 0;

    virtual ISphereCollisionBody* createSphereCollisionBody(const SSphereBodySettings& settings) = 0;
    virtual void                  releaseSphereCollisionBody(ISphereCollisionBody* body)          = 0;

    // Callback receiver registry — entities call these in onCreate / onExit.
    // The engine maps the body's internal index to the receiver so the contact
    // listener can dispatch events without knowing about spatial entities.
    virtual void registerCallbackReceiver(ICollisionBody* body,
                                          IPhysicsCallbackReceiver* receiver) = 0;
    virtual void unregisterCallbackReceiver(ICollisionBody* body)             = 0;
};

#endif //PHYSICS_ENGINE_INTERFACE_H