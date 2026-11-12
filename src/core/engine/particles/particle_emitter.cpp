//
// particle_emitter.cpp
//

#include "particle_emitter.h"

#include <algorithm>
#include <cmath>

#include "core/engine/gizmos/gizmos.h"
#include "core/utils/logger.h"
#include "simulators/particle_velocity_simulator.h"

IMPLEMENT_SPATIAL_CLASS(MParticleEmitter)

namespace
{
    // Integer hash — spreads a sequential counter into a well-distributed seed.
    uint32_t hashSeed(uint32_t x)
    {
        x ^= x >> 16; x *= 0x7feb352dU;
        x ^= x >> 15; x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    // Tiny PCG-style generator, local to one spawn — no shared RNG state.
    struct SRandom
    {
        uint32_t state;

        float next01()   // [0, 1)
        {
            state = state * 747796405u + 2891336453u;
            uint32_t w = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            w = (w >> 22u) ^ w;
            return static_cast<float>(w >> 8) * (1.0f / 16777216.0f);
        }

        float nextSigned() { return next01() * 2.0f - 1.0f; }   // [-1, 1)

        // Uniform direction on the unit sphere.
        SVector3 unitVector()
        {
            const float z   = nextSigned();
            const float phi = next01() * 6.28318530718f;
            const float r   = std::sqrt(std::max(0.0f, 1.0f - z * z));
            return { r * std::cos(phi), r * std::sin(phi), z };
        }
    };

    constexpr float MIN_LIFETIME = 0.0001f;
    const SVector3  EMIT_AXIS(0.0f, 1.0f, 0.0f);   // local +Y
}

MParticleEmitter::MParticleEmitter()
{
    name = "Particle Emitter";
    setCanTick(true);

    // Something visible out of the box: particles that move and get drawn.
    // A loaded scene replaces these with its saved modules (onDeserialise).
    addSimulator("Velocity");
    setRenderer("Sprite");
}

MParticleEmitter::~MParticleEmitter()
{
    removeAllSimulators();
    setRenderer({});
}

// -----------------------------------------------------------------------------
// Entity hooks
// -----------------------------------------------------------------------------

void MParticleEmitter::onUpdate(float deltaTime)
{
    MSpatialEntity::onUpdate(deltaTime);
    if (autoSimulate.get())
        simulate(deltaTime);
}

void MParticleEmitter::onExit()
{
    // Leaving the scene — stop drawing now; the renderer is deleted with us.
    if (activeRenderer) activeRenderer->cleanup();
    clear();
    MSpatialEntity::onExit();
}

// -----------------------------------------------------------------------------
// Simulation
// -----------------------------------------------------------------------------

SParticleSimContext MParticleEmitter::getSimContext() const
{
    SParticleSimContext ctx;
    ctx.space          = simulationSpace.get();
    ctx.emitterToWorld  = getTransformMatrix();
    ctx.emitterRotation = getWorldRotation();
    ctx.time            = elapsedTime;
    if (ctx.space == EParticleSimulationSpace::Local)
        ctx.worldToSimulation = glm::inverse(getWorldRotation());
    return ctx;
}

void MParticleEmitter::simulate(float dt)
{
    if (dt <= 0.0f) return;

    ensurePoolSize();      // one compare per frame — reallocates only when maxParticles changed
    elapsedTime += dt;
    const SParticleSimContext ctx = getSimContext();

    ageAndKill(dt);        // serial — swap-and-pop reorders the pool

    // Rate accumulator: 10/s at 60 fps is 0.166 per frame — carry the fraction.
    spawnAccumulator += std::max(0.0f, emissionRate.get()) * dt;
    const auto wanted = static_cast<uint32_t>(spawnAccumulator);
    spawnAccumulator -= static_cast<float>(wanted);
    spawnParticles(wanted, ctx);   // serial — bumps aliveCount and the seed counter

    const std::span<SParticleData> alive(particlePool.data(), aliveCount);
    for (const IParticleSimulator* sim : simulators)
        if (sim->enabled.get())
            sim->simulate(dt, alive, ctx);   // module decides serial vs parallel
}

void MParticleEmitter::emit(int count)
{
    if (count <= 0) return;
    ensurePoolSize();
    spawnParticles(static_cast<uint32_t>(count), getSimContext());
}

void MParticleEmitter::clear()
{
    aliveCount       = 0;
    spawnAccumulator = 0.0f;
}

void MParticleEmitter::ensurePoolSize()
{
    const auto target = static_cast<uint32_t>(std::max(0, maxParticles.get()));
    if (particlePool.size() == target) return;

    // Rare — maxParticles changed in the inspector or a script. Survivors
    // keep their state; any overflow is dropped.
    particlePool.resize(target);
    aliveCount = std::min(aliveCount, target);
}

void MParticleEmitter::ageAndKill(float dt)
{
    uint32_t i = 0;
    while (i < aliveCount)
    {
        SParticleData& p = particlePool[i];
        p.age += dt;
        if (p.normalizedAge() >= 1.0f)
        {
            // Swap-and-pop. The particle moved into slot i hasn't been aged
            // yet, so don't advance i.
            p = particlePool[aliveCount - 1];
            --aliveCount;
        }
        else
        {
            ++i;
        }
    }
}

void MParticleEmitter::spawnParticles(uint32_t count, const SParticleSimContext& ctx)
{
    const auto freeSlots = static_cast<uint32_t>(particlePool.size()) - aliveCount;
    count = std::min(count, freeSlots);   // pool full — extra spawns are dropped
    if (count == 0) return;

    const float    invLifetime = 1.0f / std::max(startLifetime.get(), MIN_LIFETIME);
    const SVector4 color(startColor.get(), startAlpha.get());
    // 3D on: per-axis size. Off: the uniform Start Size on every axis.
    const SVector3 size = startSize3D.get() ? glm::max(startSizeXYZ.get(), SVector3(0.0f))
                                            : SVector3(std::max(0.0f, startSize.get()));
    const float    speed  = startSpeed.get();
    const bool     global = ctx.space == EParticleSimulationSpace::Global;
    const SQuaternion emitterRotation = getWorldRotation();

    const uint32_t first = aliveCount;
    for (uint32_t i = first; i < first + count; ++i)
    {
        SParticleData& p = particlePool[i];
        p = SParticleData{};   // reset — a recycled slot still holds the previous particle
        p.seed        = hashSeed(spawnCounter++);
        p.invLifetime = invLifetime;
        p.startSize   = p.size  = size;
        p.startColor  = p.color = color;

        SVector3 position, direction;
        sampleShape(p.seed, position, direction);
        p.position = position;
        p.velocity = direction * speed;

        // Global: born at the emitter's current world pose, then left behind.
        if (global)
        {
            p.position = SVector3(ctx.emitterToWorld * SVector4(position, 1.0f));
            p.velocity = emitterRotation * p.velocity;
        }
    }
    aliveCount += count;

    const std::span<SParticleData> spawned(particlePool.data() + first, count);
    for (const IParticleSimulator* sim : simulators)
        if (sim->enabled.get())
            sim->onSpawn(spawned, ctx);
}

void MParticleEmitter::sampleShape(uint32_t seed, SVector3& outPosition, SVector3& outDirection) const
{
    SRandom rng{ seed };
    outPosition  = SVector3(0.0f);
    outDirection = EMIT_AXIS;

    const float    radius  = std::max(0.0f, shapeRadius.get());
    const SVector3 extents = glm::max(shapeExtents.get(), SVector3(0.0f));

    // Uniform point on a disc in the XZ plane: sqrt keeps the density even
    // (without it points bunch up in the middle).
    auto discPoint = [&rng](float r, float& outU, float& outAngle) -> SVector3
    {
        outU     = std::sqrt(rng.next01());
        outAngle = rng.next01() * 6.28318530718f;
        return { std::cos(outAngle) * outU * r, 0.0f, std::sin(outAngle) * outU * r };
    };

    switch (emissionShape.get())
    {
        case EParticleEmitterShape::Point:
            outDirection = rng.unitVector();
            break;

        case EParticleEmitterShape::Square:
            outPosition = { rng.nextSigned() * extents.x, 0.0f, rng.nextSigned() * extents.z };
            break;

        case EParticleEmitterShape::Circle:
        {
            float u, angle;
            outPosition = discPoint(radius, u, angle);
            break;
        }

        case EParticleEmitterShape::Box:
            outPosition = { rng.nextSigned() * extents.x, rng.nextSigned() * extents.y, rng.nextSigned() * extents.z };
            break;

        case EParticleEmitterShape::Sphere:
        {
            outDirection = rng.unitVector();
            // cbrt keeps the density even through the volume.
            outPosition  = outDirection * radius * std::cbrt(rng.next01());
            break;
        }

        case EParticleEmitterShape::Cone:
        {
            float u, angle;
            outPosition = discPoint(radius, u, angle);
            // Tilt outwards in proportion to how far from the centre the
            // particle starts: straight up at the centre, shapeAngle at the rim.
            const float spread = std::tan(glm::radians(std::clamp(shapeAngle.get(), 0.0f, 89.0f)));
            const SVector3 outward(std::cos(angle), 0.0f, std::sin(angle));
            outDirection = glm::normalize(EMIT_AXIS + outward * (u * spread));
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// Gizmo
// -----------------------------------------------------------------------------

void MParticleEmitter::onDrawGizmo(SVector2 renderResolution)
{
    MSpatialEntity::onDrawGizmo(renderResolution);
#if METEOR_EDITOR
    drawShapeGizmo();
#endif
}

void MParticleEmitter::drawShapeGizmo() const
{
    const SColor color(0.45f, 0.85f, 1.0f, 1.0f);
    constexpr float THICKNESS = 0.1f;
    constexpr float LENGTH    = 1.0f;   // cone height / arrow length, in emitter units
    constexpr int   SEGMENTS  = 32;     // per circle — ~100 lines per frame for a cone, batched

    // Shapes are authored around local +Y; the Velocity module's direction
    // turns the whole frame (same rotation onSpawn applies to particles).
    SQuaternion frame = glm::identity<SQuaternion>();
    if (const auto* vel = dynamic_cast<const MParticlesVelocitySimulator*>(getSimulator("Velocity")))
        if (vel->enabled.get())
            frame = vel->getEmissionRotation();

    // Everything is drawn in shape space and pushed through one matrix, so
    // the emitter's position, rotation and scale all apply.
    const SMatrix4 toWorld = getTransformMatrix() * glm::mat4_cast(frame);
    auto line = [&](const SVector3& a, const SVector3& b)
    {
        MGizmos::drawLine(SVector3(toWorld * SVector4(a, 1.0f)), SVector3(toWorld * SVector4(b, 1.0f)), color, THICKNESS);
    };
    // Circle of radius r in the plane spanned by axes u and v, centred on c.
    auto circle = [&](const SVector3& c, const SVector3& u, const SVector3& v, float r)
    {
        SVector3 prev = c + u * r;
        for (int i = 1; i <= SEGMENTS; ++i)
        {
            const float a = 6.28318530718f * static_cast<float>(i) / SEGMENTS;
            const SVector3 next = c + (u * std::cos(a) + v * std::sin(a)) * r;
            line(prev, next);
            prev = next;
        }
    };

    const SVector3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1), O(0.0f);
    const float    r = std::max(0.0f, shapeRadius.get());
    const SVector3 e = glm::max(shapeExtents.get(), SVector3(0.0f));

    switch (emissionShape.get())
    {
        case EParticleEmitterShape::Point:
        {
            constexpr float s = 0.1f;
            line(-X * s, X * s);
            line(-Y * s, Y * s);
            line(-Z * s, Z * s);
            break;
        }
        case EParticleEmitterShape::Square:
        {
            const SVector3 a(-e.x, 0, -e.z), b(e.x, 0, -e.z), c(e.x, 0, e.z), d(-e.x, 0, e.z);
            line(a, b); line(b, c); line(c, d); line(d, a);
            break;
        }
        case EParticleEmitterShape::Circle:
            circle(O, X, Z, r);
            break;

        case EParticleEmitterShape::Box:
        {
            // 8 corners, 12 edges: corners differing in exactly one axis are joined.
            auto corner = [&](int i) { return SVector3((i & 1) ? e.x : -e.x, (i & 2) ? e.y : -e.y, (i & 4) ? e.z : -e.z); };
            for (int i = 0; i < 8; ++i)
                for (int bit = 1; bit < 8; bit <<= 1)
                    if (!(i & bit)) line(corner(i), corner(i | bit));
            break;
        }
        case EParticleEmitterShape::Sphere:
            circle(O, X, Z, r);
            circle(O, X, Y, r);
            circle(O, Y, Z, r);
            break;

        case EParticleEmitterShape::Cone:
        {
            // Base disc, plus where the rim's spread reaches after LENGTH.
            const float spread = std::tan(glm::radians(std::clamp(shapeAngle.get(), 0.0f, 89.0f)));
            const float top    = r + LENGTH * spread;
            circle(O, X, Z, r);
            circle(Y * LENGTH, X, Z, top);
            line( X * r, Y * LENGTH + X * top);
            line(-X * r, Y * LENGTH - X * top);
            line( Z * r, Y * LENGTH + Z * top);
            line(-Z * r, Y * LENGTH - Z * top);
            break;
        }
    }

    // Emission direction arrow (Point and Sphere emit everywhere — no arrow).
    const EParticleEmitterShape shape = emissionShape.get();
    if (shape != EParticleEmitterShape::Point && shape != EParticleEmitterShape::Sphere)
    {
        const SVector3 tip = Y * LENGTH;
        line(O, tip);
        line(tip, tip + (-Y + X) * 0.12f);
        line(tip, tip + (-Y - X) * 0.12f);
        line(tip, tip + (-Y + Z) * 0.12f);
        line(tip, tip + (-Y - Z) * 0.12f);
    }
}

// -----------------------------------------------------------------------------
// Modules
// -----------------------------------------------------------------------------

IParticleSimulator* MParticleEmitter::addSimulator(const SString& typeName)
{
    if (auto* existing = getSimulator(typeName))
        return existing;

    const auto* entry = MParticleSimulatorRegistry::find(typeName);
    if (!entry)
    {
        MWARN("Particle Emitter: unknown simulator module '" + typeName + "'");
        return nullptr;
    }

    IParticleSimulator* sim = entry->create();
    sim->setTypeName(entry->name);
    simulators.push_back(sim);
    sortSimulators();
    return sim;
}

bool MParticleEmitter::removeSimulator(const SString& typeName)
{
    for (auto it = simulators.begin(); it != simulators.end(); ++it)
    {
        if ((*it)->getTypeName() != typeName) continue;
        delete *it;   // virtual destructor — MObject::operator delete returns it to its pool
        simulators.erase(it);
        return true;
    }
    return false;
}

IParticleSimulator* MParticleEmitter::getSimulator(const SString& typeName) const
{
    for (IParticleSimulator* sim : simulators)
        if (sim->getTypeName() == typeName) return sim;
    return nullptr;
}

void MParticleEmitter::removeAllSimulators()
{
    for (IParticleSimulator* sim : simulators) delete sim;
    simulators.clear();
}

void MParticleEmitter::sortSimulators()
{
    std::stable_sort(simulators.begin(), simulators.end(),
                     [](const IParticleSimulator* a, const IParticleSimulator* b)
                     { return a->getSortingOrder() < b->getSortingOrder(); });
}

IParticleRenderer* MParticleEmitter::setRenderer(const SString& typeName)
{
    if (activeRenderer && activeRenderer->getTypeName() == typeName)
        return activeRenderer;

    if (activeRenderer)
    {
        activeRenderer->cleanup();
        delete activeRenderer;
        activeRenderer = nullptr;
    }
    if (typeName.empty()) return nullptr;

    const auto* entry = MParticleRendererRegistry::find(typeName);
    if (!entry)
    {
        MWARN("Particle Emitter: unknown renderer '" + typeName + "'");
        return nullptr;
    }

    activeRenderer = entry->create();
    activeRenderer->setTypeName(entry->name);
    activeRenderer->setEmitter(this);
    return activeRenderer;
}

// -----------------------------------------------------------------------------
// Serialisation
// -----------------------------------------------------------------------------

void MParticleEmitter::onSerialise(pugi::xml_node& node)
{
    MSpatialEntity::onSerialise(node);

    pugi::xml_node modules = node.append_child("particle_modules");
    for (IParticleSimulator* sim : simulators)
    {
        pugi::xml_node m = modules.append_child("module");
        m.append_attribute("type").set_value(sim->getTypeName().c_str());
        sim->serialiseToNode(m);
    }

    if (activeRenderer)
    {
        pugi::xml_node r = node.append_child("particle_renderer");
        r.append_attribute("type").set_value(activeRenderer->getTypeName().c_str());
        activeRenderer->serialiseToNode(r);
    }
}

void MParticleEmitter::onDeserialise(const pugi::xml_node& node)
{
    MSpatialEntity::onDeserialise(node);

    // Saved modules replace the constructor's defaults. A file without the
    // node (older scene) keeps the defaults.
    if (const pugi::xml_node modules = node.child("particle_modules"))
    {
        removeAllSimulators();
        for (const pugi::xml_node m : modules.children("module"))
        {
            const SString type = m.attribute("type").as_string();
            if (IParticleSimulator* sim = addSimulator(type))
                sim->deserialiseFromNode(m);
        }
    }

    if (const pugi::xml_node r = node.child("particle_renderer"))
    {
        if (IParticleRenderer* renderer = setRenderer(r.attribute("type").as_string()))
            renderer->deserialiseFromNode(r);
    }
    else if (node.child("particle_modules"))
    {
        // Saved by this version with no renderer — the user removed it.
        setRenderer({});
    }

    clear();
}
