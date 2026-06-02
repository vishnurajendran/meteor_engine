//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef OPAQUE_STAGE_H
#define OPAQUE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;

// Opaque geometry stage -- RS_Opaque (1000).
//
// Renders all SRenderItems into BUFFER_OPAQUE using each item's own material
// via bindMaterial().
//
// Supports a pipeline-level light override: when active, writes ambient and
// directional light data to private UBOs instead of calling prepareLights(),
// and skips dynamic light preparation per item.
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
    // Writes override ambient + directional light data to UBOs at the
    // standard binding indices, bypassing MLightSystemManager entirely.
    void applyLightOverride(const IRenderPipeline::SLightOverride& lo);
    void destroyOverrideUBOs();

    SFrameBuffer* opaqueBuffer = nullptr;

    // UBO handles for light override -- created on first use, reused after.
    unsigned int overrideAmbientUBO     = 0;
    unsigned int overrideDirectionalUBO = 0;
};

#endif // OPAQUE_STAGE_H