//
// Created by ssj5v on 22-05-2026.
//

#ifndef JOLT_BASE_COLLISION_BODY_H
#define JOLT_BASE_COLLISION_BODY_H
#include <Jolt/Jolt.h>
#include "Jolt/Physics/Body/BodyID.h"
#include "core/engine/physics/interface/bodies/collision_body_interface.h"

namespace JPH
{
    class BodyInterface;
    class PhysicsSystem;
}

class MJoltBaseCollisionBody : public virtual ICollisionBody
{
public:
    MJoltBaseCollisionBody(JPH::BodyInterface& inBodyInterface,
                           JPH::PhysicsSystem& inPhysicsScene,
                           JPH::BodyID         inBodyId);

    ~MJoltBaseCollisionBody() override = default;

    [[nodiscard]] bool        isValidBody()         const override { return !joltBodyID.IsInvalid(); }
    [[nodiscard]] SVector3    getBodySyncPosition() const override { return internalPosition; }
    [[nodiscard]] SQuaternion getBodySyncRotation() const override { return internalRotation; }
    [[nodiscard]] SVector3    getLinearVelocity()   const override { return internalLinearVelocity; }
    [[nodiscard]] SVector3    getAngularVelocity()  const override { return internalAngularVelocity; }
    [[nodiscard]] SVector3    getCenterOfMass()     const override;

    void physicsTick(const float& fixedDeltaTime) override;

    void setBodyType(const ECollisionBodyType& type)    override;
    void enableGravity(const bool& enable)              override;
    void setGravity(const float& gravity)               override;
    void setMass(const float& mass)                     override;
    void setMovementConstraints(bool x, bool y, bool z) override;
    void setRotationConstraints(bool x, bool y, bool z) override;
    void setIsSensor(bool sensor)                       override;
    void setLinearDamping(float drag)                   override;
    void setAngularDamping(float drag)                  override;
    void setPositionAndRotation(const SVector3& position,
                                const SQuaternion& rotation) override;

    // Exposes the internal Jolt body index so MJoltPhysicsEngine can key its
    // receiver registry. The index is stable for the body's lifetime.
    [[nodiscard]] uint32_t getJoltBodyIndex() const { return joltBodyID.GetIndex(); }
    [[nodiscard]] JPH::BodyID getBodyID() const { return joltBodyID; }

protected:
    void syncPositionAndRotation();

    JPH::BodyInterface& joltBodyInterface;
    JPH::PhysicsSystem& joltPhysicsSystem;
    JPH::BodyID         joltBodyID;

    SVector3    internalPosition        = {};
    SQuaternion internalRotation        = {};
    SVector3    internalLinearVelocity  = {};
    SVector3    internalAngularVelocity = {};

    bool moveAllowed[3]   = { true, true, true };
    bool rotateAllowed[3] = { true, true, true };
};

#endif //JOLT_BASE_COLLISION_BODY_H