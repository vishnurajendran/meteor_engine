//
// Prepass stage - RS_Prepass (100), right after the clear stage.
//

#pragma once
#ifndef PREPASS_STAGE_H
#define PREPASS_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class MShader;
class SDepthBuffer;
class STextureBuffer;
struct SRenderItem;

// The only camera-view pass that needs per-material lighting inputs.
// Draws every camera-visible opaque item once with prepass.mesl and writes
// three registry buffers through one MRT framebuffer this stage owns:
//
//   BUFFER_DEPTH   (SDepthBuffer)    hardware depth. Shared as BUFFER_OPAQUE's
//                                    depth attachment, so opaque shades each
//                                    pixel once (GL_LEQUAL).
//   BUFFER_NORMAL  (STextureBuffer)  output 0, RGBA16F
//                                      rgb = view-space normal, normal map applied
//                                      a   = 1 where geometry was drawn, else 0
//   BUFFER_SURFACE (STextureBuffer)  output 1, RGBA8
//                                      r = metallic, g = smoothness,
//                                      b = 1 lit / 0 unlit, a = 1 where drawn
//
// Everything after this reads those buffers instead of iterating meshes:
// MLightingStage is a fullscreen pass over them, and post-processing
// (SSAO, outlines, SSR, ...) can do the same.
class MPrepassStage : public MRenderStage
{
    DEFINE_OBJECT_SUBCLASS(MPrepassStage)
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Prepass; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;

private:
    // (Re)attaches the three buffers' textures when any of them changed
    // (first frame, or after a resize). Returns false if the FBO is unusable.
    bool syncFramebuffer();

    // Forwards the item's material values (normal map, metallic, smoothness)
    // to the bound prepass shader. Returns the next free texture unit.
    int  bindMaterialInputs(const SRenderItem& item, unsigned int program);

    SDepthBuffer*   depthBuffer   = nullptr;   // owned by the buffer registry
    STextureBuffer* normalBuffer  = nullptr;
    STextureBuffer* surfaceBuffer = nullptr;

    unsigned int fbo               = 0;        // owned by this stage
    // Storage ids (SBuffer::getStorageId) currently attached - not GL names,
    // which can repeat after a resize.
    unsigned long long attachedDepth   = 0;
    unsigned long long attachedNormal  = 0;
    unsigned long long attachedSurface = 0;

    MShader* prepassShader = nullptr;          // owned by MShaderAsset
};

#endif // PREPASS_STAGE_H
