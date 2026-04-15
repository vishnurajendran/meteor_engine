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
#include "stages/legacy_render_stage/graphicsrenderer.h"
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

    addStage<MDepthRenderStage>();
    addStage<MGraphicsRenderer>();
    addStage<MSkyboxStage>();      // after opaques, before transparents
    addStage<MCompositeStage>();   // always last — composites whatever flags are set

    initialised = true;
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
    // bufferRegistry destructor handles all named buffers.
}

// ---------------------------------------------------------------------------
// Render target
// ---------------------------------------------------------------------------

void MRenderPipeline::setRenderBuffer(SRenderBuffer* renderBuffer)
{
    if (!initialised)
        return;

    bufferRegistry.setRenderBuffer(renderBuffer);

    // Resize every registered buffer to match the new render target immediately.
    // This is the single resize trigger — stages never need to poll for changes.
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
// Frame lifecycle — pure orchestration, no render logic
// ---------------------------------------------------------------------------

void MRenderPipeline::preRender()
{
    if (!initialised)
        return;

    // Collect items every frame — drawables may submit before the render
    // buffer is set and that is fine.
    renderItems.clear();
    clearCompositeFlags();
    MRenderQueue::collectAll(this);

    // Do not drive stages until a render buffer is available — any GL call
    // made with an uninitialised FBO (handle 0) triggers GL_INVALID_OPERATION.
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