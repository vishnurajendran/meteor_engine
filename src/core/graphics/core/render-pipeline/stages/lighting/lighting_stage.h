//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef LIGHTING_STAGE_H
#define LIGHTING_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

// Lighting stage - RS_Lighting (4000).
//
// A single fullscreen pass - it does not iterate meshes. lighting_pass.mesl
// reads the prepass buffers per pixel:
//   BUFFER_DEPTH   -> world position (reconstructed with the frame's camera)
//   BUFFER_NORMAL  -> shading normal (normal map already applied)
//   BUFFER_SURFACE -> metallic, smoothness, lit/unlit
// and writes the raw light intensity (no albedo) into BUFFER_LIGHTS, with
// alpha = 1 on lit pixels and 0 on unlit pixels and sky.
//
// Point/spot lights come from one list uploaded per frame for the whole
// view (MLightSystemManager::prepareViewDynamicLights).
//
// MCompositeStage multiplies BUFFER_OPAQUE × BUFFER_LIGHTS to produce the
// final lit image:  albedo × (ambient + directional + spot/point) = result.
class MLightingStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MLightingStage)
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Lighting; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    void initFullscreenQuad();
    void destroyFullscreenQuad();

    SFrameBuffer* lightsBuffer   = nullptr;
    // No cached MShader* - looked up fresh each render() to survive refresh().
    SString       lightingShaderPath;

    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int quadEBO = 0;

    bool warnedMissingInputs = false;
};

#endif // LIGHTING_STAGE_H
