//
// Velocity — moves particles: position += velocity * dt.
//
// Every force module writes velocity; this one turns it into movement.
// Without it, particles stay where they spawned.
//
// `direction` sets the emission axis. Shapes are authored around the
// emitter's local +Y; at spawn this module rotates every new particle's
// position and velocity so that +Y lines up with `direction` — the whole
// shape turns with it (a cone points that way, a circle faces that way).
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_VELOCITY_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_VELOCITY_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"
#include "core/utils/field_engine_types.h"

class MParticlesVelocitySimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesVelocitySimulator)

    DECLARE_FIELD(direction, SVector3, SVector3(0.0f, 1.0f, 0.0f))   // emission axis, emitter local space — replaces +Y
    DECLARE_FIELD(drag,      float,    0.0f)                          // fraction of speed lost per second, 0 = none

public:
    // After every force has written velocity.
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Velocity; }

    void onSpawn(std::span<SParticleData> spawned, const SParticleSimContext& ctx) const override;
    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    // Rotation (emitter local space) that turns +Y onto `direction`.
    // Identity when direction is +Y or zero. Also used by the emitter's gizmo.
    [[nodiscard]] SQuaternion getEmissionRotation() const;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_VELOCITY_SIMULATOR_H
