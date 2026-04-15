//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef DEPTH_RENDER_STAGE_H
#define DEPTH_RENDER_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/utils/glmhelper.h"

class MShader;

// Depth-only prepass stage (RS_Depth = 0).
//
// Renders all scene geometry into a dedicated depth FBO.
// The shader is loaded from "meteor_assets/engine_assets/shaders/depth.mesl"
// so no GLSL lives in engine source code.
class MDepthRenderStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Depth; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    SDepthBuffer* depthBuffer  = nullptr;
    MShader*      depthShader  = nullptr;   // owned by MShaderAsset, not by this stage
    SVector2      resolution   = { 0.f, 0.f };
};

#endif // DEPTH_RENDER_STAGE_H