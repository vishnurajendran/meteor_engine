//
// Created by ssj5v on 11-04-2026.
//

#pragma once
#ifndef RENDER_PIPELINE_INTERFACE_H
#define RENDER_PIPELINE_INTERFACE_H

#include <cstdint>
#include <vector>

#include "core/graphics/core/render-pipeline/render_item.h"
#include "render_stage_interface.h"

class MBufferRegistery;

// Bitmask set by stages to declare what they have rendered into their buffer.
// MCompositeStage reads these flags to decide what to composite into the
// final render target.
enum ECompositeFlags : uint32_t
{
    ECF_None        = 0,
    ECF_Depth       = 1 << 0,
    ECF_Opaque      = 1 << 1,
    ECF_Lights      = 1 << 2,
    ECF_Shadow      = 1 << 3,
    ECF_Transparent = 1 << 4,
};

// Pure orchestration interface.  No render logic lives here -- the pipeline
// creates buffers and stages, drives the frame lifecycle, and exposes state
// that stages need to communicate with each other.
class IRenderPipeline
{
public:
    virtual ~IRenderPipeline() = default;

    virtual void init()=0;
    virtual void cleanup()=0;

    // stages
    template<typename T>
    bool addStage()
    {
        static_assert(std::is_base_of_v<IRenderStage, T>,
                      "T must derive from IRenderStage");
        IRenderStage* stage = new T();
        stage->init(this);
        renderStages.push_back(stage);

        // sort the render stages after each addition.
        std::sort(renderStages.begin(), renderStages.end(),
            [](IRenderStage* a, IRenderStage* b) { return a->getSortingOrder() < b->getSortingOrder(); });

        return true;
    }

    // Frame lifecycle
    virtual void preRender()  = 0;
    virtual void render()     = 0;
    virtual void postRender() = 0;

    // Render target
    virtual SVector2       getRenderResolution()                  = 0;
    virtual void           setRenderBuffer(SRenderBuffer* buffer) = 0;
    virtual SRenderBuffer* getRenderBuffer()                      = 0;

    // Buffer registry -- stages read/write named buffers
    virtual MBufferRegistery& getBufferRegistry() = 0;

    // Render items -- collected once per frame, consumed by every stage
    virtual const std::vector<SRenderItem>& getRenderItems() const = 0;

    // Composite flags -- stages declare what they produced this frame
    virtual uint32_t getCompositeFlags()                  const = 0;
    virtual void     addCompositeFlag(ECompositeFlags flag)     = 0;
    virtual void     removeCompositeFlag(ECompositeFlags flag)  = 0;
    virtual void     clearCompositeFlags()                      = 0;

    // ---- Camera override ----------------------------------------------------
    // When active, stages use these matrices instead of querying
    // MViewManagement.  Used by the thumbnail pipeline and any future
    // offscreen pipeline that needs its own camera.
    struct SCameraOverride
    {
        bool      active = false;
        glm::mat4 view   = glm::mat4(1.f);
        glm::mat4 proj   = glm::mat4(1.f);
    };

    void setCameraOverride(const glm::mat4& view, const glm::mat4& proj)
    {
        cameraOverride.active = true;
        cameraOverride.view   = view;
        cameraOverride.proj   = proj;
    }

    void clearCameraOverride() { cameraOverride.active = false; }

    const SCameraOverride& getCameraOverride() const { return cameraOverride; }

protected:
    std::vector<IRenderStage*> renderStages;
    SCameraOverride            cameraOverride;
};

#endif // RENDER_PIPELINE_INTERFACE_H