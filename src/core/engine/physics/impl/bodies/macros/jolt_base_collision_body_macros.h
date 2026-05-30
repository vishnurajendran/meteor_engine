#ifndef JOLT_BASE_COLLISION_MACROS_H
#define JOLT_BASE_COLLISION_MACROS_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include "core/engine/physics/interface/bodies/collision_body_interface.h"

namespace JPH {
    class BodyInterface;
    class PhysicsSystem;
}

#define DECLARE_BASE_COLLISION_BODY() \
public: \
    [[nodiscard]] bool        isValidBody()         const override { return !joltBodyID.IsInvalid(); } \
    [[nodiscard]] SVector3    getBodySyncPosition() const override { return internalPosition; } \
    [[nodiscard]] SQuaternion getBodySyncRotation() const override { return internalRotation; } \
    [[nodiscard]] SVector3    getLinearVelocity()   const override { return internalLinearVelocity; } \
    [[nodiscard]] SVector3    getAngularVelocity()  const override { return internalAngularVelocity; } \
    [[nodiscard]] SVector3    getCenterOfMass()     const override; \
    [[nodiscard]] float       getLinearDamping()    const override; \
    [[nodiscard]] float       getAngularDamping()   const override; \
\
    void physicsTick(const float& fixedDeltaTime) override; \
\
    void setBodyType(const ECollisionBodyType& type)    override; \
    void enableGravity(const bool& enable)              override; \
    void setGravity(const float& gravity)               override; \
    void setMass(const float& mass)                     override; \
    void setMovementConstraints(bool x, bool y, bool z) override; \
    void setRotationConstraints(bool x, bool y, bool z) override; \
    void setIsSensor(bool sensor)                       override; \
    void setLinearDamping(float drag)                   override; \
    void setAngularDamping(float drag)                  override; \
    void setLinearVelocity(const SVector3& linearVelocity)  override; \
    void setAngularVelocity(const SVector3& angularVelocity) override; \
    void setPositionAndRotation(const SVector3& position, \
                                const SQuaternion& rotation) override; \
\
    void applyForce(const SVector3& force, EForceMode mode) override; \
    void applyForceAtPosition(const SVector3& force, const SVector3& worldPoint, EForceMode mode) override; \
    void applyTorque(const SVector3& torque, EForceMode mode) override; \
\
    [[nodiscard]] uint32_t getJoltBodyIndex() const override { return joltBodyID.GetIndex(); } \
    [[nodiscard]] JPH::BodyID getBodyID() const override { return joltBodyID; } \
\
protected: \
    void syncPositionAndRotation(); \
\
    JPH::BodyInterface& joltBodyInterface; \
    JPH::PhysicsSystem& joltPhysicsSystem; \
    JPH::BodyID         joltBodyID; \
\
    SVector3    internalPosition        = {}; \
    SQuaternion internalRotation        = {}; \
    SVector3    internalLinearVelocity  = {}; \
    SVector3    internalAngularVelocity = {}; \
\
    bool moveAllowed[3]   = { true, true, true }; \
    bool rotateAllowed[3] = { true, true, true };

#define INIT_BASE_COLLISION_BODY(bodyInterface, physicsSystem, bodyId) \
    joltBodyInterface(bodyInterface), \
    joltPhysicsSystem(physicsSystem), \
    joltBodyID(bodyId)

#define IMPLEMENT_BASE_COLLISION_BODY(ClassName) \
void ClassName::syncPositionAndRotation() \
{ \
    if (!isValidBody()) \
        return; \
    JPH::RVec3 jphPosition; \
    JPH::RVec3 jphLinearVelocity; \
    JPH::RVec3 jphAngularVelocity; \
    JPH::Quat  jphRotation; \
    joltBodyInterface.GetPositionAndRotation(joltBodyID, jphPosition, jphRotation); \
    joltBodyInterface.GetLinearAndAngularVelocity(joltBodyID, jphLinearVelocity, jphAngularVelocity); \
    internalPosition        = { jphPosition.GetX(),        jphPosition.GetY(),        jphPosition.GetZ() }; \
    internalRotation        = { jphRotation.GetW(),         jphRotation.GetX(),         jphRotation.GetY(),         jphRotation.GetZ() }; \
    internalLinearVelocity  = { jphLinearVelocity.GetX(),  jphLinearVelocity.GetY(),  jphLinearVelocity.GetZ() }; \
    internalAngularVelocity = { jphAngularVelocity.GetX(), jphAngularVelocity.GetY(), jphAngularVelocity.GetZ() }; \
} \
void ClassName::physicsTick(const float& fixedDeltaTime) \
{ \
    if (!isValidBody()) \
        return; \
    syncPositionAndRotation(); \
    const bool hasMoveLock   = !moveAllowed[0]   || !moveAllowed[1]   || !moveAllowed[2]; \
    const bool hasRotateLock = !rotateAllowed[0] || !rotateAllowed[1] || !rotateAllowed[2]; \
    if (!(hasMoveLock || hasRotateLock)) \
        return; \
    if (!joltBodyInterface.IsActive(joltBodyID)) \
        return; \
    const JPH::Vec3 linVel = joltBodyInterface.GetLinearVelocity(joltBodyID); \
    const JPH::Vec3 angVel = joltBodyInterface.GetAngularVelocity(joltBodyID); \
    const JPH::Vec3 clampedLin( \
        moveAllowed[0] ? linVel.GetX() : 0.0f, \
        moveAllowed[1] ? linVel.GetY() : 0.0f, \
        moveAllowed[2] ? linVel.GetZ() : 0.0f \
    ); \
    const JPH::Vec3 clampedAng( \
        rotateAllowed[0] ? angVel.GetX() : 0.0f, \
        rotateAllowed[1] ? angVel.GetY() : 0.0f, \
        rotateAllowed[2] ? angVel.GetZ() : 0.0f \
    ); \
    joltBodyInterface.SetLinearAndAngularVelocity(joltBodyID, clampedLin, clampedAng); \
} \
void ClassName::setBodyType(const ECollisionBodyType& type) \
{ \
    if (!isValidBody()) { \
        MERROR(#ClassName "::setBodyType - called on an invalid body"); \
        return; \
    } \
    JPH::EMotionType motionType; \
    switch (type) { \
    case ECollisionBodyType::StaticBody:    motionType = JPH::EMotionType::Static;    break; \
    case ECollisionBodyType::DynamicBody:   motionType = JPH::EMotionType::Dynamic;   break; \
    case ECollisionBodyType::KinematicBody: motionType = JPH::EMotionType::Kinematic; break; \
    default:                                motionType = JPH::EMotionType::Static;    break; \
    } \
    joltBodyInterface.SetMotionType(joltBodyID, motionType, JPH::EActivation::Activate); \
} \
void ClassName::enableGravity(const bool& enable) \
{ \
    if (!isValidBody()) { \
        return; \
    } \
    joltBodyInterface.SetGravityFactor(joltBodyID, enable ? 1.0f : 0.0f); \
} \
void ClassName::setGravity(const float& gravity) \
{ \
    if (!isValidBody()) { \
        return; \
    } \
    joltBodyInterface.SetGravityFactor(joltBodyID, gravity); \
} \
void ClassName::setMass(const float& mass) \
{ \
    if (!isValidBody()) { \
        return; \
    } \
    if (mass <= 0.0f) { \
        return; \
    } \
    JPH::BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) { \
        return; \
    } \
    JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
    if (!mp) { \
        return; \
    } \
    mp->SetInverseMass(1.0f / mass); \
} \
void ClassName::setMovementConstraints(bool x, bool y, bool z) \
{ \
    moveAllowed[0] = x; \
    moveAllowed[1] = y; \
    moveAllowed[2] = z; \
} \
void ClassName::setRotationConstraints(bool x, bool y, bool z) \
{ \
    rotateAllowed[0] = x; \
    rotateAllowed[1] = y; \
    rotateAllowed[2] = z; \
} \
void ClassName::setIsSensor(const bool sensor) \
{ \
    if (!isValidBody()) { \
        MERROR(#ClassName "::setIsSensor - called on an invalid body"); \
        return; \
    } \
    JPH::BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) { \
        MERROR(#ClassName "::setIsSensor - failed to acquire write lock"); \
        return; \
    } \
    lock.GetBody().SetIsSensor(sensor); \
} \
void ClassName::setLinearDamping(const float drag) \
{ \
    if (!isValidBody()) { \
        MERROR(#ClassName "::setLinearDamping - called on an invalid body"); \
        return; \
    } \
    if (drag < 0.0f) { \
        return; \
    } \
    JPH::BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) { \
        return; \
    } \
    JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
    if (!mp) { \
        return; \
    } \
    mp->SetLinearDamping(drag); \
} \
float ClassName::getLinearDamping() const \
{ \
    if (!isValidBody()) return 0.0f; \
    JPH::BodyLockRead lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) return 0.0f; \
    const JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
    if (!mp) return 0.0f; \
    return mp->GetLinearDamping(); \
} \
float ClassName::getAngularDamping() const \
{ \
    if (!isValidBody()) return 0.0f; \
    JPH::BodyLockRead lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) return 0.0f; \
    const JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
    if (!mp) return 0.0f; \
    return mp->GetAngularDamping(); \
} \
void ClassName::setAngularDamping(const float drag) \
{ \
    if (!isValidBody()) { \
        MERROR(#ClassName "::setAngularDamping - called on an invalid body"); \
        return; \
    } \
    if (drag < 0.0f) { \
        return; \
    } \
    JPH::BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
    if (!lock.Succeeded()) { \
        return; \
    } \
    JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
    if (!mp) { \
        return; \
    } \
    mp->SetAngularDamping(drag); \
} \
void ClassName::setLinearVelocity(const SVector3& linearVelocity) \
{ \
    if (!isValidBody()) return; \
    const JPH::Vec3 vel(linearVelocity.x, linearVelocity.y, linearVelocity.z); \
    joltBodyInterface.SetLinearVelocity(joltBodyID, vel); \
} \
void ClassName::setAngularVelocity(const SVector3& angularVelocity) \
{ \
    if (!isValidBody()) return; \
    const JPH::Vec3 vel(angularVelocity.x, angularVelocity.y, angularVelocity.z); \
    joltBodyInterface.SetAngularVelocity(joltBodyID, vel); \
} \
void ClassName::setPositionAndRotation(const SVector3& position, const SQuaternion& rotation) \
{ \
    if (!isValidBody()) { \
        MERROR(#ClassName "::setPositionAndRotation - called on an invalid body"); \
        return; \
    } \
    const JPH::RVec3 jphPos(position.x, position.y, position.z); \
    const JPH::Quat  jphRot(rotation.x, rotation.y, rotation.z, rotation.w); \
    joltBodyInterface.SetPositionAndRotation(joltBodyID, jphPos, jphRot, JPH::EActivation::Activate); \
} \
SVector3 ClassName::getCenterOfMass() const \
{ \
    if (!isValidBody()) return {}; \
    const JPH::RVec3 com = joltBodyInterface.GetCenterOfMassPosition(joltBodyID); \
    return { com.GetX(), com.GetY(), com.GetZ() }; \
} \
void ClassName::applyForce(const SVector3& force, EForceMode mode) \
{ \
    if (!isValidBody()) return; \
    if (joltBodyInterface.GetMotionType(joltBodyID) == JPH::EMotionType::Static) \
    { \
        MWARN(SString::format("{0}::applyForce — called on a Static body; force has no effect", #ClassName)); \
        return; \
    } \
    const JPH::Vec3 jphForce(force.x, force.y, force.z); \
    switch (mode) \
    { \
    case EForceMode::Force: \
        joltBodyInterface.AddForce(joltBodyID, jphForce); \
        break; \
    case EForceMode::Impulse: \
        joltBodyInterface.AddImpulse(joltBodyID, jphForce); \
        break; \
    case EForceMode::Acceleration: \
    { \
        float invMass = 0.0f; \
        { \
            JPH::BodyLockRead lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
            if (!lock.Succeeded()) return; \
            const JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
            if (!mp) return; \
            invMass = mp->GetInverseMass(); \
        } \
        if (invMass <= 0.0f) return; \
        joltBodyInterface.AddForce(joltBodyID, jphForce * (1.0f / invMass)); \
        break; \
    } \
    case EForceMode::VelocityChange: \
        joltBodyInterface.AddLinearVelocity(joltBodyID, jphForce); \
        break; \
    } \
} \
void ClassName::applyForceAtPosition(const SVector3& force, const SVector3& worldPoint, EForceMode mode) \
{ \
    if (!isValidBody()) return; \
    if (joltBodyInterface.GetMotionType(joltBodyID) == JPH::EMotionType::Static) \
    { \
        MWARN(SString::format("{0}::applyForceAtPosition — called on a Static body; force has no effect", #ClassName)); \
        return; \
    } \
    const JPH::Vec3  jphForce(force.x, force.y, force.z); \
    const JPH::RVec3 jphPoint(worldPoint.x, worldPoint.y, worldPoint.z); \
    switch (mode) \
    { \
    case EForceMode::Force: \
        joltBodyInterface.AddForce(joltBodyID, jphForce, jphPoint); \
        break; \
    case EForceMode::Impulse: \
        joltBodyInterface.AddImpulse(joltBodyID, jphForce, jphPoint); \
        break; \
    case EForceMode::Acceleration: \
    { \
        float invMass = 0.0f; \
        { \
            JPH::BodyLockRead lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
            if (!lock.Succeeded()) return; \
            const JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
            if (!mp) return; \
            invMass = mp->GetInverseMass(); \
        } \
        if (invMass <= 0.0f) return; \
        joltBodyInterface.AddForce(joltBodyID, jphForce * (1.0f / invMass), jphPoint); \
        break; \
    } \
    case EForceMode::VelocityChange: \
        MWARN(SString::format("{0}::applyForceAtPosition — VelocityChange has no point-of-application variant in Jolt; falling back to Impulse at the given point", #ClassName)); \
        joltBodyInterface.AddImpulse(joltBodyID, jphForce, jphPoint); \
        break; \
    } \
} \
void ClassName::applyTorque(const SVector3& torque, EForceMode mode) \
{ \
    if (!isValidBody()) return; \
    if (joltBodyInterface.GetMotionType(joltBodyID) == JPH::EMotionType::Static) \
    { \
        MWARN(SString::format("{0}::applyTorque — called on a Static body; torque has no effect", #ClassName)); \
        return; \
    } \
    const JPH::Vec3 jphTorque(torque.x, torque.y, torque.z); \
    switch (mode) \
    { \
    case EForceMode::Force: \
        joltBodyInterface.AddTorque(joltBodyID, jphTorque); \
        break; \
    case EForceMode::Impulse: \
        joltBodyInterface.AddAngularImpulse(joltBodyID, jphTorque); \
        break; \
    case EForceMode::Acceleration: \
    { \
        float invMass = 0.0f; \
        { \
            JPH::BodyLockRead lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID); \
            if (!lock.Succeeded()) return; \
            const JPH::MotionProperties* mp = lock.GetBody().GetMotionPropertiesUnchecked(); \
            if (!mp) return; \
            invMass = mp->GetInverseMass(); \
        } \
        if (invMass <= 0.0f) return; \
        joltBodyInterface.AddTorque(joltBodyID, jphTorque * (1.0f / invMass)); \
        break; \
    } \
    case EForceMode::VelocityChange: \
    { \
        const JPH::Vec3 current = joltBodyInterface.GetAngularVelocity(joltBodyID); \
        joltBodyInterface.SetAngularVelocity(joltBodyID, current + jphTorque); \
        break; \
    } \
    } \
}

#endif // JOLT_BASE_COLLISION_MACROS_H