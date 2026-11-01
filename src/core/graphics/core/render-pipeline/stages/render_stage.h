//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef RENDER_STAGE_H
#define RENDER_STAGE_H

#include "core/graphics/core/render-pipeline/render_pipeline.h"
#include "core/object/object.h"

enum ERenderStageOrder
{
    RS_Clear         = 0,       // clear BUFFER_OPAQUE colour + depth - first stage of the frame
    RS_Prepass       = 100,     // depth/normal/surface prepass -> BUFFER_DEPTH, BUFFER_NORMAL, BUFFER_SURFACE
    RS_Shadow        = 200,     // shadow maps
    RS_Skybox        = 300,     // cubemap + procedural sky into BUFFER_OPAQUE, only where the prepass left the far plane

    RS_Opaque        = 1000,    // opaque geometry
    RS_Lighting      = 4000,    // fullscreen lighting from prepass buffers
    RS_Resolve       = 4100,    // albedo x light -> BUFFER_SCENE (linear HDR)

    // Post-processing runs in two slots, both on BUFFER_SCENE (linear HDR):
    //   RS_PostOpaque     - effects that must only see solid surfaces (SSAO,
    //                       SSR, contact shadows). BUFFER_DEPTH holds opaque
    //                       depth only.
    //   RS_PostProcessing - effects on the finished image (bloom, tonemapping,
    //                       grading, DOF, fog). BUFFER_DEPTH now also holds
    //                       transparents whose material has Write Depth on.
    RS_PostOpaque    = 4200,
    RS_Transparent   = 4500,    // forward-lit transparents, blended into BUFFER_SCENE
    RS_PostProcessing = 6000,

    RS_UserInterface = 7000,    // game UI - after post so it isn't bloomed/tonemapped

    RS_Composite     = 8000,    // encode BUFFER_SCENE to sRGB into the render target
};

class MRenderStage : public MObject, public IRenderStage
{
public:
    MRenderStage()          = default;
    ~MRenderStage() override = default;

    virtual int  getSortingOrder()                           = 0;
    virtual void init      (IRenderPipeline* const pipeline) = 0;
    virtual void cleanup   (IRenderPipeline* const pipeline) = 0;
    virtual void preRender (IRenderPipeline* const pipeline) = 0;
    virtual void render    (IRenderPipeline* const pipeline) = 0;
    virtual void postRender(IRenderPipeline* const pipeline) = 0;
};

#endif // RENDER_STAGE_H