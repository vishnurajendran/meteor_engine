#pragma once
#ifndef COMPOSITE_STAGE_H
#define COMPOSITE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

// Which buffer the scene view is visualising.
enum class EBufferDebugView
{
    Final = 0,      // normal composite output
    Opaque,         // BUFFER_OPAQUE color (unlit albedo)
    Lighting,       // BUFFER_LIGHTS color (light contribution)
    Depth,          // BUFFER_OPAQUE depth (linearised grayscale)
    LightMask,      // BUFFER_LIGHTS alpha (1 = lit, 0 = unlit)
    COUNT
};

class MCompositeStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Composite; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

    // Set from the scene view dropdown. Default = Final.
    static EBufferDebugView debugView;

private:
    void compositeOpaqueWithLights(IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;
    void compositeOpaqueOnly      (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;
    void compositeDepth           (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;

    // Debug buffer visualisation — fullscreen quad with inline shader.
    void renderDebugView(IRenderPipeline* const pipeline,
                         int sfmlFBO, int w, int h) const;

    void initFullscreenQuad();
    void destroyFullscreenQuad();
    void buildDebugShader();

    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int quadEBO = 0;

    // Debug visualisation shader (built once in init).
    unsigned int debugShader   = 0;
    int          dbgLocTex     = -1;
    int          dbgLocMode    = -1;   // 0=color, 1=depth, 2=alpha
    int          dbgLocNearFar = -1;
};

#endif