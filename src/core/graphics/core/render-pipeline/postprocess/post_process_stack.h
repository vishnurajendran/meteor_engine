//
// Post-process stack - pipeline attachment that owns post-process effects.
//

#pragma once
#ifndef POST_PROCESS_STACK_H
#define POST_PROCESS_STACK_H

#include <functional>
#include <typeindex>
#include <vector>

#include "core/graphics/core/render-pipeline/attachments/pipeline_attachment.h"
#include "core/utils/sstring.h"
#include "post_process_effect.h"

// The single source of truth for a pipeline's post-processing. Owns every
// effect and its settings. MPostProcessStage reads it each frame; the scene
// (MPostProcessStackEntity) only adds/removes effects and edits settings
// through this API, and saves them.
//
// Effects are unique per type. Settings pointers returned here stay valid
// until that effect is removed (or the stack is cleared) - don't cache them
// across frames; query again.
class MPostProcessStack : public IPipelineAttachment
{
public:
    ~MPostProcessStack() override;

    // ---- IPipelineAttachment -------------------------------------------------
    void onAttach(IRenderPipeline* pipeline) override;
    void onDetach(IRenderPipeline* pipeline) override;
    void onResize(int width, int height)     override;

    // ---- Effects ---------------------------------------------------------------
    // False if the type isn't registered or is already in the stack.
    template <class T> bool addEffect()    { return addEffect(std::type_index(typeid(T))); }
    template <class T> bool removeEffect() { return removeEffect(std::type_index(typeid(T))); }
    template <class T> bool hasEffect() const { return findEffect(std::type_index(typeid(T))) != nullptr; }

    bool addEffect   (const SString& typeName);   // registered name, e.g. "Vignette"
    bool removeEffect(const SString& typeName);
    bool hasEffect   (const SString& typeName) const;
    void clear();

    // ---- Settings (weak - see class comment) ------------------------------------
    template <class T>
    typename T::Settings* getSettings()
    {
        auto* effect = dynamic_cast<T*>(findEffect(std::type_index(typeid(T))));
        return effect ? &effect->settingsData() : nullptr;
    }
    SPostProcessEffectSettings* getSettings(const SString& typeName);

    // Type names in run order (slot, then order) - for saving and the inspector.
    [[nodiscard]] std::vector<SString> getEffectTypeNames() const;

    // ---- For MPostProcessStage ---------------------------------------------------
    // Effects in `slot` whose settings are enabled, in order.
    void forEachEnabled(EPostProcessSlot slot, const std::function<void(MPostProcessEffect&)>& fn) const;

    // Whole stack on/off (the scene entity's enabled state).
    void setEnabled(bool value) { enabled = value; }
    [[nodiscard]] bool isEnabled() const { return enabled; }

    // ---- Ownership -----------------------------------------------------------------
    // The scene object that currently drives the stack. Lets an old scene's
    // entity clean up on exit without wiping effects a newly loaded scene
    // has already added.
    void        setOwner(const void* newOwner) { owner = newOwner; }
    const void* getOwner() const               { return owner; }
    // Clears the stack only if `who` still owns it.
    void        releaseIfOwner(const void* who);

private:
    bool addEffect   (std::type_index type);
    bool removeEffect(std::type_index type);
    MPostProcessEffect* findEffect(std::type_index type) const;
    MPostProcessEffect* findEffect(const SString& typeName) const;
    void sortEffects();

    std::vector<MPostProcessEffect*> effects;   // owned, sorted by (slot, order)
    IRenderPipeline* pipeline = nullptr;
    int  width   = 0;
    int  height  = 0;
    bool enabled = true;
    const void* owner = nullptr;
};

#endif // POST_PROCESS_STACK_H
