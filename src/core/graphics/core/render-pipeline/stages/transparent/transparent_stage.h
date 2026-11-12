//
// Transparent stage - RS_Transparent (4500).
//

#pragma once
#ifndef TRANSPARENT_STAGE_H
#define TRANSPARENT_STAGE_H

#include <vector>
#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class SFrameBuffer;
struct SRenderItem;

// Draws pipeline->getTransparentItems() into BUFFER_SCENE, after
// MResolveStage has written the lit opaque image there.
//
//   - Sorted back to front by distance from the camera, every frame.
//   - Forward lit: each material's shader calls meteorSurfaceLighting()
//     (meteor_lighting_utils.glsl) with this frame's shadow maps
//     (MShadowInputs) and the point/spot lights near that mesh
//     (MLightSystemManager::prepareDynamicLights, BVH query).
//   - Depth-tested against the prepass depth (BUFFER_SCENE shares it). By
//     default never writes depth, so transparents don't hide each other or
//     what is behind them. Materials with Write Depth on do write it, for
//     post effects in the RS_PostProcessing slot (runs after this stage).
//   - Blend per material: Alpha or Additive. Double-sided materials draw
//     back faces too.
//
// Must be added after MResolveStage (it looks up BUFFER_SCENE in init()).
class MTransparentStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MTransparentStage)
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Transparent; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    // Forwards the item's material properties to the bound program.
    // Textures go on units from MShadowInputs::UNIT_COUNT up.
    // Returns the highest texture unit used, or -1.
    int bindMaterialProperties(const SRenderItem& item, unsigned int program) const;

    SFrameBuffer* sceneBuffer = nullptr;   // owned by the buffer registry

    // Reused every frame to avoid reallocating.
    std::vector<const SRenderItem*> sorted;
};

#endif // TRANSPARENT_STAGE_H
