#include <GL/glew.h>
#include "gizmo_stage.h"

#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"

void MGizmoStage::init(IRenderPipeline* const pipeline) {}

void MGizmoStage::cleanup(IRenderPipeline* const pipeline) {}

void MGizmoStage::preRender(IRenderPipeline* const pipeline) {}

void MGizmoStage::render(IRenderPipeline* const pipeline)
{
    SRenderBuffer* renderBuffer = pipeline->getRenderBuffer();
    if (!renderBuffer) return;

    renderBuffer->bindAsActive();
    GLint sfmlFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sfmlFBO);

    const SVector2 res = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    // Blit scene depth so icon gizmos can depth-test against geometry.
    auto* opaqueBuf = pipeline->getBufferRegistry()
                               .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (opaqueBuf && opaqueBuf->getFBOHandle() != 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, opaqueBuf->getFBOHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
    glViewport(0, 0, w, h);

    glDisable(GL_DEPTH_TEST);   // ← add this — lines draw on top of everything
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    MGizmos::requestGizmoDraws();
}

void MGizmoStage::postRender(IRenderPipeline* const pipeline) {}