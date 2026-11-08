//
// Resolve stage - RS_Resolve (4100).
//

#pragma once
#ifndef RESOLVE_STAGE_H
#define RESOLVE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/graphics/core/render-pipeline/fullscreen_quad.h"

class SFrameBuffer;

// Combines the deferred opaque result into one linear HDR image:
//
//   BUFFER_SCENE = albedo (BUFFER_OPAQUE) × light (BUFFER_LIGHTS) + emission
//                  (unlit pixels and sky: albedo as-is)
//
// BUFFER_SCENE shares BUFFER_DEPTH, so MTransparentStage can draw into it
// with depth testing, and post-processing (bloom, tonemapping, ...) can run
// on it before MCompositeStage encodes it to sRGB for the screen.
class MResolveStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MResolveStage)
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Resolve; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SFrameBuffer*   sceneBuffer = nullptr;   // owned by the buffer registry
    SFullscreenQuad quad;
};

#endif // RESOLVE_STAGE_H
