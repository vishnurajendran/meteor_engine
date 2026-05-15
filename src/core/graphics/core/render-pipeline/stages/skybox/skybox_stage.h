//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef SKYBOX_STAGE_H
#define SKYBOX_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

// Skybox stage — RS_Skybox (750).
//
// Renders into BUFFER_OPAQUE after MClearStage clears the buffer but before
// opaque geometry. The skybox fills the background; geometry overwrites it
// via depth testing (GL_LESS). Skybox depth stays at the far plane so the
// lighting stage's depth blit preserves correct geometry depth.
class MSkyboxStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Skybox; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SFrameBuffer* opaqueBuffer = nullptr;
};

#endif // SKYBOX_STAGE_H