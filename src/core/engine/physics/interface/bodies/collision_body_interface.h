//
// Created by ssj5v on 21-05-2026.
//

#ifndef IPHYSICSBODY_H
#define IPHYSICSBODY_H
#include "../../data/default_body_settings.h"

class ICollisionBody
{
public:
    ICollisionBody()          = default;
    virtual ~ICollisionBody() = default;

    // Body configuration
    virtual void setBodyType(const ECollisionBodyType& type)     = 0;
    virtual void enableGravity(const bool& enable)               = 0;
    virtual void setGravity(const float& gravity)                = 0;
    virtual void setMass(const float& mass)                      = 0;
    virtual void setMovementConstraints(bool x, bool y, bool z)  = 0;
    virtual void setRotationConstraints(bool x, bool y, bool z)  = 0;
    virtual void setIsSensor(bool sensor)                        = 0;
    virtual void setLinearDamping(float drag)                    = 0;
    virtual void setAngularDamping(float drag)                   = 0;

    // Directly move the body to a world position and rotation.
    // For kinematic bodies this is the correct way to drive motion — Jolt computes
    // the implied velocity internally. For dynamic bodies this teleports the body
    // (no velocity is preserved), which should be used sparingly.
    virtual void setPositionAndRotation(const SVector3& position,
                                        const SQuaternion& rotation) = 0;

    // Validity and state queries
    [[nodiscard]] virtual bool        isValidBody()         const = 0;
    [[nodiscard]] virtual SVector3    getBodySyncPosition() const = 0;
    [[nodiscard]] virtual SQuaternion getBodySyncRotation() const = 0;
    [[nodiscard]] virtual SVector3    getLinearVelocity()   const = 0;
    [[nodiscard]] virtual SVector3    getAngularVelocity()  const = 0;
    [[nodiscard]] virtual SVector3    getCenterOfMass()     const = 0;

    virtual void physicsTick(const float& fixedDeltaTime) = 0;
};

#endif //IPHYSICSBODY_H