//
// Post-process stages - run MPostProcessStack effects on BUFFER_SCENE.
//

#pragma once
#ifndef POST_PROCESS_STAGE_H
#define POST_PROCESS_STAGE_H

#include "core/graphics/core/render-pipeline/fullscreen_quad.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_effect.h"
#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class MPostProcessStack;
class SFrameBuffer;

// Runs the stack's enabled effects for one slot, in order. Effects chain
// through two HDR ping-pong buffers (BUFFER_POST_A / B): each reads the
// previous result and writes the other buffer. The final result is copied
// back into BUFFER_SCENE, so later stages (transparent, composite) see it.
//
// Two concrete stages, one per slot:
//   MPostProcessOpaqueStage - RS_PostOpaque,     EPostProcessSlot::BeforeTransparent
//   MPostProcessStage       - RS_PostProcessing, EPostProcessSlot::AfterTransparent
// Both need BUFFER_SCENE (add after MResolveStage) and the pipeline's
// MPostProcessStack attachment (pipelines without one simply skip post).
class MPostProcessStageBase : public MRenderStage
{
public:
    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override {}
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override {}

protected:
    [[nodiscard]] virtual EPostProcessSlot getSlot() const = 0;

private:
    MPostProcessStack* stack       = nullptr;   // pipeline attachment
    SFrameBuffer*      sceneBuffer = nullptr;   // owned by the buffer registry
    SFrameBuffer*      pingPong[2] = { nullptr, nullptr };
    SFullscreenQuad    quad;
};

class MPostProcessOpaqueStage : public MPostProcessStageBase
{
    DEFINE_OBJECT_SUBCLASS(MPostProcessOpaqueStage)
public:
    int getSortingOrder() override { return ERenderStageOrder::RS_PostOpaque; }
protected:
    EPostProcessSlot getSlot() const override { return EPostProcessSlot::BeforeTransparent; }
};

class MPostProcessStage : public MPostProcessStageBase
{
    DEFINE_OBJECT_SUBCLASS(MPostProcessStage)
public:
    int getSortingOrder() override { return ERenderStageOrder::RS_PostProcessing; }
protected:
    EPostProcessSlot getSlot() const override { return EPostProcessSlot::AfterTransparent; }
};

#endif // POST_PROCESS_STAGE_H
