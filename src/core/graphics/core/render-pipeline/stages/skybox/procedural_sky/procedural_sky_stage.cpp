//
// MProceduralSkyboxStage
//
#include "procedural_sky_stage.h"
#include <GL/glew.h>

#include "core/engine/skybox/procedural_sky/procedural_skybox_drawcall.h"
#include "core/engine/skybox/procedural_sky/procedural_skybox_queue.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/utils/logger.h"

void MProceduralSkyboxStage::init(IRenderPipeline* const pipeline)
{
    opaqueBuffer = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer)
        MERROR("MProceduralSkyboxStage::init — BUFFER_OPAQUE not found; ensure MOpaqueStage is added first");
}

void MProceduralSkyboxStage::cleanup(IRenderPipeline* const pipeline)
{
    opaqueBuffer = nullptr;
}

void MProceduralSkyboxStage::preRender(IRenderPipeline* const pipeline)
{
    // Re-fetch in case the buffer was recreated after a resize.
    opaqueBuffer = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
}

void MProceduralSkyboxStage::render(IRenderPipeline* const pipeline)
{
    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0) return;
    if (MProceduralSkyboxQueue::getAll().empty()) return;

    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    for (auto* drawCall : MProceduralSkyboxQueue::getAll())
    {
        if (!drawCall) continue;
        drawCall->setTargetResolution(res);
        drawCall->draw();
    }

    opaqueBuffer->unbind();
}