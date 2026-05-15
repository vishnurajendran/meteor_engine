//
// Clear stage — RS_Clear (500).
//
// Clears BUFFER_OPAQUE colour and depth before any rendering.
// Runs after depth/shadow (which use their own buffers) and before
// skybox and opaque geometry, so each stage only does its own job.
//

#pragma once
#ifndef CLEAR_STAGE_H
#define CLEAR_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

class MClearStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Clear; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;
};

#endif // CLEAR_STAGE_H