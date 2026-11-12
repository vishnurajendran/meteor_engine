#pragma once
#ifndef COMPOSITE_STAGE_H
#define COMPOSITE_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

// Which buffer the scene view is visualising.
enum class EBufferDebugView
{
    Final = 0,      // normal composite output
    Scene,          // BUFFER_SCENE: lit HDR image incl. transparents (before encode)
    Opaque,         // BUFFER_OPAQUE color (unlit albedo)
    Lighting,       // BUFFER_LIGHTS color (light contribution)
    Depth,          // BUFFER_DEPTH (contrast-boosted grayscale)
    LightMask,      // BUFFER_LIGHTS alpha (1 = lit, 0 = unlit)
    Normals,        // BUFFER_NORMAL view-space normals (remapped to 0..1)
    Surface,        // BUFFER_SURFACE: r metallic, g smoothness, b lit
    Emission,       // BUFFER_EMISSION: material emission (added after lighting)
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
    // fromScene: encode BUFFER_SCENE (resolve ran). Otherwise combine
    // BUFFER_OPAQUE x BUFFER_LIGHTS directly (pipelines without a resolve stage).
    void compositeOpaqueWithLights(IRenderPipeline* const pipeline, bool useLights, bool fromScene,
                                   int sfmlFBO, int w, int h) const;
    void compositeOpaqueOnly      (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;
    // Copies the prepass depth into the render target so later stages
    // (editor gizmos) depth-test against the scene.
    void compositeDepth           (IRenderPipeline* const pipeline,
                                   int sfmlFBO, int w, int h) const;

    // Debug buffer visualisation - fullscreen quad with inline shader.
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
    int          dbgLocMode    = -1;   // 0=color, 1=depth, 2=alpha, 3=normals, 4=raw data
    int          dbgLocNearFar = -1;
};

#endif