//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include "skybox_stage.h"

#include "skybox_queue.h"
#include "skyboxdrawcall.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/utils/logger.h"

void MSkyboxStage::init(IRenderPipeline* const pipeline) {}
void MSkyboxStage::cleanup(IRenderPipeline* const pipeline) { renderTarget = nullptr; }
void MSkyboxStage::postRender(IRenderPipeline* const pipeline) {}

void MSkyboxStage::preRender(IRenderPipeline* const pipeline)
{
    auto* buffer = pipeline->getRenderBuffer();
    if (!buffer) return;

    renderTarget = buffer->getCoreRenderTarget();
    if (!renderTarget) return;

    if (!renderTarget->setActive(true))
        MERROR("MSkyboxStage::preRender — could not activate render target");
}

void MSkyboxStage::render(IRenderPipeline* const pipeline)
{
    if (!renderTarget) return;

    const auto size = renderTarget->getSize();
    glViewport(0, 0, size.x, size.y);

    for (auto* drawCall : MSkyboxQueue::getDrawCalls())
    {
        if (!drawCall) continue;
        drawCall->setTargetResolution({ static_cast<float>(size.x),
                                        static_cast<float>(size.y) });
        drawCall->draw();
    }
}