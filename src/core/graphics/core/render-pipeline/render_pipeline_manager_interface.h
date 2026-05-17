//
// Created by ssj5v on 18-05-2026.
//

#ifndef RENDER_PIPELINE_MANAGER_INTERFACE_H
#define RENDER_PIPELINE_MANAGER_INTERFACE_H
#include "buffers/render/renderbuffer.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "render_pipeline.h"

class IRenderPipelineManagerSubsystem : public IEngineSubSystem {
public:
    ~IRenderPipelineManagerSubsystem() override = default;

    virtual void setRenderTarget(SRenderBuffer* renderBuffer) = 0;
    virtual SVector2 getRenderResolution() = 0;

    virtual void preRender()=0;
    virtual void render()=0;
    virtual void postRender()=0;

    virtual IRenderPipeline* getPipeline() = 0;
};



#endif //RENDER_PIPELINE_MANAGER_INTERFACE_H
