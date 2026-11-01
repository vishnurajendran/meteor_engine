//
// Clear stage - RS_Clear (0), the first stage of every frame.
//
// Clears BUFFER_OPAQUE colour and depth before anything else renders.
// When BUFFER_OPAQUE shares the prepass depth, that depth is cleared here
// too, which is safe because the prepass runs after this stage (and clears
// its own targets again, so it also works in pipelines without a clear stage).
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