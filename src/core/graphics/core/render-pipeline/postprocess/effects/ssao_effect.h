//
// SSAO - screen-space ambient occlusion: darkens creases, corners and
// contact points where surrounding geometry blocks ambient light.
//

#pragma once
#ifndef SSAO_EFFECT_H
#define SSAO_EFFECT_H

#include <glm/vec3.hpp>
#include <vector>
#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"

class SSSAOSettings : public SPostProcessEffectSettings
{
    DECLARE_FIELD(radius,    float, 0.5f)    // world units searched around each point
    DECLARE_FIELD(intensity, float, 1.0f)    // strength (power applied to the result)
    DECLARE_FIELD(bias,      float, 0.025f)  // depth tolerance against self-occlusion
    DECLARE_FIELD(showAO,    bool,  false)   // debug: show the occlusion term only

public:
    [[nodiscard]] SFieldMeta getFieldMeta(const std::string& fieldName) const override;
};

// Hemisphere SSAO (Crytek / LearnOpenGL style), in the BeforeTransparent
// slot so it only sees opaque surfaces (BUFFER_DEPTH and BUFFER_NORMAL
// hold opaque data there):
//   1. AO     - full res: rebuild the view-space position from BUFFER_DEPTH,
//               take SAMPLE_COUNT points in the hemisphere around the
//               BUFFER_NORMAL normal, count how many sit behind the depth
//               buffer. The kernel is rotated per pixel with interleaved
//               gradient noise, so no noise texture is needed.
//   2. Blur   - separable, depth-aware (horizontal, then vertical): removes
//               the noise without bleeding occlusion across object edges.
//   3. Apply  - scene colour × AO.
// Note: this darkens the final lit image, direct light included - a
// simplification over applying AO to ambient light only.
class MSSAOEffect : public TPostProcessEffect<SSSAOSettings>
{
    DEFINE_OBJECT_SUBCLASS(MSSAOEffect)
public:
    [[nodiscard]] EPostProcessSlot getSlot()  const override { return EPostProcessSlot::BeforeTransparent; }
    [[nodiscard]] int              getOrder() const override { return EPostProcessOrder::AmbientOcclusion; }

    void init(IRenderPipeline* pipeline) override;
    void cleanup() override;
    void resize(int width, int height) override;
    bool apply(const SPostProcessContext& ctx) override;

    static constexpr int SAMPLE_COUNT = 16;   // must match ssao.mesl

private:
    struct STarget { unsigned int texture = 0; int width = 0; int height = 0; };

    void destroyTargets();
    void drawInto(const STarget& target) const;

    STarget aoA;   // r = AO, g = linear depth (for the blur's edge test)
    STarget aoB;
    unsigned int fbo = 0;
    int builtWidth  = 0;
    int builtHeight = 0;

    std::vector<glm::vec3> kernel;   // hemisphere samples, z up, length <= 1

    MShader* aoShader    = nullptr;  // owned by the asset manager
    MShader* blurShader  = nullptr;
    MShader* applyShader = nullptr;
};

#endif // SSAO_EFFECT_H
