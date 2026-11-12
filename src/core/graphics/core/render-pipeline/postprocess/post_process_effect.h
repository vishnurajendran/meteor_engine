//
// Post-process effect - the GPU half of an effect.
//

#pragma once
#ifndef POST_PROCESS_EFFECT_H
#define POST_PROCESS_EFFECT_H

#include "core/object/object.h"
#include "core/utils/glmhelper.h"
#include "post_process_settings.h"

class IRenderPipeline;
class MBufferRegistery;
class MShader;
class SFrameBuffer;
class SFullscreenQuad;
struct SViewData;

// Where in the frame an effect runs (both on BUFFER_SCENE, linear HDR):
//   BeforeTransparent - RS_PostOpaque: sees opaque surfaces only (SSAO, SSR).
//   AfterTransparent  - RS_PostProcessing: the finished image (bloom,
//                       tonemapping, grading, vignette, DOF, fog).
enum class EPostProcessSlot { BeforeTransparent, AfterTransparent };

// Suggested orders within AfterTransparent, low runs first. HDR effects go
// before tonemapping, display-space effects after it.
namespace EPostProcessOrder
{
    // BeforeTransparent slot
    inline constexpr int AmbientOcclusion = 100;

    // AfterTransparent slot
    inline constexpr int Fog          = 100;
    inline constexpr int DepthOfField = 200;
    inline constexpr int Bloom        = 300;
    inline constexpr int Tonemapping  = 500;   // HDR -> 0..1 display range
    inline constexpr int ColorGrading = 600;
    inline constexpr int FXAA         = 700;   // after colour work, before screen-space overlays
    inline constexpr int Vignette     = 800;
}

// Everything an effect gets for one frame. The stage has already bound
// `target` and set the viewport; draw a fullscreen quad that reads `source`.
struct SPostProcessContext
{
    IRenderPipeline*  pipeline      = nullptr;
    MBufferRegistery* registry      = nullptr;   // BUFFER_DEPTH / NORMAL / SURFACE
    const SViewData*  view          = nullptr;   // camera matrices
    SVector2          resolution    = { 0.f, 0.f };
    unsigned int      sourceTexture = 0;         // current HDR image - read this
    SFrameBuffer*     target        = nullptr;   // bound for writing
    const SFullscreenQuad* quad     = nullptr;
};

// Created by MPostProcessStack (one per effect type per pipeline), which
// owns it and its settings. Register each concrete effect with
// REGISTER_POST_EFFECT so it can be added by name (scene loading, inspector).
class MPostProcessEffect : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MPostProcessEffect)
public:
    ~MPostProcessEffect() override = default;

    [[nodiscard]] virtual EPostProcessSlot getSlot()  const = 0;
    [[nodiscard]] virtual int              getOrder() const = 0;

    virtual SPostProcessEffectSettings&       getSettings()       = 0;
    virtual const SPostProcessEffectSettings& getSettings() const = 0;

    // GPU setup / teardown. init runs when the effect is added.
    virtual void init(IRenderPipeline* pipeline) {}
    virtual void cleanup() {}
    virtual void resize(int width, int height) {}

    // Draw into ctx.target. Return false if nothing was written (the stage
    // then keeps the previous image as the input to the next effect).
    virtual bool apply(const SPostProcessContext& ctx) = 0;

    // Registered type name ("Tonemapping"), set by the stack on creation.
    [[nodiscard]] const SString& getTypeName() const { return typeName; }
    void setTypeName(const SString& name) { typeName = name; }

protected:
    // Fullscreen helpers for the common case: bind `shader`, set identity
    // transforms, bind ctx.sourceTexture to unit 0 as "_source", and return
    // the GL program id for setting further uniforms. Then drawFullscreen().
    static unsigned int beginFullscreen(MShader* shader, const SPostProcessContext& ctx);

    // Binds `shader` with identity transforms (for a fullscreen quad) and
    // returns the program id. For passes that read something other than
    // ctx.sourceTexture (e.g. bloom's mip chain).
    static unsigned int bindFullscreenShader(MShader* shader);
    static void         drawFullscreen(const SPostProcessContext& ctx);

    // Loads an engine shader asset; logs and returns nullptr on failure.
    static MShader* loadShader(const char* path);

private:
    SString typeName;
};

// Gives each effect its own settings type:
//   class MVignetteEffect : public TPostProcessEffect<SVignetteSettings> { ... };
//   stack.getSettings<MVignetteEffect>() -> SVignetteSettings*
template <class TSettings>
class TPostProcessEffect : public MPostProcessEffect
{
public:
    using Settings = TSettings;

    SPostProcessEffectSettings&       getSettings()       override { return settings; }
    const SPostProcessEffectSettings& getSettings() const override { return settings; }

    TSettings&       settingsData()       { return settings; }
    const TSettings& settingsData() const { return settings; }

protected:
    TSettings settings;
};

#endif // POST_PROCESS_EFFECT_H
