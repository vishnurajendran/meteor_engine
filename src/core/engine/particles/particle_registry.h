//
// particle_registry.h
//
// Simulator modules and renderers by name — for scene loading (the type
// name is saved) and the inspector's Add Module / Renderer lists. Same
// pattern as MPostProcessEffectRegistry.
//
// In the module's .cpp:
//   REGISTER_PARTICLE_SIMULATOR(MParticlesGravitySimulator, "Gravity")
//   REGISTER_PARTICLE_RENDERER(MSpriteParticleRenderer, "Sprite")
//
// Names are saved in scene files — don't rename them lightly.
//
#pragma once
#ifndef METEOR_ENGINE_PARTICLE_REGISTRY_H
#define METEOR_ENGINE_PARTICLE_REGISTRY_H

#include <functional>
#include <typeindex>
#include <vector>

#include "core/memory/mnew.h"
#include "core/utils/logger.h"
#include "core/utils/sstring.h"

class IParticleSimulator;
class IParticleRenderer;

template <class TBase>
class TParticleTypeRegistry
{
public:
    struct SEntry
    {
        SString                 name;
        std::type_index         type;
        std::function<TBase*()> create;
    };

    static bool add(const SString& name, std::type_index type, std::function<TBase*()> factory)
    {
        if (find(name))
        {
            MERROR("Particle type registered twice: " + name);
            return false;
        }
        entries().push_back({ name, type, std::move(factory) });
        return true;
    }

    static const SEntry* find(const SString& name)
    {
        for (const auto& e : entries())
            if (e.name == name) return &e;
        return nullptr;
    }

    static const SEntry* find(std::type_index type)
    {
        for (const auto& e : entries())
            if (e.type == type) return &e;
        return nullptr;
    }

    static const std::vector<SEntry>& getAll() { return entries(); }

private:
    // Function-local static: safe to use from other translation units'
    // static initialisers (the REGISTER_ macros), whatever order they run in.
    static std::vector<SEntry>& entries()
    {
        static std::vector<SEntry> instance;
        return instance;
    }
};

using MParticleSimulatorRegistry = TParticleTypeRegistry<IParticleSimulator>;
using MParticleRendererRegistry  = TParticleTypeRegistry<IParticleRenderer>;

#define REGISTER_PARTICLE_SIMULATOR(SimClass, SimName)                                    \
    static const bool SimClass##_particleSimRegistered =                                  \
        MParticleSimulatorRegistry::add(SimName, typeid(SimClass),                        \
            []() -> IParticleSimulator* { return mnew<SimClass>(); });

#define REGISTER_PARTICLE_RENDERER(RendererClass, RendererName)                           \
    static const bool RendererClass##_particleRendererRegistered =                        \
        MParticleRendererRegistry::add(RendererName, typeid(RendererClass),               \
            []() -> IParticleRenderer* { return mnew<RendererClass>(); });

#endif // METEOR_ENGINE_PARTICLE_REGISTRY_H
