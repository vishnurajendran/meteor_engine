//
// particle_emitter.h
//
// MParticleEmitter — spawns particles, runs its simulator modules over
// them, and owns the renderer that draws them.
//
// Frame (simulate):
//   ensurePoolSize → ageAndKill → spawn → every enabled module, in order
//
//   ageAndKill and spawn are serial (they reorder the pool / bump counters).
//   Modules may go parallel internally (MParallel::forEach).
//
// Memory: one std::vector<SParticleData> sized to maxParticles — the pool.
// [0, aliveCount) are alive and always contiguous; killing swaps the last
// alive particle into the hole. Nothing is allocated per particle or per
// frame once the pool exists.
//
// Saved with the scene: the DECLARE_FIELDs below, plus every module and the
// renderer by type name with their own fields (onSerialise).
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_EMITTER_H
#define METEOR_ENGINE_PARTICLE_EMITTER_H

#include <span>
#include <type_traits>
#include <vector>

#include "core/engine/entities/spatial/spatial.h"
#include "core/utils/field_engine_types.h"
#include "particle_data.h"
#include "particle_registry.h"
#include "particle_renderer.h"
#include "particle_simulator.h"

// exposed to the scripting system
SCRIPT_BIND_CLASS()
class MParticleEmitter : public MSpatialEntity
{
    DEFINE_SPATIAL_CLASS(MParticleEmitter)

    // region, vars
    DECLARE_FIELD(maxParticles,    int,                      100)
    DECLARE_FIELD(autoSimulate,    bool,                     true)
    DECLARE_FIELD(emissionRate,    float,                    10.0f)    // particles per second
    DECLARE_FIELD(emissionShape,   EParticleEmitterShape,    EParticleEmitterShape::Cone)
    DECLARE_FIELD(simulationSpace, EParticleSimulationSpace, EParticleSimulationSpace::Local)

    // Start values — every new particle gets these.
    DECLARE_FIELD(startLifetime,   float,    2.0f)             // seconds
    DECLARE_FIELD(startSpeed,      float,    2.0f)             // units per second, along the shape's direction
    DECLARE_FIELD(startSize,       float,    0.5f)             // uniform size — used while startSize3D is off
    DECLARE_FIELD(startSize3D,     bool,     false)            // on: use startSizeXYZ instead
    DECLARE_FIELD(startSizeXYZ,    SVector3, SVector3(0.5f))   // per-axis size — X/Y for sprites, X/Y/Z for meshes
    DECLARE_FIELD(startColor,      SVector3, SVector3(1.0f))   // sRGB
    DECLARE_FIELD(startAlpha,      float,    1.0f)

    // Shape parameters — which ones apply depends on emissionShape.
    DECLARE_FIELD(shapeRadius,     float,    0.5f)             // Circle, Sphere, Cone
    DECLARE_FIELD(shapeAngle,      float,    25.0f)            // Cone — degrees from +Y at the rim
    DECLARE_FIELD(shapeExtents,    SVector3, SVector3(0.5f))   // Square (x, z), Box — half sizes

public:
    MParticleEmitter();
    ~MParticleEmitter() override;

    // ---- Simulation --------------------------------------------------------------

    // public api on simulate, used to step in the simulation.
    SCRIPT_BIND_FUNC()
    void simulate(float dt);

    // Spawns `count` particles right now (a burst), on top of the rate.
    SCRIPT_BIND_FUNC()
    void emit(int count);

    // Kills every particle.
    SCRIPT_BIND_FUNC()
    void clear();

    SCRIPT_BIND_FUNC()
    [[nodiscard]] int getAliveCount() const { return static_cast<int>(aliveCount); }

    SCRIPT_BIND_FUNC()
    [[nodiscard]] float getEmissionRate() const { return emissionRate.get(); }
    SCRIPT_BIND_FUNC()
    void setEmissionRate(float rate) { emissionRate = rate < 0.0f ? 0.0f : rate; }

    SCRIPT_BIND_FUNC()
    [[nodiscard]] bool getAutoSimulate() const { return autoSimulate.get(); }
    SCRIPT_BIND_FUNC()
    void setAutoSimulate(bool value) { autoSimulate = value; }

    [[nodiscard]] std::span<const SParticleData> getAliveParticles() const { return { particlePool.data(), aliveCount }; }

    // This frame's context — what modules and the renderer get.
    [[nodiscard]] SParticleSimContext getSimContext() const;

    // ---- Modules -------------------------------------------------------------------

    // api to attach simulator... T must be registered (REGISTER_PARTICLE_SIMULATOR).
    // Returns the existing module if one of that type is already attached.
    template <typename T>
    T* attachSimulator()
    {
        static_assert(std::is_base_of_v<MObject, T> && std::is_base_of_v<IParticleSimulator, T>,
                      "T must be an MObject implementing IParticleSimulator");
        const auto* entry = MParticleSimulatorRegistry::find(typeid(T));
        if (!entry)
        {
            MERROR("attachSimulator: type is not registered — add REGISTER_PARTICLE_SIMULATOR");
            return nullptr;
        }
        return dynamic_cast<T*>(addSimulator(entry->name));
    }

    // By registered name ("Gravity"). nullptr if the name is unknown.
    // One module per type: returns the existing one if already attached.
    IParticleSimulator* addSimulator(const SString& typeName);
    bool                removeSimulator(const SString& typeName);
    [[nodiscard]] IParticleSimulator* getSimulator(const SString& typeName) const;
    [[nodiscard]] const std::vector<IParticleSimulator*>& getSimulators() const { return simulators; }

    // ---- Renderer --------------------------------------------------------------------

    // api is simple attach Sprite/ Mesh type renderer. Replaces the current one.
    template <typename T>
    T* attachRenderer()
    {
        static_assert(std::is_base_of_v<MObject, T> && std::is_base_of_v<IParticleRenderer, T>,
                      "T must be an MObject implementing IParticleRenderer");
        const auto* entry = MParticleRendererRegistry::find(typeid(T));
        if (!entry)
        {
            MERROR("attachRenderer: type is not registered — add REGISTER_PARTICLE_RENDERER");
            return nullptr;
        }
        return dynamic_cast<T*>(setRenderer(entry->name));
    }

    // By registered name ("Sprite"). Empty name removes the renderer.
    IParticleRenderer* setRenderer(const SString& typeName);
    [[nodiscard]] IParticleRenderer* getRenderer() const { return activeRenderer; }

    // ---- Entity hooks ------------------------------------------------------------------
    void onUpdate(float deltaTime) override;
    void onExit() override;
    // Emission shape wireframe + direction arrow (editor only).
    void onDrawGizmo(SVector2 renderResolution) override;

protected:
    //   <particle_modules>
    //     <module type="Gravity"> <enabled>true</enabled> <gravity>...</gravity> </module>
    //   </particle_modules>
    //   <particle_renderer type="Sprite"> ... </particle_renderer>
    void onSerialise(pugi::xml_node& node) override;
    void onDeserialise(const pugi::xml_node& node) override;

private:
    void ensurePoolSize();
    void ageAndKill(float dt);
    void spawnParticles(uint32_t count, const SParticleSimContext& ctx);
    void sampleShape(uint32_t seed, SVector3& outPosition, SVector3& outDirection) const;
    void sortSimulators();
    void removeAllSimulators();
    void drawShapeGizmo() const;

    // internal vars
    std::vector<SParticleData> particlePool;      // size == maxParticles — the pool
    uint32_t aliveCount       = 0;                // [0, aliveCount) alive — dense, no holes
    float    spawnAccumulator = 0.0f;             // fractional spawns carried across frames
    uint32_t spawnCounter     = 0;                // hashed into per-particle seeds
    float    elapsedTime      = 0.0f;

    // Owned. Sorted by getSortingOrder() — sorted on attach, never per frame.
    std::vector<IParticleSimulator*> simulators;
    IParticleRenderer*               activeRenderer = nullptr;
};

#endif // METEOR_ENGINE_PARTICLE_EMITTER_H
