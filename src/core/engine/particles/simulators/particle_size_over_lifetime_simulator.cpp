//
// Size over lifetime — size = startSize * curve(normalized age).
//

#include "particle_size_over_lifetime_simulator.h"

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesSizeOverLifetimeSimulator, "Size Over Lifetime")

void MParticlesSizeOverLifetimeSimulator::simulate(float dt, std::span<SParticleData> particles,
                                                   const SParticleSimContext& ctx) const
{
    // Reads startSize, writes size — never compounds frame to frame.
    // The curve is only read here, so sharing it across threads is safe.
    const SCurve& curve = sizeCurve.get();
    MParallel::forEach(particles, [&curve](SParticleData& p) {
        p.size = p.startSize * curve.evaluate(p.normalizedAge());
    });
}

SParticleFieldMeta MParticlesSizeOverLifetimeSimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    if (fieldName == "sizeCurve")
    {
        meta.curveMin = 0.0f;
        meta.curveMax = 1.0f;
        meta.tooltip  = "Multiplies the start size. Left edge = birth, right edge = death.";
    }
    return meta;
}
