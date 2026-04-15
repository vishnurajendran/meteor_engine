//
// Created by ssj5v on 11-04-2026.
//

#pragma once
#ifndef RENDER_PIPELINE_INTERFACE_H
#define RENDER_PIPELINE_INTERFACE_H

#include <cstdint>
#include <vector>
#include "core/graphics/core/render-pipeline/render_item.h"

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

// Pure orchestration interface.  No render logic lives here — the pipeline
// creates buffers and stages, drives the frame lifecycle, and exposes state
// that stages need to communicate with each other.
class IRenderPipeline
{
public:
    virtual ~IRenderPipeline() = default;

    // Frame lifecycle
    virtual void preRender()  = 0;
    virtual void render()     = 0;
    virtual void postRender() = 0;

    // Render target
    virtual SVector2       getRenderResolution()                  = 0;
    virtual void           setRenderBuffer(SRenderBuffer* buffer) = 0;
    virtual SRenderBuffer* getRenderBuffer()                      = 0;

    // Buffer registry — stages read/write named buffers
    virtual MBufferRegistery& getBufferRegistry() = 0;

    // Render items — collected once per frame, consumed by every stage
    virtual const std::vector<SRenderItem>& getRenderItems() const = 0;

    // Composite flags — stages declare what they produced this frame
    virtual uint32_t getCompositeFlags()                  const = 0;
    virtual void     addCompositeFlag(ECompositeFlags flag)     = 0;
    virtual void     removeCompositeFlag(ECompositeFlags flag)  = 0;
    virtual void     clearCompositeFlags()                      = 0;
};

#endif // RENDER_PIPELINE_INTERFACE_H