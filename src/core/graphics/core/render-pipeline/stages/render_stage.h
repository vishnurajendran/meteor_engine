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
    RS_Depth         = 0,       // depth prepass
    RS_Shadow        = 1,       // shadow maps

    RS_Opaque        = 1000,    // opaque geometry
    RS_Transparent   = 3000,    // transparent geometry

    RS_Lighting      = 4000,    // lighting accumulation

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