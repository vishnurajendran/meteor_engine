//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef COMPOSITE_STAGE_H
#define COMPOSITE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

// Composite stage — RS_Composite (order 8000), runs after all render stages.
//
// Reads the pipeline's ECompositeFlags bitmask and blits each flagged buffer
// into the final render target.  This is the only place in the pipeline that
// knows how to combine stage outputs — the pipeline itself has no render logic.
//
// To add a new buffer to the composite pass:
//   1. Your stage sets addCompositeFlag(ECF_YourBuffer) after writing to it.
//   2. Add a case here that reads the buffer by name and composites it.
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
    void compositeDepth(IRenderPipeline* const pipeline) const;
};

#endif // COMPOSITE_STAGE_H