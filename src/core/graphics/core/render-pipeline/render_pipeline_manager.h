//
// Created by ssj5v on 22-03-2026.
//

#ifndef RENERER_H
#define RENERER_H
#include "core/object/object.h"
#include "render_pipeline.h"
#include "render_pipeline_manager_interface.h"

class MRenderPipelineManager : public MObject, public IRenderPipelineManagerSubsystem {
    DEFINE_OBJECT_SUBCLASS(MRenderPipelineManager)
public:
    MRenderPipelineManager();
    ~MRenderPipelineManager() override;

    void setRenderTarget(SRenderBuffer* renderBuffer) override;
    SVector2 getRenderResolution() override { return pipeline->getRenderResolution(); }

    void init() override { pipeline->init(); }
    void cleanup() override { pipeline->cleanup(); };

    void preRender() override { pipeline->preRender(); }
    void render() override { pipeline->render(); }
    void postRender() override { pipeline->postRender(); }

    IRenderPipeline* getPipeline() override { return pipeline; }

private:
    IRenderPipeline* pipeline;
};
#endif //RENERER_H
