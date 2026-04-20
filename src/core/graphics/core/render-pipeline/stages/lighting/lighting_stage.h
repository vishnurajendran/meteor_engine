//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef LIGHTING_STAGE_H
#define LIGHTING_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;
class MShader;

// Lighting stage — RS_Lighting (4000).
//
// Renders all scene geometry into BUFFER_LIGHTS using lighting_pass.mesl —
// outputs applyLighting(vec3(1.0)) — the raw light intensity with no albedo.
//
// MCompositeStage multiplies BUFFER_OPAQUE × BUFFER_LIGHTS to produce the
// final lit image:  albedo × (ambient + directional + spot/point) = result.
class MLightingStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Lighting; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SFrameBuffer* lightsBuffer   = nullptr;
    MShader*      lightingShader = nullptr; // owned by MShaderAsset
};

#endif // LIGHTING_STAGE_H