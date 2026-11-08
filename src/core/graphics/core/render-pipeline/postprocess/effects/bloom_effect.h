//
// Bloom - bright parts of the HDR image bleed light into their surroundings.
//

#pragma once
#ifndef BLOOM_EFFECT_H
#define BLOOM_EFFECT_H

#include <vector>
#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"

class SBloomSettings : public SPostProcessEffectSettings
{
    DECLARE_FIELD(intensity, float, 0.5f)   // how much bloom is added back
    DECLARE_FIELD(threshold, float, 1.0f)   // linear brightness where bloom starts (1 = white)
    DECLARE_FIELD(softKnee,  float, 0.5f)   // 0 = hard cut at threshold, 1 = gradual
    DECLARE_FIELD(scatter,   float, 0.7f)   // how far the glow spreads

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

// Classic HDR bloom (as in Call of Duty: Advanced Warfare / Unity URP):
//   1. Prefilter: keep only light above the threshold (soft knee), at half
//      resolution, with a 13-tap filter and Karis averaging so single
//      very bright pixels don't flicker.
//   2. Downsample: halve repeatedly (up to 6 levels), 13-tap filter each.
//   3. Upsample: from the smallest level back up, a tent filter each step,
//      added onto the next larger level (scatter scales each step).
//   4. Composite: scene + bloom * intensity.
// Runs in the HDR part of the frame (before tonemapping), so material
// emission with intensity > 1 and bright lights both bloom.
class MBloomEffect : public TPostProcessEffect<SBloomSettings>
{
    DEFINE_OBJECT_SUBCLASS(MBloomEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::AfterTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::Bloom; }

    void init(IRenderPipeline* pipeline) override;
    void cleanup() override;
    void resize(int width, int height) override;
    bool apply(const SPostProcessContext& ctx) override;

private:
    struct SMip { unsigned int texture = 0; int width = 0; int height = 0; };

    void destroyMips();
    void drawInto(const SMip& mip) const;

    static constexpr int MAX_MIPS = 6;

    std::vector<SMip> mips;
    unsigned int fbo = 0;
    int builtWidth  = 0;
    int builtHeight = 0;

    MShader* downsampleShader = nullptr;   // owned by the asset manager
    MShader* upsampleShader   = nullptr;
    MShader* compositeShader  = nullptr;
};

#endif // BLOOM_EFFECT_H
