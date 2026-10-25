//
// Created by ssj5v on 18-05-2026.
//

#ifndef RENDER_PIPELINE_MANAGER_INTERFACE_H
#define RENDER_PIPELINE_MANAGER_INTERFACE_H
#include "buffers/render/renderbuffer.h"
#include "core/engine/subsystem/subsystem_interface.h"
#include "render_pipeline.h"

class MLightSystemManager;
class MProceduralSkyboxQueue;
class MRenderQueue;
class MSkyboxQueue;

class IRenderPipelineManagerSubsystem : public IEngineSubSystem {
public:
    ~IRenderPipelineManagerSubsystem() override = default;

    virtual void setRenderTarget(SRenderBuffer* renderBuffer) = 0;
    virtual SVector2 getRenderResolution() = 0;

    virtual void preRender()=0;
    virtual void render()=0;
    virtual void postRender()=0;

    virtual IRenderPipeline* getPipeline() = 0;

    // -- World render state ---------------------------------------------------
    // What there is to draw, registered by entities (on create, removed on
    // exit) and read by the pipeline's stages each frame. Every pipeline,
    // including the editor's thumbnail pipeline, reads the same state.
    virtual MRenderQueue&           getRenderQueue() = 0;            // drawables
    virtual MSkyboxQueue&           getSkyboxQueue() = 0;            // cubemap skyboxes
    virtual MProceduralSkyboxQueue& getProceduralSkyboxQueue() = 0;  // procedural skies
    virtual MLightSystemManager&    getLightSystem() = 0;            // ambient, directional, dynamic lights
};



#endif //RENDER_PIPELINE_MANAGER_INTERFACE_H
