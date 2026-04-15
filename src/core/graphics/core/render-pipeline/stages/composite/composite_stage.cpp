//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include "composite_stage.h"

#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/utils/logger.h"

void MCompositeStage::init(IRenderPipeline* const pipeline) {}
void MCompositeStage::cleanup(IRenderPipeline* const pipeline) {}
void MCompositeStage::preRender(IRenderPipeline* const pipeline) {}
void MCompositeStage::postRender(IRenderPipeline* const pipeline) {}

// ---------------------------------------------------------------------------
// render() — the only place composite logic lives
// ---------------------------------------------------------------------------

void MCompositeStage::render(IRenderPipeline* const pipeline)
{
    const uint32_t flags = pipeline->getCompositeFlags();
    if (flags == ECF_None) return;

    if (flags & ECF_Depth)
        compositeDepth(pipeline);

    // Future buffers follow the same pattern:
    //   if (flags & ECF_Opaque)      compositeOpaque(pipeline);
    //   if (flags & ECF_Lights)      compositeLights(pipeline);
    //   if (flags & ECF_Transparent) compositeTransparent(pipeline);
}

// ---------------------------------------------------------------------------
// Per-buffer composite helpers
// ---------------------------------------------------------------------------

void MCompositeStage::compositeDepth(IRenderPipeline* const pipeline) const
{
    auto* depthBuffer  = pipeline->getBufferRegistry()
                                  .getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    auto* renderBuffer = pipeline->getRenderBuffer();

    // Also guard on fboHandle: if setRenderBuffer() was never called, the FBO
    // was never allocated and blitting from handle 0 causes GL_INVALID_OPERATION.
    if (!depthBuffer || !renderBuffer || depthBuffer->getFBOHandle() == 0)
        return;

    // Activate the SFML render target so its internal FBO is current.
    renderBuffer->bindAsActive();

    GLint sfmlFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sfmlFBO);

    const SVector2 res = pipeline->getRenderResolution();
    const int      w   = static_cast<int>(res.x);
    const int      h   = static_cast<int>(res.y);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, depthBuffer->getFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);

    // Clear any pre-existing errors so they don't mask the blit result.
    while (glGetError() != GL_NO_ERROR) {}

    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Check immediately — if the blit failed (e.g. GL_DEPTH_COMPONENT32F vs
    // SFML's GL_DEPTH_COMPONENT24 renderbuffer), log once and skip silently
    // rather than letting the error propagate into SFML's own glCheck calls.
    GLenum blitErr = glGetError();
    if (blitErr != GL_NO_ERROR)
        MWARN("MCompositeStage::compositeDepth — depth blit failed (GL "
              + std::to_string(blitErr) + "). Depth will not be composited.");

    // Restore SFML's FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
}