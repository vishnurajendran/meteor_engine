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

    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    glDepthMask(GL_FALSE);  // don't corrupt the clean depth from clear stage

    for (auto* drawCall : MSkyboxQueue::getDrawCalls())
    {
        if (!drawCall) continue;
        drawCall->setTargetResolution(res);
        drawCall->draw();
    }

    glDepthMask(GL_TRUE);   // restore for opaque stage

    // Unbind any cubemap left by the draw call so it doesn't pollute
    // subsequent stages' sampler state. The cubemap skybox binds its
    // texture at unit 0 and never cleans up — stale cubemap bindings
    // cause undefined behaviour for sampler2D/samplerCube uniforms in
    // material and lighting shaders that default to unit 0.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    opaqueBuffer->unbind();
}

void MSkyboxStage::postRender(IRenderPipeline* const pipeline) {}