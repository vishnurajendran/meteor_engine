//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef SKYBOX_STAGE_H
#define SKYBOX_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

// Skybox stage — runs just before transparents (RS_Transparent - 1 = 2999).
//
// Renders into BUFFER_OPAQUE so it sits in the same colour buffer as the
// opaque geometry.  The skybox draw call uses GL_LEQUAL depth so it only
// fills pixels where no geometry was drawn.
// MCompositeStage blits the finished BUFFER_OPAQUE to the render target.
class MSkyboxStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Transparent - 1; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SFrameBuffer* opaqueBuffer = nullptr;
};

#endif // SKYBOX_STAGE_H