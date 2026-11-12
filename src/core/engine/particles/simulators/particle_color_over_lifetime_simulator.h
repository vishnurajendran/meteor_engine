//
// Color over lifetime — color = startColor * gradient(normalized age).
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_COLOR_OVER_LIFETIME_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_COLOR_OVER_LIFETIME_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"
#include "core/utils/curve.h"

class MParticlesColorOverLifetimeSimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesColorOverLifetimeSimulator)

    // Multiplier on startColor — fades out by default.
    DECLARE_FIELD(colorGradient, SGradient, SGradient::linear(SVector4(1.0f), SVector4(1.0f, 1.0f, 1.0f, 0.0f)))

public:
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Color; }

    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_COLOR_OVER_LIFETIME_SIMULATOR_H
