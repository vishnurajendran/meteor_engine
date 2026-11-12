//
// Velocity — moves particles: position += velocity * dt.
//

#include "particle_velocity_simulator.h"

#include <algorithm>

#include "core/engine/particles/particle_registry.h"
#include "core/utils/parallel_for.h"

REGISTER_PARTICLE_SIMULATOR(MParticlesVelocitySimulator, "Velocity")

namespace
{
    const SVector3 EMIT_AXIS(0.0f, 1.0f, 0.0f);   // what the emitter's shapes are authored around

    // Shortest rotation taking unit vector `from` onto unit vector `to`.
    SQuaternion rotationBetween(const SVector3& from, const SVector3& to)
    {
        const float d = glm::dot(from, to);
        if (d > 0.99999f) return glm::identity<SQuaternion>();   // same direction
        if (d < -0.99999f)                                        // opposite — any perpendicular axis works
            return SQuaternion(0.0f, 1.0f, 0.0f, 0.0f);           // 180° around X (w, x, y, z)

        // Half-angle trick: (1 + cos θ, sin θ * axis) normalised is the
        // quaternion for θ around axis — no trig needed.
        const SVector3 axis = glm::cross(from, to);
        return glm::normalize(SQuaternion(1.0f + d, axis.x, axis.y, axis.z));
    }

    bool isIdentity(const SQuaternion& q) { return q.w > 0.999999f; }
}

SQuaternion MParticlesVelocitySimulator::getEmissionRotation() const
{
    const SVector3 dir = direction.get();
    const float    len = glm::length(dir);
    if (len < 1e-6f) return glm::identity<SQuaternion>();   // zero vector — keep +Y
    return rotationBetween(EMIT_AXIS, dir / len);
}

void MParticlesVelocitySimulator::onSpawn(std::span<SParticleData> spawned, const SParticleSimContext& ctx) const
{
    const SQuaternion q = getEmissionRotation();
    if (isIdentity(q)) return;   // default +Y — nothing to do

    // Serial: spawn batches are small (rate * dt), and onSpawn is always serial.
    if (ctx.space == EParticleSimulationSpace::Local)
    {
        // Positions and velocities are in emitter space — rotate both about
        // the emitter's origin.
        for (SParticleData& p : spawned)
        {
            p.position = q * p.position;
            p.velocity = q * p.velocity;
        }
        return;
    }

    // Global: spawn has already moved these into world space. Rotate in
    // emitter space, then go back to world. One matrix inverse per batch.
    const SMatrix4    toLocal = glm::inverse(ctx.emitterToWorld);
    const SQuaternion worldQ  = ctx.emitterRotation * q * glm::inverse(ctx.emitterRotation);
    for (SParticleData& p : spawned)
    {
        const SVector3 local = SVector3(toLocal * SVector4(p.position, 1.0f));
        p.position = SVector3(ctx.emitterToWorld * SVector4(q * local, 1.0f));
        p.velocity = worldQ * p.velocity;
    }
}

void MParticlesVelocitySimulator::simulate(float dt, std::span<SParticleData> particles,
                                           const SParticleSimContext& ctx) const
{
    // Linear drag as a per-frame scale. Clamped so a big dt can't flip the
    // velocity's direction.
    const float damping = std::clamp(1.0f - drag.get() * dt, 0.0f, 1.0f);

    MParallel::forEach(particles, [dt, damping](SParticleData& p) {
        p.velocity *= damping;
        p.position += p.velocity * dt;   // explicit Euler — fine for visual effects
    });
}

SParticleFieldMeta MParticlesVelocitySimulator::getFieldMeta(const std::string& fieldName) const
{
    SParticleFieldMeta meta;
    if (fieldName == "direction")
    {
        meta.speed   = 0.01f;
        meta.tooltip = "Emission axis in the emitter's local space (normalised for you).\n"
                       "The whole shape turns to face it — (0, 1, 0) is the default up.";
    }
    else if (fieldName == "drag")
    {
        meta.min     = 0.0f;
        meta.max     = 10.0f;
        meta.tooltip = "Slows particles down. Fraction of speed lost per second (0 = none).";
    }
    return meta;
}
