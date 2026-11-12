//
// Post-process stack - pipeline attachment that owns post-process effects.
//

#include "post_process_stack.h"

#include <algorithm>

#include "post_process_registry.h"
#include "core/utils/logger.h"

MPostProcessStack::~MPostProcessStack()
{
    clear();
}

void MPostProcessStack::onAttach(IRenderPipeline* p) { pipeline = p; }

void MPostProcessStack::onDetach(IRenderPipeline*)
{
    clear();
    pipeline = nullptr;
}

void MPostProcessStack::onResize(int w, int h)
{
    width  = w;
    height = h;
    for (auto* e : effects) e->resize(w, h);
}

bool MPostProcessStack::addEffect(std::type_index type)
{
    const auto* entry = MPostProcessEffectRegistry::find(type);
    if (!entry)
    {
        MERROR("MPostProcessStack::addEffect - effect type not registered (missing REGISTER_POST_EFFECT?)");
        return false;
    }
    return addEffect(entry->name);
}

bool MPostProcessStack::addEffect(const SString& typeName)
{
    if (findEffect(typeName)) return false;   // one per type

    const auto* entry = MPostProcessEffectRegistry::find(typeName);
    if (!entry)
    {
        MERROR("MPostProcessStack::addEffect - unknown effect: " + typeName);
        return false;
    }

    MPostProcessEffect* effect = entry->create();
    if (!effect) return false;

    effect->setTypeName(entry->name);
    effect->init(pipeline);
    if (width > 0 && height > 0) effect->resize(width, height);

    effects.push_back(effect);
    sortEffects();
    return true;
}

bool MPostProcessStack::removeEffect(std::type_index type)
{
    const auto* entry = MPostProcessEffectRegistry::find(type);
    return entry ? removeEffect(entry->name) : false;
}

bool MPostProcessStack::removeEffect(const SString& typeName)
{
    for (auto it = effects.begin(); it != effects.end(); ++it)
    {
        if ((*it)->getTypeName() != typeName) continue;
        (*it)->cleanup();
        delete *it;
        effects.erase(it);
        return true;
    }
    return false;
}

bool MPostProcessStack::hasEffect(const SString& typeName) const
{
    return findEffect(typeName) != nullptr;
}

void MPostProcessStack::clear()
{
    for (auto* e : effects)
    {
        e->cleanup();
        delete e;
    }
    effects.clear();
}

void MPostProcessStack::releaseIfOwner(const void* who)
{
    if (owner != who) return;
    clear();
    owner   = nullptr;
    enabled = true;
}

SPostProcessEffectSettings* MPostProcessStack::getSettings(const SString& typeName)
{
    auto* effect = findEffect(typeName);
    return effect ? &effect->getSettings() : nullptr;
}

std::vector<SString> MPostProcessStack::getEffectTypeNames() const
{
    std::vector<SString> names;
    names.reserve(effects.size());
    for (const auto* e : effects) names.push_back(e->getTypeName());
    return names;
}

void MPostProcessStack::forEachEnabled(EPostProcessSlot slot,
                                       const std::function<void(MPostProcessEffect&)>& fn) const
{
    if (!enabled) return;
    for (auto* e : effects)
        if (e->getSlot() == slot && e->getSettings().enabled.get())
            fn(*e);
}

MPostProcessEffect* MPostProcessStack::findEffect(std::type_index type) const
{
    const auto* entry = MPostProcessEffectRegistry::find(type);
    return entry ? findEffect(entry->name) : nullptr;
}

MPostProcessEffect* MPostProcessStack::findEffect(const SString& typeName) const
{
    for (auto* e : effects)
        if (e->getTypeName() == typeName) return e;
    return nullptr;
}

void MPostProcessStack::sortEffects()
{
    std::stable_sort(effects.begin(), effects.end(),
        [](const MPostProcessEffect* a, const MPostProcessEffect* b)
        {
            if (a->getSlot() != b->getSlot())
                return static_cast<int>(a->getSlot()) < static_cast<int>(b->getSlot());
            return a->getOrder() < b->getOrder();
        });
}
