//
// Created by ssj5v on 21-03-2026.
//
#include <GL/glew.h>
#include "render_pipeline.h"
#include "buffers/render/renderbuffer.h"
#include "core/utils/logger.h"
#include "render_queue.h"
#include "stages/composite/composite_stage.h"
#include "stages/depth/depth_render_stage.h"
#include "stages/lighting/lighting_stage.h"
#include "stages/opaque/opaque_stage.h"
#include "stages/shadow/shadow_stage.h"
#include "stages/skybox/procedural_sky/procedural_sky_stage.h"
#include "stages/skybox/skybox_stage.h"

MRenderPipeline::~MRenderPipeline()
{
    cleanup();
}

// ---------------------------------------------------------------------------
// Init / cleanup
// ---------------------------------------------------------------------------

void MRenderPipeline::init()
{
    if (initialised)
        return;

    addStage<MDepthRenderStage>();        // depth prepass
    addStage<MShadowStage>();             // shadow map         -> BUFFER_SHADOW
    addStage<MOpaqueStage>();             // opaque geometry    -> BUFFER_OPAQUE
    addStage<MLightingStage>();           // light accumulation -> BUFFER_LIGHTS
    addStage<MSkyboxStage>();             // cubemap skybox        (after opaques)
    addStage<MProceduralSkyboxStage>();   // procedural skybox     (after opaques)
    addStage<MCompositeStage>();          // blits all buffers  -> render target

    initialised = true;
    MLOG("MRenderPipeline:: Initialised");
}

void MRenderPipeline::cleanup()
{
    if (!initialised)
        return;

    for (auto* stage : renderStages)
        stage->cleanup(this);

    while (!renderStages.empty())
    {
        delete renderStages.back();
        renderStages.pop_back();
    }

    renderItems.clear();
    compositeFlags = ECF_None;
}

// ---------------------------------------------------------------------------
// Render target
// ---------------------------------------------------------------------------

void MRenderPipeline::setRenderBuffer(SRenderBuffer* renderBuffer)
{
    if (!initialised)
        return;

    bufferRegistry.setRenderBuffer(renderBuffer);

    if (renderBuffer)
    {
        const auto res = renderBuffer->getResolution();
        bufferRegistry.resizeAll(static_cast<int>(res.x), static_cast<int>(res.y));
    }
}

// ---------------------------------------------------------------------------
// IRenderItemCollector
// ---------------------------------------------------------------------------

void MRenderPipeline::submitRenderItem(const SRenderItem& item)
{
    if (!initialised)
        return;

    renderItems.push_back(item);
}

// ---------------------------------------------------------------------------
// Frame lifecycle
// ---------------------------------------------------------------------------

void MRenderPipeline::preRender()
{
    if (!initialised)
        return;

    renderItems.clear();
    clearCompositeFlags();
    MRenderQueue::collectAll(this);

    if (!bufferRegistry.getRenderBuffer())
        return;

    std::sort(renderItems.begin(), renderItems.end(),
              [](const SRenderItem& a, const SRenderItem& b)
              { return a.sortOrder < b.sortOrder; });

    for (auto* stage : renderStages)
        stage->preRender(this);
}

void MRenderPipeline::render()
{
    if (!initialised || !bufferRegistry.getRenderBuffer())
        return;

    for (auto* stage : renderStages)
        stage->render(this);
}

void MRenderPipeline::postRender()
{
    if (!initialised || !bufferRegistry.getRenderBuffer())
        return;

    for (auto* stage : renderStages)
        stage->postRender(this);

    forceResetGLStates();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MRenderPipeline::forceResetGLStates()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);
}