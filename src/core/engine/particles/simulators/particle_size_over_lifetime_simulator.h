//
// Size over lifetime — size = startSize * curve(normalized age).
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_SIZE_OVER_LIFETIME_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_SIZE_OVER_LIFETIME_SIMULATOR_H

#include "core/engine/particles/particle_simulator.h"
#include "core/object/object.h"
#include "core/utils/curve.h"

class MParticlesSizeOverLifetimeSimulator : public MObject, public IParticleSimulator
{
    DEFINE_OBJECT_SUBCLASS(MParticlesSizeOverLifetimeSimulator)

    DECLARE_FIELD(sizeCurve, SCurve, SCurve::linear(1.0f, 0.0f))   // multiplier on startSize — shrinks to nothing by default

public:
    [[nodiscard]] int getSortingOrder() const override { return EParticleModuleOrder::Size; }

    void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const override;

    [[nodiscard]] SParticleFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

#endif // METEOR_ENGINE_PARTICLE_SIZE_OVER_LIFETIME_SIMULATOR_H
