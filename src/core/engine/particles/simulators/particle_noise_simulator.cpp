//
// Noise — pushes particles around with a smooth, swirling 3D noise field.
//

#include "particle_noise_simulator.h"

#include <cmath>

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesNoiseSimulator, "Noise")

namespace
{
    uint32_t hashCell(int x, int y, int z)
    {
        uint32_t h = static_cast<uint32_t>(x) * 0x8da6b343u
                   ^ static_cast<uint32_t>(y) * 0xd8163841u
                   ^ static_cast<uint32_t>(z) * 0xcb1ab31fu;
        h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        return h;
    }

    // Dot of the cell corner's pseudo-random gradient with the offset to it.
    // 12 gradients — the edge midpoints of a cube (classic Perlin set).
    float gradientDot(uint32_t hash, float x, float y, float z)
    {
        switch (hash % 12u)
        {
            case 0:  return  x + y;   case 1:  return -x + y;
            case 2:  return  x - y;   case 3:  return -x - y;
            case 4:  return  x + z;   case 5:  return -x + z;
            case 6:  return  x - z;   case 7:  return -x - z;
            case 8:  return  y + z;   case 9:  return -y + z;
            case 10: return  y - z;   default: return -y - z;
        }
    }

    float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }   // smootherstep — C2 continuous
    float lerp(float a, float b, float t) { return a + (b - a) * t; }

    // 3D gradient noise, roughly in [-1, 1]. Pure function — safe on any thread.
    float gradientNoise(float px, float py, float pz)
    {
        const float fx = std::floor(px), fy = std::floor(py), fz = std::floor(pz);
        const int   ix = static_cast<int>(fx), iy = static_cast<int>(fy), iz = static_cast<int>(fz);
        const float x = px - fx, y = py - fy, z = pz - fz;
        const float u = fade(x), v = fade(y), w = fade(z);

        auto corner = [&](int dx, int dy, int dz)
        {
            return gradientDot(hashCell(ix + dx, iy + dy, iz + dz),
                               x - static_cast<float>(dx), y - static_cast<float>(dy), z - static_cast<float>(dz));
        };

        const float x00 = lerp(corner(0, 0, 0), corner(1, 0, 0), u);
        const float x10 = lerp(corner(0, 1, 0), corner(1, 1, 0), u);
        const float x01 = lerp(corner(0, 0, 1), corner(1, 0, 1), u);
        const float x11 = lerp(corner(0, 1, 1), corner(1, 1, 1), u);
        return lerp(lerp(x00, x10, v), lerp(x01, x11, v), w);
    }
}

void MParticlesNoiseSimulator::simulate(float dt, std::span<SParticleData> particles,
                                        const SParticleSimContext& ctx) const
{
    const float freq  = frequency.get();
    const float accel = strength.get() * dt;
    const float t     = ctx.time * scrollSpeed.get();
    if (accel == 0.0f) return;

    // Three noise lookups per particle (one per axis, offset so they don't
    // match) — ~24 hashes + dots each. Compute-bound, so this module gains
    // the most from threads: chunk smaller and go parallel sooner than the
    // defaults.
    MParallel::forEach(particles, [freq, accel, t](SParticleData& p) {
        const SVector3 q = p.position * freq;
        const SVector3 force(gradientNoise(q.x + t,          q.y,          q.z),
                             gradientNoise(q.x + 31.416f,    q.y + t,      q.z + 47.853f),
                             gradientNoise(q.x + 12.989f,    q.y + 78.233f, q.z + t));
        p.velocity += force * accel;
    }, /*chunkSize*/ 256, /*threshold*/ 512);
}

SParticleFieldMeta MParticlesNoiseSimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    if (fieldName == "strength")
    {
        meta.speed   = 0.05f;
        meta.tooltip = "How hard the noise pushes (units / s²). Needs the Velocity module.";
    }
    else if (fieldName == "frequency")
    {
        meta.min     = 0.01f;
        meta.max     = 5.0f;
        meta.tooltip = "Size of the swirls — higher means smaller, busier motion.";
    }
    else if (fieldName == "scrollSpeed")
    {
        meta.min     = 0.0f;
        meta.max     = 5.0f;
        meta.tooltip = "How fast the noise field changes over time. 0 = frozen field.";
    }
    return meta;
}
