//
// Gizmo stage — RS_Gizmo (8500).
//
// Runs after MCompositeStage. Blits scene depth from BUFFER_OPAQUE into the
// render target so icon gizmos can depth-test against geometry, then draws
// all gizmos with alpha blending enabled. Lines and overlays render without
// depth testing (drawn on top); icon gizmos that want depth testing enable
// it internally.
//

#pragma once
#ifndef GIZMO_STAGE_H
#define GIZMO_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class MGizmoStage : public MRenderStage
{
public:
    int  getSortingOrder() override { return ERenderStageOrder::RS_Composite + 500; }

    void init      (IRenderPipeline* const pipeline) override;
    void cleanup   (IRenderPipeline* const pipeline) override;
    void preRender (IRenderPipeline* const pipeline) override;
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override;
};

#endif // GIZMO_STAGE_H