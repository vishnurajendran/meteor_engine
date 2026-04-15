//
// Created by ssj5v on 15-04-2026.
//

#pragma once
#ifndef SKYBOX_STAGE_H
#define SKYBOX_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

// Skybox stage — runs just before transparents (RS_Transparent - 1 = 2999).
//
// Iterates MSkyboxQueue and calls draw() on each registered MSkyboxDrawCall.
// The draw call owns all skybox-specific GL state (shader bind, cubemap bind,
// glDepthMask(false), GL_LEQUAL), so this stage stays thin.
//
// The skybox does NOT produce an SRenderItem and does NOT go through
// MRenderQueue — its shader and cubemap binding don't map to the standard
// material/VAO contract.
class MSkyboxStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Transparent - 1; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    sf::RenderTarget* renderTarget = nullptr;
};

#endif // SKYBOX_STAGE_H