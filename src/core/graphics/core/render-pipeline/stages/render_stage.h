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
    RS_Transparent   = 3000,    // transparent geometry

    RS_Lighting      = 4000,    // fullscreen lighting from prepass buffers

    RS_UserInterface = 5000,    // UI

    RS_Composite     = 8000,    // composite all flagged buffers into render target
    RS_PostProcessing = 9999,   // post-process on the composited result
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