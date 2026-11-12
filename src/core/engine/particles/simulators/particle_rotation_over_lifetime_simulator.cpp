//
// Rotation over lifetime — spins particles, speed per axis from a curve.
//

#include "particle_rotation_over_lifetime_simulator.h"

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesRotationOverLifetimeSimulator, "Rotation Over Lifetime")

void MParticlesRotationOverLifetimeSimulator::simulate(float dt, std::span<SParticleData> particles,
                                                       const SParticleSimContext& ctx) const
{
    const SCurve& cx = speedX.get();
    const SCurve& cy = speedY.get();
    const SCurve& cz = speedZ.get();

    // Per particle: 3 curve lookups + a quaternion build, multiply and
    // normalise (~40 flops) — compute-bound, so go parallel sooner than the
    // defaults.
    MParallel::forEach(particles, [&cx, &cy, &cz, dt](SParticleData& p) {
        const float t = p.normalizedAge();
        const SVector3 degreesPerSecond(cx.evaluate(t), cy.evaluate(t), cz.evaluate(t));
        if (degreesPerSecond == SVector3(0.0f)) return;

        // This frame's step as a small rotation. glm builds it from Euler
        // angles (radians). Multiplying on the right applies it around the
        // particle's own axes, not the world's.
        const SQuaternion step(glm::radians(degreesPerSecond) * dt);
        // Renormalise — tiny float errors would otherwise build up over
        // thousands of frames and start scaling the particle.
        p.rotation = glm::normalize(p.rotation * step);
    }, /*chunkSize*/ 256, /*threshold*/ 1024);
}

SParticleFieldMeta MParticlesRotationOverLifetimeSimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    meta.curveMin = -180.0f;
    meta.curveMax =  180.0f;
    if (fieldName == "speedX")
        meta.tooltip = "Spin around the particle's X axis, degrees / second over its life. Meshes only.";
    else if (fieldName == "speedY")
        meta.tooltip = "Spin around the particle's Y axis, degrees / second over its life. Meshes only.";
    else if (fieldName == "speedZ")
        meta.tooltip = "Spin around the particle's Z axis, degrees / second over its life.\n"
                       "For sprites this is the roll you see on screen.";
    return meta;
}
