#pragma once
#ifndef COMPOSITE_STAGE_H
#define COMPOSITE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class MShader;

// Composite stage — RS_Composite (8000).
//
// Blends all flagged buffers and writes the final result directly to the
// SFML render target (its internal FBO).
// When MPostProcessStage is added later, this stage will instead write to
// BUFFER_COMPOSITE and MPostProcessStage will do the final SFML blit.
class MCompositeStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Composite; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    void compositeOpaqueWithLights(IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;
    void compositeOpaqueOnly      (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;
    void compositeDepth           (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;

    void initFullscreenQuad();
    void destroyFullscreenQuad();

    unsigned int quadVAO              = 0;
    unsigned int quadVBO              = 0;
    unsigned int quadEBO              = 0;
    MShader*     finalCompositeShader = nullptr;
};

#endif