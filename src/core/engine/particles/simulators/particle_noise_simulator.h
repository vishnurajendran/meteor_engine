//
// Noise — pushes particles around with a smooth, swirling 3D noise field.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_NOISE_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_NOISE_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"

class MParticlesNoiseSimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesNoiseSimulator)

    DECLARE_FIELD(strength,    float, 1.0f)   // acceleration, units / s²
    DECLARE_FIELD(frequency,   float, 0.5f)   // how tight the swirls are — higher = smaller features
    DECLARE_FIELD(scrollSpeed, float, 0.5f)   // how fast the field changes over time

public:
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Noise; }

    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_NOISE_SIMULATOR_H
