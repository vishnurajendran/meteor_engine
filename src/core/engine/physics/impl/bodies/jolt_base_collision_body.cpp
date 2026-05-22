//
// Created by ssj5v on 22-05-2026.
//

#include "jolt_base_collision_body.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Body/MotionProperties.h>

#include "core/utils/logger.h"

using namespace JPH;

MJoltBaseCollisionBody::MJoltBaseCollisionBody(BodyInterface& inBodyInterface,
                                               PhysicsSystem& inPhysicsScene,
                                               BodyID         inBodyId)
    : joltBodyInterface(inBodyInterface)
    , joltPhysicsSystem(inPhysicsScene)
    , joltBodyID(inBodyId)
{
}

// ---------------------------------------------------------------------------
// Internal sync
// ---------------------------------------------------------------------------

void MJoltBaseCollisionBody::syncPositionAndRotation()
{
    if (!isValidBody())
        return;

    RVec3 jphPosition;
    RVec3 jphLinearVelocity;
    RVec3 jphAngularVelocity;
    Quat  jphRotation;

    joltBodyInterface.GetPositionAndRotation(joltBodyID, jphPosition, jphRotation);
    joltBodyInterface.GetLinearAndAngularVelocity(joltBodyID, jphLinearVelocity, jphAngularVelocity);

    internalPosition        = { jphPosition.GetX(),        jphPosition.GetY(),        jphPosition.GetZ() };
    internalRotation        = { jphRotation.GetW(),         jphRotation.GetX(),         jphRotation.GetY(),         jphRotation.GetZ() };
    internalLinearVelocity  = { jphLinearVelocity.GetX(),  jphLinearVelocity.GetY(),  jphLinearVelocity.GetZ() };
    internalAngularVelocity = { jphAngularVelocity.GetX(), jphAngularVelocity.GetY(), jphAngularVelocity.GetZ() };
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void MJoltBaseCollisionBody::physicsTick(const float& fixedDeltaTime)
{
    if (!isValidBody())
        return;

    syncPositionAndRotation();

    const bool hasMoveLock   = !moveAllowed[0]   || !moveAllowed[1]   || !moveAllowed[2];
    const bool hasRotateLock = !rotateAllowed[0] || !rotateAllowed[1] || !rotateAllowed[2];

    if (!(hasMoveLock || hasRotateLock))
        return;

    if (!joltBodyInterface.IsActive(joltBodyID))
        return;

    const Vec3 linVel = joltBodyInterface.GetLinearVelocity(joltBodyID);
    const Vec3 angVel = joltBodyInterface.GetAngularVelocity(joltBodyID);

    const Vec3 clampedLin(
        moveAllowed[0] ? linVel.GetX() : 0.0f,
        moveAllowed[1] ? linVel.GetY() : 0.0f,
        moveAllowed[2] ? linVel.GetZ() : 0.0f
    );
    const Vec3 clampedAng(
        rotateAllowed[0] ? angVel.GetX() : 0.0f,
        rotateAllowed[1] ? angVel.GetY() : 0.0f,
        rotateAllowed[2] ? angVel.GetZ() : 0.0f
    );

    joltBodyInterface.SetLinearAndAngularVelocity(joltBodyID, clampedLin, clampedAng);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void MJoltBaseCollisionBody::setBodyType(const ECollisionBodyType& type)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setBodyType — called on an invalid body");
        return;
    }

    EMotionType motionType;
    switch (type)
    {
    case ECollisionBodyType::StaticBody:    motionType = EMotionType::Static;    break;
    case ECollisionBodyType::DynamicBody:   motionType = EMotionType::Dynamic;   break;
    case ECollisionBodyType::KinematicBody: motionType = EMotionType::Kinematic; break;
    default:                                motionType = EMotionType::Static;    break;
    }

    joltBodyInterface.SetMotionType(joltBodyID, motionType, EActivation::Activate);
}

void MJoltBaseCollisionBody::enableGravity(const bool& enable)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::enableGravity — called on an invalid body");
        return;
    }

    joltBodyInterface.SetGravityFactor(joltBodyID, enable ? 1.0f : 0.0f);
}

void MJoltBaseCollisionBody::setGravity(const float& gravity)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setGravity — called on an invalid body");
        return;
    }

    joltBodyInterface.SetGravityFactor(joltBodyID, gravity);
}

void MJoltBaseCollisionBody::setMass(const float& mass)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setMass — called on an invalid body");
        return;
    }

    if (mass <= 0.0f)
    {
        MERROR("MJoltBaseCollisionBody::setMass — mass must be positive");
        return;
    }

    BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID);
    if (!lock.Succeeded())
    {
        MERROR("MJoltBaseCollisionBody::setMass — failed to acquire write lock");
        return;
    }

    MotionProperties* mp = lock.GetBody().GetMotionProperties();
    if (!mp)
    {
        MERROR("MJoltBaseCollisionBody::setMass — body has no MotionProperties (is it static?)");
        return;
    }

    mp->SetInverseMass(1.0f / mass);
}

void MJoltBaseCollisionBody::setMovementConstraints(bool x, bool y, bool z)
{
    moveAllowed[0] = x;
    moveAllowed[1] = y;
    moveAllowed[2] = z;
}

void MJoltBaseCollisionBody::setRotationConstraints(bool x, bool y, bool z)
{
    rotateAllowed[0] = x;
    rotateAllowed[1] = y;
    rotateAllowed[2] = z;
}

void MJoltBaseCollisionBody::setIsSensor(const bool sensor)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setIsSensor — called on an invalid body");
        return;
    }

    BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID);
    if (!lock.Succeeded())
    {
        MERROR("MJoltBaseCollisionBody::setIsSensor — failed to acquire write lock");
        return;
    }

    lock.GetBody().SetIsSensor(sensor);
}

void MJoltBaseCollisionBody::setLinearDamping(const float drag)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setLinearDamping — called on an invalid body");
        return;
    }

    if (drag < 0.0f)
    {
        MERROR("MJoltBaseCollisionBody::setLinearDamping — damping must be >= 0");
        return;
    }

    BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID);
    if (!lock.Succeeded())
    {
        MERROR("MJoltBaseCollisionBody::setLinearDamping — failed to acquire write lock");
        return;
    }

    MotionProperties* mp = lock.GetBody().GetMotionProperties();
    if (!mp)
    {
        MERROR("MJoltBaseCollisionBody::setLinearDamping — body has no MotionProperties (is it static?)");
        return;
    }

    mp->SetLinearDamping(drag);
}

void MJoltBaseCollisionBody::setAngularDamping(const float drag)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setAngularDamping — called on an invalid body");
        return;
    }

    if (drag < 0.0f)
    {
        MERROR("MJoltBaseCollisionBody::setAngularDamping — damping must be >= 0");
        return;
    }

    BodyLockWrite lock(joltPhysicsSystem.GetBodyLockInterface(), joltBodyID);
    if (!lock.Succeeded())
    {
        MERROR("MJoltBaseCollisionBody::setAngularDamping — failed to acquire write lock");
        return;
    }

    MotionProperties* mp = lock.GetBody().GetMotionProperties();
    if (!mp)
    {
        MERROR("MJoltBaseCollisionBody::setAngularDamping — body has no MotionProperties (is it static?)");
        return;
    }

    mp->SetAngularDamping(drag);
}

void MJoltBaseCollisionBody::setPositionAndRotation(const SVector3& position,
                                                    const SQuaternion& rotation)
{
    if (!isValidBody())
    {
        MERROR("MJoltBaseCollisionBody::setPositionAndRotation — called on an invalid body");
        return;
    }

    // SQuaternion packs as {w, x, y, z} — match to JPH::Quat(x, y, z, w).
    const RVec3 jphPos(position.x, position.y, position.z);
    const Quat  jphRot(rotation.x, rotation.y, rotation.z, rotation.w);

    // EActivation::Activate wakes a sleeping kinematic body so it updates its
    // broadphase entry. For large scenes with many sleeping statics, pass
    // EActivation::DontActivate if you are moving a body that was just created
    // and hasn't slept yet.
    joltBodyInterface.SetPositionAndRotation(joltBodyID, jphPos, jphRot, EActivation::Activate);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

SVector3 MJoltBaseCollisionBody::getCenterOfMass() const
{
    if (!isValidBody())
        return {};

    const RVec3 com = joltBodyInterface.GetCenterOfMassPosition(joltBodyID);
    return { com.GetX(), com.GetY(), com.GetZ() };
}