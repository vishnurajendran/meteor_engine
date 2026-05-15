#include <GL/glew.h>
#include "clear_stage.h"

#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"

void MClearStage::init(IRenderPipeline* const pipeline) {}

void MClearStage::cleanup(IRenderPipeline* const pipeline) {}

void MClearStage::preRender(IRenderPipeline* const pipeline) {}

void MClearStage::render(IRenderPipeline* const pipeline)
{
    auto* opaqueBuffer = pipeline->getBufferRegistry()
                                  .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0) return;

    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    glDepthMask(GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    opaqueBuffer->unbind();
}

void MClearStage::postRender(IRenderPipeline* const pipeline) {}