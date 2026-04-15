//
// Created by Vishnu Rajendran on 2024-09-25.
//

#pragma once
#ifndef METEOR_ENGINE_GRAPHICSRENDERER_H
#define METEOR_ENGINE_GRAPHICSRENDERER_H

#include <GL/glew.h>
#include "core/graphics/core/render-pipeline/stages/render_stage.h"
#include "core/meteor_core_minimal.h"

// Opaque geometry stage — RS_Opaque (order 1000).
//
// Iterates the pipeline's SRenderItems, calls material->bindMaterial() on
// each, then sets u_model and issues the draw.  Items without a material
// are skipped (they may be handled by a specialised stage).
//
// IGraphicRenderer and MDrawCall are no longer part of the active path.
// They remain in the codebase for now as legacy stubs until all drawables
// have been migrated to IMeteorDrawable::submitRenderItem().
class MGraphicsRenderer : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Opaque; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    // Sets u_model on the currently bound shader program.
    void setModelUniform(const glm::mat4& model) const;

private:
    sf::RenderTarget* renderTarget = nullptr;
    GLint             uModel       = -1;

public:
    bool drawGizmos = true;
};

#endif // METEOR_ENGINE_GRAPHICSRENDERER_H