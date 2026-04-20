//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include "skybox_stage.h"

#include "skybox_queue.h"
#include "skyboxdrawcall.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/utils/logger.h"

void MSkyboxStage::init(IRenderPipeline* const pipeline)
{
    // BUFFER_OPAQUE is created by MOpaqueStage which runs earlier in init().
    // We just grab a reference — we don't own this buffer.
    opaqueBuffer = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer)
        MERROR("MSkyboxStage::init — BUFFER_OPAQUE not found; ensure MOpaqueStage is added first");
}

void MSkyboxStage::cleanup(IRenderPipeline* const pipeline)
{
    opaqueBuffer = nullptr;
}

void MSkyboxStage::preRender(IRenderPipeline* const pipeline)
{
    // Re-fetch in case the buffer was recreated (e.g. after a resize).
    opaqueBuffer = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
}

void MSkyboxStage::render(IRenderPipeline* const pipeline)
{
    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0) return;
    if (MSkyboxQueue::getDrawCalls().empty()) return;

    // Bind directly into BUFFER_OPAQUE so the skybox and geometry share the
    // same colour buffer.  The composite stage blits this to the render target.
    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    for (auto* drawCall : MSkyboxQueue::getDrawCalls())
    {
        if (!drawCall) continue;
        drawCall->setTargetResolution(res);
        drawCall->draw();
    }

    opaqueBuffer->unbind();
}

void MSkyboxStage::postRender(IRenderPipeline* const pipeline) {}