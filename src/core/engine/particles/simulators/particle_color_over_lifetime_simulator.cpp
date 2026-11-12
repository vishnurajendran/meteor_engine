//
// Color over lifetime — color = startColor * gradient(normalized age).
//

#include "particle_color_over_lifetime_simulator.h"

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesColorOverLifetimeSimulator, "Color Over Lifetime")

void MParticlesColorOverLifetimeSimulator::simulate(float dt, std::span<SParticleData> particles,
                                                    const SParticleSimContext& ctx) const
{
    const SGradient& gradient = colorGradient.get();
    MParallel::forEach(particles, [&gradient](SParticleData& p) {
        p.color = p.startColor * gradient.evaluate(p.normalizedAge());
    });
}

SParticleFieldMeta MParticlesColorOverLifetimeSimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    if (fieldName == "colorGradient")
        meta.tooltip = "Multiplies the start colour and alpha. Left edge = birth, right edge = death.";
    return meta;
}
