//
// ui_render_stage.h
//
// MUIRenderStage - draws the main viewport's UI as the pipeline's last stage:
// after composite (the image is tonemapped and sRGB-encoded, which is what
// RmlUi's sRGB colours and premultiplied blending expect) and after the
// editor's gizmos, so a HUD is never covered by them.
//
// Added only to pipelines that show a viewport: the player window and the
// editor's scene pipeline. Thumbnails and other manual pipelines never draw UI.
//

#ifndef UI_RENDER_STAGE_H
#define UI_RENDER_STAGE_H

#include "core/graphics/core/render-pipeline/stages/render_stage.h"

class MUIRenderStage : public MRenderStage
{
public:
    // Gizmos are RS_Composite + 500.
    int  getSortingOrder() override { return ERenderStageOrder::RS_Composite + 600; }

    void init      (IRenderPipeline* const pipeline) override {}
    void cleanup   (IRenderPipeline* const pipeline) override {}
    void preRender (IRenderPipeline* const pipeline) override {}
    void render    (IRenderPipeline* const pipeline) override;
    void postRender(IRenderPipeline* const pipeline) override {}
};

#endif // UI_RENDER_STAGE_H
