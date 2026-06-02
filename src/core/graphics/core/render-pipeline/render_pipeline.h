//
// Created by ssj5v on 21-03-2026.
//

#pragma once
#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <vector>
#include <cstdint>
#include "buffer_registry.h"
#include "core/object/object.h"
#include "core/utils/glmhelper.h"
#include "interfaces/render_item_collector.h"
#include "interfaces/render_pipeline_interface.h"
#include "interfaces/render_stage_interface.h"
#include "render_item.h"

// Pure orchestration -- owns stages and the buffer registry, drives the frame
// lifecycle, and collects render items from the scene.  Contains zero render
// logic; all drawing decisions live inside the stages.
class MRenderPipeline : public MObject,
                        public IRenderPipeline,
                        public IRenderItemCollector
{
    DEFINE_OBJECT_SUBCLASS(MRenderPipeline)
public:
    MRenderPipeline() = default;
    ~MRenderPipeline() override;

    // Creates default buffers and registers all pipeline stages.
    void init() override;
    void cleanup() override;

    // Initialize without default stages.  The caller adds stages manually
    // via addStage<T>() afterwards.  Use this for secondary pipelines
    // (thumbnails, reflection probes, etc.) that need a different stage set.
    void initManual();

    // ---- IRenderPipeline -- frame lifecycle ---------------------------------
    void preRender()  override;
    void render()     override;
    void postRender() override;

    // ---- IRenderPipeline -- render target ------------------------------------
    SVector2       getRenderResolution() override { return bufferRegistry.getRenderBuffer()->getResolution(); }
    void           setRenderBuffer(SRenderBuffer* renderBuffer) override;
    SRenderBuffer* getRenderBuffer()     override { return bufferRegistry.getRenderBuffer(); }

    // ---- IRenderPipeline -- buffer registry ----------------------------------
    MBufferRegistery& getBufferRegistry() override { return bufferRegistry; }

    // ---- IRenderPipeline -- render items -------------------------------------
    const std::vector<SRenderItem>& getRenderItems() const override { return renderItems; }

    // ---- IRenderPipeline -- composite flags ----------------------------------
    uint32_t getCompositeFlags()                  const override { return compositeFlags; }
    void     addCompositeFlag(ECompositeFlags flag)     override { compositeFlags |=  flag; }
    void     removeCompositeFlag(ECompositeFlags flag)  override { compositeFlags &= ~flag; }
    void     clearCompositeFlags()                      override { compositeFlags  =  ECF_None; }

    // ---- IRenderItemCollector -----------------------------------------------
    void submitRenderItem(const SRenderItem& item) override;

    // ---- Manual item mode ---------------------------------------------------
    // When enabled, preRender() does not collect from MRenderQueue.
    // The caller populates items via clearRenderItems() + submitRenderItem()
    // before calling preRender().
    void setManualItemMode(bool manual) { manualItemMode = manual; }

    // Clear the render item list.  Call before submitRenderItem() when
    // using manual item mode between frames.
    void clearRenderItems() { renderItems.clear(); }

private:
    void forceResetGLStates();

private:
    MBufferRegistery           bufferRegistry;
    std::vector<SRenderItem>   renderItems;
    uint32_t                   compositeFlags = ECF_None;
    bool                       initialised    = false;
    bool                       manualItemMode = false;
};

#endif // RENDER_PIPELINE_H