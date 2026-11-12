//
// Depth of field - blurs what is nearer or farther than the focus distance.
//

#pragma once
#ifndef DEPTH_OF_FIELD_EFFECT_H
#define DEPTH_OF_FIELD_EFFECT_H

#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"

class SDepthOfFieldSettings : public SPostProcessEffectSettings
{
    DECLARE_FIELD(focusDistance, float, 10.0f)   // world units from the camera that stay sharp
    DECLARE_FIELD(focusRange,    float, 3.0f)    // distance over which blur ramps to full
    DECLARE_FIELD(bokehRadius,   float, 4.0f)    // max blur radius, half-resolution pixels
    DECLARE_FIELD(showFocus,     bool,  false)   // debug: tint near/far instead of blurring

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

// Bokeh depth of field (gather approach, as in Unity's post-processing stack):
//   1. CoC     - full res: circle of confusion per pixel from BUFFER_DEPTH,
//                signed (negative = in front of focus, positive = behind).
//   2. Prefilter - half res colour + CoC (the CoC of largest magnitude among
//                the four source pixels, colour weighted against fireflies).
//   3. Bokeh   - half res disc gather. Background and foreground are summed
//                separately so a blurry foreground can spill over sharp
//                pixels behind it, but sharp foreground never smears outward.
//   4. Postfilter - small tent blur to hide the disc's sampling pattern.
//   5. Combine - full res: sharp image where CoC is small, blurred elsewhere.
//
// Depth comes from BUFFER_DEPTH: opaque geometry, plus transparents whose
// material has Write Depth on. Other transparents take the blur of whatever
// is behind them.
class MDepthOfFieldEffect : public TPostProcessEffect<SDepthOfFieldSettings>
{
    DEFINE_OBJECT_SUBCLASS(MDepthOfFieldEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::AfterTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::DepthOfField; }

    void init(IRenderPipeline* pipeline) override;
    void cleanup() override;
    void resize(int width, int height) override;
    bool apply(const SPostProcessContext& ctx) override;

private:
    struct STarget { unsigned int texture = 0; int width = 0; int height = 0; };

    void destroyTargets();
    void drawInto(const STarget& target) const;

    STarget coc;     // full res, r = signed CoC in half-res pixels
    STarget halfA;   // half res ping
    STarget halfB;   // half res pong
    unsigned int fbo = 0;
    int builtWidth  = 0;
    int builtHeight = 0;

    MShader* cocShader        = nullptr;   // owned by the asset manager
    MShader* prefilterShader  = nullptr;
    MShader* bokehShader      = nullptr;
    MShader* postfilterShader = nullptr;
    MShader* combineShader    = nullptr;
};

#endif // DEPTH_OF_FIELD_EFFECT_H
