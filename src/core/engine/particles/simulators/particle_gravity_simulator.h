//
// Gravity — a constant world-space acceleration.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_GRAVITY_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_GRAVITY_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"
#include "core/utils/field_engine_types.h"

class MParticlesGravitySimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesGravitySimulator)

    DECLARE_FIELD(gravity, SVector3, SVector3(0.0f, -9.81f, 0.0f))   // world space, units / s²

public:
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Gravity; }

    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_GRAVITY_SIMULATOR_H
