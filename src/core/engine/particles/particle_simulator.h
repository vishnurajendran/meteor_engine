//
// particle_simulator.h
//
// IParticleSimulator — one behaviour (gravity, size over lifetime, ...)
// attached to an emitter. The emitter calls every enabled module once per
// frame, in getSortingOrder() order, with all alive particles.
//
// -- Threading contract ------------------------------------------------------------
//   simulate() is called on the main thread. A module may split its own
//   work with MParallel::forEach — the lambda then runs on worker threads
//   and must only touch the particle it is given (plus read-only data:
//   the module's fields, ctx). simulate() is const so a module can't write
//   its own members by accident while workers run it.
//
//   onSpawn() is always serial.
//
// -- Ordering ------------------------------------------------------------------------
//   Forces write velocity, the velocity module integrates position, visual
//   modules read age. See EParticleModuleOrder.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_SIMULATOR_H
#define METEOR_ENGINE_PARTICLE_SIMULATOR_H

#include <span>

#include "particle_data.h"
#include "particle_settings.h"

namespace EParticleModuleOrder
{
    // 100-899: forces — write velocity
    inline constexpr int Gravity  = 100;
    inline constexpr int Noise    = 200;
    // integration — position += velocity * dt
    inline constexpr int Velocity = 1000;
    // 1100+: visuals — read age, write size / colour / rotation
    inline constexpr int Size     = 1100;
    inline constexpr int Color    = 1200;
    inline constexpr int Rotation = 1300;
}

class IParticleSimulator : public IParticleSettings
{
    DECLARE_FIELD(enabled, bool, true)

public:
    ~IParticleSimulator() override = default;

    [[nodiscard]] virtual int getSortingOrder() const = 0;

    // Once for each batch of newly spawned particles, before their first
    // simulate(). Set start values here.
    virtual void onSpawn(std::span<SParticleData> spawned, const SParticleSimContext& ctx) const {}

    // Every frame, with every alive particle.
    virtual void simulate(float dt, std::span<SParticleData> particles, const SParticleSimContext& ctx) const = 0;
};

#endif // METEOR_ENGINE_PARTICLE_SIMULATOR_H
