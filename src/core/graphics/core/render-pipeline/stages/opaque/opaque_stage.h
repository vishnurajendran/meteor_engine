//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef OPAQUE_STAGE_H
#define OPAQUE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;
class MShader;

// Opaque geometry stage — RS_Opaque (1000).
//
// Renders all SRenderItems into BUFFER_OPAQUE using albedo_pass.mesl as a
// shader override — outputs texture * color only, no lighting math.
// MLightingStage renders the same geometry separately into BUFFER_LIGHTS with
// the light contribution, and MCompositeStage multiplies them together.
class MOpaqueStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Opaque; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SFrameBuffer* opaqueBuffer  = nullptr;
    MShader*      albedoShader  = nullptr; // owned by MShaderAsset
};

#endif // OPAQUE_STAGE_H