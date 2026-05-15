//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef OPAQUE_STAGE_H
#define OPAQUE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

// Opaque geometry stage — RS_Opaque (1000).
//
// Renders all SRenderItems into BUFFER_OPAQUE using each item's own material
// via bindMaterial(). Material shaders output albedo only — no lighting.
//
// The lighting stage writes a light mask into BUFFER_LIGHTS:
//   alpha = 0  →  unlit pixel (composite passes through opaque value)
//   alpha = 1  →  lit pixel   (composite multiplies opaque × light)
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
    SFrameBuffer* opaqueBuffer = nullptr;
};

#endif // OPAQUE_STAGE_H