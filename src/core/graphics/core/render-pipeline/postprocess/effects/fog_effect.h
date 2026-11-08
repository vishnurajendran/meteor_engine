//
// Fog - fades distant surfaces towards a fog colour.
//

#pragma once
#ifndef FOG_EFFECT_H
#define FOG_EFFECT_H

#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"
#include "core/utils/field_engine_types.h"   // Field<SVector3> save/load

class SFogSettings : public SPostProcessEffectSettings
{
    DECLARE_FIELD(mode,           int,      1)                             // 0 Linear, 1 Exponential, 2 Exponential Squared
    DECLARE_FIELD(color,          SVector3, SVector3(0.62f, 0.68f, 0.75f)) // linear RGB
    DECLARE_FIELD(colorIntensity, float,    1.0f)                          // HDR multiplier on color
    DECLARE_FIELD(density,        float,    0.02f)                         // exponential modes
    DECLARE_FIELD(startDistance,  float,    0.0f)                          // world units with no fog
    DECLARE_FIELD(endDistance,    float,    100.0f)                        // linear mode: fully fogged here
    DECLARE_FIELD(maxOpacity,     float,    1.0f)                          // cap, so far geometry stays visible
    DECLARE_FIELD(skyAmount,      float,    0.0f)                          // how much fog covers the sky

    // Volumetric: sunlight scattered by the fog, shadowed per sample -> god rays.
    // Its own medium, independent of mode / density / maxOpacity above.
    DECLARE_FIELD(volumetric,     bool,     true)
    DECLARE_FIELD(volumetricDensity, float, 0.03f)                         // how thick the sunlit air is
    DECLARE_FIELD(sunScattering,  float,    0.3f)                          // brightness of the scattered sun light
    DECLARE_FIELD(anisotropy,     float,    0.6f)                          // 0 even, toward 1 = brighter toward the sun
    DECLARE_FIELD(steps,          int,      32)                            // samples per ray
    DECLARE_FIELD(marchDistance,  float,    150.0f)                        // longest ray marched (sky, far geometry)
    DECLARE_FIELD(showVolumetric, bool,     false)                         // debug: only the scattered light

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

// Distance fog applied as one fullscreen pass in HDR, before depth of field,
// bloom and tonemapping:
//     dist   = distance from the camera, rebuilt from BUFFER_DEPTH
//     amount = f(dist)  (linear / exp / exp2), capped by maxOpacity
//     out    = mix(scene, fogColor, amount)
//
// Volumetric adds the directional light scattered toward the camera by a
// separate uniform medium (volumetricDensity): the ray is marched to the
// surface, and each step's share is lit by the sun unless the step is in
// shadow (directional cascades). Shadowed air
// stays dark, so trees and buildings cast visible light shafts. Shafts
// appear within the directional light's shadow distance; past it the fog is
// evenly sunlit.
//
// Uses the true distance to the camera rather than view-space depth, so fog
// doesn't creep in at the screen edges as the camera turns.
// Pixels with no depth (cleared to the far plane) are the sky and get
// skyAmount instead. Like depth of field, transparents that don't write
// depth take the fog of whatever is behind them.
class MFogEffect : public TPostProcessEffect<SFogSettings>
{
    DEFINE_OBJECT_SUBCLASS(MFogEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::AfterTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::Fog; }

    void init(IRenderPipeline* pipeline) override;
    bool apply(const SPostProcessContext& ctx) override;

private:
    MShader* shader = nullptr;   // owned by the asset manager
};

#endif // FOG_EFFECT_H
