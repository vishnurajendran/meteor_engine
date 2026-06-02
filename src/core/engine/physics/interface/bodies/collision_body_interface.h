//
// collision_body_interface.h
//

#ifndef IPHYSICSBODY_H
#define IPHYSICSBODY_H
#include "../../data/default_body_settings.h"

// Determines how a force value is interpreted when passed to the apply* methods.
// The naming mirrors Unity's ForceMode so it stays intuitive for anyone familiar
// with standard game-engine conventions.
enum class EForceMode
{
    Force,         // Continuous force in Newtons - accumulated each physics step.
    Impulse,       // Instantaneous impulse - scaled by mass (J = kg * m/s).
    Acceleration,  // Continuous acceleration in m/s² - independent of body mass.
    VelocityChange // Instantaneous velocity delta in m/s - independent of body mass.
};

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

    [[nodiscard]] virtual float getLinearDamping()  const = 0;
    [[nodiscard]] virtual float getAngularDamping() const = 0;

    virtual void setLinearVelocity(const SVector3& linearVelocity) = 0;
    virtual void setAngularVelocity(const SVector3& velocity)      = 0;

    // Directly move the body to a world position and rotation.
    // For kinematic bodies this is the correct way to drive motion - Jolt computes
    // the implied velocity internally. For dynamic bodies this teleports the body
    // (no velocity is preserved), which should be used sparingly.
    virtual void setPositionAndRotation(const SVector3& position,
                                        const SQuaternion& rotation) = 0;

    // ---- Force application -------------------------------------------------

    // Apply a force, impulse, acceleration, or velocity change to the body's
    // centre of mass. Has no effect on Static bodies - a MWARN is emitted.
    virtual void applyForce(const SVector3& force, EForceMode mode) = 0;

    // Apply a force at a world-space point - generates both linear and angular
    // response. VelocityChange has no point-of-application variant in Jolt and
    // falls back to Impulse at the given point with a MWARN.
    virtual void applyForceAtPosition(const SVector3& force,
                                      const SVector3& worldPoint,
                                      EForceMode mode) = 0;

    // Apply a torque (or angular impulse / angular acceleration / angular velocity
    // delta depending on mode). Acceleration mode uses mass as a scalar inertia
    // approximation - accurate for uniform bodies, approximate otherwise.
    virtual void applyTorque(const SVector3& torque, EForceMode mode) = 0;

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