//
// Created by ssj5v on 22-03-2026.
//

#ifndef RENERER_H
#define RENERER_H
#include <memory>

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

    MRenderQueue&           getRenderQueue() override           { return *renderQueue; }
    MSkyboxQueue&           getSkyboxQueue() override           { return *skyboxQueue; }
    MProceduralSkyboxQueue& getProceduralSkyboxQueue() override { return *proceduralSkyboxQueue; }
    MLightSystemManager&    getLightSystem() override           { return *lightSystem; }

private:
    IRenderPipeline* pipeline;

    std::unique_ptr<MRenderQueue>           renderQueue;
    std::unique_ptr<MSkyboxQueue>           skyboxQueue;
    std::unique_ptr<MProceduralSkyboxQueue> proceduralSkyboxQueue;
    std::unique_ptr<MLightSystemManager>    lightSystem;
};
#endif //RENERER_H
