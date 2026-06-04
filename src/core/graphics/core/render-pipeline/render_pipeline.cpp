//
// Created by ssj5v on 21-03-2026.
//
#include <GL/glew.h>
#include "render_pipeline.h"
#include "buffers/render/renderbuffer.h"
#include "core/engine/camera/camera_spatial_entity.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/profiling/simple_profiler/simple_profiler.h"
#include "core/utils/frustum.h"
#include "core/utils/logger.h"
#include "render_queue.h"
#include "stages/clear/clear_stage.h"
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

    addStage<MClearStage>();              // clear screen pass.
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

void MRenderPipeline::initManual()
{
    if (initialised)
        return;

    // Mark as ready without adding any stages.
    // The caller adds only the stages it needs via addStage<T>().
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
    START_PROFILING_SAMPLE("RenderPipeline.PreRender")
    if (!initialised)
        return;

    // In manual item mode (thumbnail pipeline, reflection probes, etc.),
    // items are submitted externally via clearRenderItems() + submitRenderItem()
    // BEFORE preRender() is called. Do not overwrite them.
    if (!manualItemMode)
    {
        renderItems.clear();
        clearCompositeFlags();
        MRenderQueue::collectAll(this);
    }

    if (!bufferRegistry.getRenderBuffer())
        return;

    // -- Frustum cull against the active camera --------------------------------
    // Skip frustum culling in manual item mode -- the caller controls
    // what items are submitted and may be using a camera that is not
    // registered with MViewManagement.
    if (!manualItemMode)
    {
        MCameraEntity* camera = MViewManagement::getFirstActiveCamera();
        if (camera)
        {
            const SVector2 res = bufferRegistry.getRenderBuffer()->getResolution();
            const SMatrix4 vp  = camera->getProjectionMatrix(res) * camera->getViewMatrix();

            SFrustum frustum;
            frustum.extractFromVP(vp);

            renderItems.erase(
                std::remove_if(renderItems.begin(), renderItems.end(),
                    [&frustum](const SRenderItem& item)
                    {
                        if (item.bounds.min == item.bounds.max)
                            return false;

                        return !frustum.testAABB(item.bounds);
                    }),
                renderItems.end());
        }
    }

    std::sort(renderItems.begin(), renderItems.end(),
              [](const SRenderItem& a, const SRenderItem& b)
              { return a.sortOrder < b.sortOrder; });

    for (auto* stage : renderStages)
        stage->preRender(this);
    STOP_PROFILING_SAMPLE("RenderPipeline.PreRender")
}

void MRenderPipeline::render()
{
    START_PROFILING_SAMPLE("RenderPipeline.Render")
    if (!initialised || !bufferRegistry.getRenderBuffer())
        return;

    for (auto* stage : renderStages)
        stage->render(this);
    STOP_PROFILING_SAMPLE("RenderPipeline.Render")
}

void MRenderPipeline::postRender()
{
    START_PROFILING_SAMPLE("RenderPipeline.PostRender")
    if (!initialised || !bufferRegistry.getRenderBuffer())
        return;

    for (auto* stage : renderStages)
        stage->postRender(this);

    forceResetGLStates();
    STOP_PROFILING_SAMPLE("RenderPipeline.PostRender")
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