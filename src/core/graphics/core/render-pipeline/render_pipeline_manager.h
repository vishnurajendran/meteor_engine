//
// Created by ssj5v on 22-03-2026.
//

#ifndef RENERER_H
#define RENERER_H

#include "render_pipeline.h"
#include "core/object/object.h"
class MRenderPipelineManager : public MObject {
public:
    MRenderPipelineManager();
    ~MRenderPipelineManager() override;

    void setRenderTarget(SRenderBuffer* renderBuffer);
    SVector2 getRenderResolution() { return pipeline.getRenderResolution(); }

    inline void initalise() { pipeline.init(); }
    void preRender() { pipeline.preRender(); }
    void render() { pipeline.render(); };
    void postRender() { pipeline.postRender(); }

    static MRenderPipelineManager* const getInstance() { return activeInstance; }
private:
    MRenderPipeline pipeline;
    static MRenderPipelineManager* activeInstance;
};
#endif //RENERER_H
