//
// FXAA - fast approximate anti-aliasing, in screen space.
//

#pragma once
#ifndef FXAA_EFFECT_H
#define FXAA_EFFECT_H

#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"

class SFXAASettings : public SPostProcessEffectSettings
{
    // Edges darker than this absolute luma contrast are skipped (cheap pass on flat areas).
    DECLARE_FIELD(contrastThreshold, float, 0.0312f)
    // ...or than this fraction of the local brightness (stops over-smoothing bright areas).
    DECLARE_FIELD(relativeThreshold, float, 0.063f)
    // How much single-pixel detail (thin lines, specks) is softened.
    DECLARE_FIELD(subpixelBlending,  float, 0.75f)

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

// FXAA (Timothy Lottes, 3.11 quality algorithm): for each pixel, measure luma
// contrast against its neighbours, find the edge direction, walk along the
// edge to both ends, and resample with a sub-pixel offset across it.
// One full-screen pass, no extra buffers. Runs near the end of the
// post-process chain (EPostProcessOrder::FXAA) so it smooths the final edges,
// including those produced by earlier effects.
class MFXAAEffect : public TPostProcessEffect<SFXAASettings>
{
    DEFINE_OBJECT_SUBCLASS(MFXAAEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::AfterTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::FXAA; }

    void init(IRenderPipeline* pipeline) override;
    bool apply(const SPostProcessContext& ctx) override;

private:
    MShader* shader = nullptr;   // owned by the asset manager
};

#endif // FXAA_EFFECT_H
