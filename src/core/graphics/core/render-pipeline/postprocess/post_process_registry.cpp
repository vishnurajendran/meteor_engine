//
// Post-process effect registry - effect types by name.
//

#include "post_process_registry.h"
#include "core/utils/logger.h"

// Function-local static: safe to use from other translation units' static
// initialisers (REGISTER_POST_EFFECT), whatever order they run in.
static std::vector<MPostProcessEffectRegistry::SEntry>& entries()
{
    static std::vector<MPostProcessEffectRegistry::SEntry> instance;
    return instance;
}

bool MPostProcessEffectRegistry::registerEffect(const SString& name, std::type_index type,
                                                std::function<MPostProcessEffect*()> factory)
{
    if (find(name))
    {
        MERROR("Post-process effect registered twice: " + name);
        return false;
    }
    entries().push_back({ name, type, std::move(factory) });
    return true;
}

const MPostProcessEffectRegistry::SEntry* MPostProcessEffectRegistry::find(const SString& name)
{
    for (const auto& e : entries())
        if (e.name == name) return &e;
    return nullptr;
}

const MPostProcessEffectRegistry::SEntry* MPostProcessEffectRegistry::find(std::type_index type)
{
    for (const auto& e : entries())
        if (e.type == type) return &e;
    return nullptr;
}

const std::vector<MPostProcessEffectRegistry::SEntry>& MPostProcessEffectRegistry::getAll()
{
    return entries();
}
