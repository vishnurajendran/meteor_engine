//
// Vignette - darkens the edges of the image.
//

#pragma once
#ifndef VIGNETTE_EFFECT_H
#define VIGNETTE_EFFECT_H

#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"

class SVignetteSettings : public SPostProcessEffectSettings
{
    DECLARE_FIELD(intensity,  float, 0.3f)   // how dark the corners get, 0..1
    DECLARE_FIELD(smoothness, float, 0.5f)   // how far the falloff reaches in, 0..1

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

class MVignetteEffect : public TPostProcessEffect<SVignetteSettings>
{
    DEFINE_OBJECT_SUBCLASS(MVignetteEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::AfterTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::Vignette; }

    void init(IRenderPipeline* pipeline) override;
    bool apply(const SPostProcessContext& ctx) override;

private:
    MShader* shader = nullptr;   // owned by the asset manager
};

#endif // VIGNETTE_EFFECT_H
