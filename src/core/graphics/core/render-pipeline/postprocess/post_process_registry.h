//
// Post-process effect registry - effect types by name.
//

#pragma once
#ifndef POST_PROCESS_REGISTRY_H
#define POST_PROCESS_REGISTRY_H

#include <functional>
#include <typeindex>
#include <vector>

#include "core/memory/mnew.h"
#include "core/utils/sstring.h"

class MPostProcessEffect;

// Every effect type registers here once (REGISTER_POST_EFFECT), so it can be
// created from a name: scene loading, and the inspector's Add Effect list.
class MPostProcessEffectRegistry
{
public:
    struct SEntry
    {
        SString                              name;      // saved in scene files - don't rename lightly
        std::type_index                      type;
        std::function<MPostProcessEffect*()> create;
    };

    static bool registerEffect(const SString& name, std::type_index type,
                               std::function<MPostProcessEffect*()> factory);

    static const SEntry* find(const SString& name);
    static const SEntry* find(std::type_index type);
    static const std::vector<SEntry>& getAll();
};

// In the effect's .cpp:
//   REGISTER_POST_EFFECT(MVignetteEffect, "Vignette")
#define REGISTER_POST_EFFECT(EffectClass, EffectName)                                   \
    static const bool EffectClass##_postEffectRegistered =                              \
        MPostProcessEffectRegistry::registerEffect(EffectName, typeid(EffectClass),     \
            []() -> MPostProcessEffect* { return mnew<EffectClass>(); });

#endif // POST_PROCESS_REGISTRY_H
