#pragma once
#ifndef SHADOW_STAGE_H
#define SHADOW_STAGE_H

#include <glm/mat4x4.hpp>
#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SShadowBuffer;
struct SFrustum;
class MShader;
class MLightEntity;

class MShadowStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MShadowStage)
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
    // cullFrustum — the light's own frustum; items whose bounds are entirely
    // outside it are skipped. nullptr draws every caster.
    void drawItems(IRenderPipeline* const pipeline, MShader* shader,
                   unsigned int* rawProg, bool shadowCastersOnly,
                   const SFrustum* cullFrustum = nullptr);

    // For the inline point shadow program - sets model via raw GL uniform.
    void drawItemsRaw(IRenderPipeline* const pipeline,
                      unsigned int prog, bool shadowCastersOnly,
                      const SFrustum* cullFrustum = nullptr);

    SShadowBuffer* shadowBuffer       = nullptr;
    MShader*       shadowShader       = nullptr;
    unsigned int   pointShadowProgram = 0; // inline GLSL, writes linear depth
};

#endif