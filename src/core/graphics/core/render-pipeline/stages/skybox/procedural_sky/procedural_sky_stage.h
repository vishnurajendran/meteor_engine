//
// MProceduralSkyboxStage
// Mirrors MSkyboxStage exactly — renders into BUFFER_OPAQUE so the sky sits
// in the same colour buffer as opaque geometry.
// MCompositeStage blits the finished BUFFER_OPAQUE to the render target.
//

#pragma once
#ifndef PROCEDURAL_SKYBOX_STAGE_H
#define PROCEDURAL_SKYBOX_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

class MProceduralSkyboxStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Transparent - 1; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override {}

private:
    SFrameBuffer* opaqueBuffer = nullptr;
};

#endif // PROCEDURAL_SKYBOX_STAGE_H