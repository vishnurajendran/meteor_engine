//
// Gravity — a constant world-space acceleration.
//

#include "particle_gravity_simulator.h"

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesGravitySimulator, "Gravity")

void MParticlesGravitySimulator::simulate(float dt, std::span<SParticleData> particles,
                                          const SParticleSimContext& ctx) const
{
    // Gravity is a world direction. In Local space "down" depends on how the
    // emitter is rotated, so turn it into the emitter's frame first. Once per
    // call — not per particle.
    const SVector3 dv = (ctx.worldToSimulation * gravity.get()) * dt;

    // One add per particle — memory-bound, so extra threads only pay off for
    // big pools. forEach stays serial below its threshold.
    MParallel::forEach(particles, [dv](SParticleData& p) {
        p.velocity += dv;
    });
}

SParticleFieldMeta MParticlesGravitySimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    meta.speed = 0.05f;
    if (fieldName == "gravity")
        meta.tooltip = "Acceleration in world space (units / s²). Earth is (0, -9.81, 0).\n"
                       "Needs the Velocity module to move particles.";
    return meta;
}
