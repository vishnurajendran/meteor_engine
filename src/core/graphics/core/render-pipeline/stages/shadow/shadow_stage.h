#pragma once
#ifndef SHADOW_STAGE_H
#define SHADOW_STAGE_H

#include <glm/mat4x4.hpp>
#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SShadowBuffer;
class MShader;
class MLightEntity;

class MShadowStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Shadow; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    void renderDirectionalShadow(IRenderPipeline* const pipeline, MLightEntity* dirLight);
    void renderSpotShadows      (IRenderPipeline* const pipeline);
    void renderPointShadows     (IRenderPipeline* const pipeline);

    // For mesl-based shaders (directional, spot).
    void drawItems(IRenderPipeline* const pipeline, MShader* shader,
                   unsigned int* rawProg, bool shadowCastersOnly);

    // For the inline point shadow program — sets model via raw GL uniform.
    void drawItemsRaw(IRenderPipeline* const pipeline,
                      unsigned int prog, bool shadowCastersOnly);

    SShadowBuffer* shadowBuffer       = nullptr;
    MShader*       shadowShader       = nullptr;
    unsigned int   pointShadowProgram = 0; // inline GLSL, writes linear depth
};

#endif