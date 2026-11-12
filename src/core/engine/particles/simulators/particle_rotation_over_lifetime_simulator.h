//
// Rotation over lifetime — spins particles. Each axis has a curve giving
// the spin speed (degrees per second) at each point of the particle's life.
//
// Spin is around the particle's own axes and accumulates frame by frame.
// Sprites (billboards) only show Z — a roll in the screen plane. X / Y tilt
// the quad away from the camera, so it looks squashed, like a flipping card.
// Meshes use all three.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_ROTATION_OVER_LIFETIME_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_ROTATION_OVER_LIFETIME_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"
#include "core/utils/curve.h"

class MParticlesRotationOverLifetimeSimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesRotationOverLifetimeSimulator)

    // Degrees per second around each of the particle's local axes.
    DECLARE_FIELD(speedX, SCurve, SCurve::constant(0.0f))
    DECLARE_FIELD(speedY, SCurve, SCurve::constant(0.0f))
    DECLARE_FIELD(speedZ, SCurve, SCurve::constant(90.0f))

public:
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Rotation; }

    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_ROTATION_OVER_LIFETIME_SIMULATOR_H
