//
// Post-process stages - run MPostProcessStack effects on BUFFER_SCENE.
//

#include <GL/glew.h>

#include "post_process_stage.h"

#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/postprocess/post_process_stack.h"
#include "core/graphics/core/render-pipeline/view_data.h"
#include "core/utils/logger.h"

void MPostProcessStageBase::init(IRenderPipeline* const pipeline)
{
    stack = pipeline->getAttachment<MPostProcessStack>();

    auto& registry = pipeline->getBufferRegistry();
    sceneBuffer = registry.getBuffer<SFrameBuffer>(MBufferNames::BUFFER_SCENE);
    if (!sceneBuffer)
        MERROR("MPostProcessStage::init - BUFFER_SCENE not found; add MResolveStage first");

    // Shared by both post stages - create once.
    const SString names[2] = { MBufferNames::BUFFER_POST_A, MBufferNames::BUFFER_POST_B };
    for (int i = 0; i < 2; ++i)
    {
        pingPong[i] = registry.getBuffer<SFrameBuffer>(names[i]);
        if (!pingPong[i])
        {
            pingPong[i] = registry.createBuffer<SFrameBuffer>(names[i]);
            if (pingPong[i]) pingPong[i]->setColorFormat(0x881A); // GL_RGBA16F
        }
    }

    quad.create();
}

void MPostProcessStageBase::cleanup(IRenderPipeline* const pipeline)
{
    quad.destroy();
    stack       = nullptr;
    sceneBuffer = nullptr;
    pingPong[0] = pingPong[1] = nullptr;
}

void MPostProcessStageBase::render(IRenderPipeline* const pipeline)
{
    if (!stack || !stack->isEnabled()) return;
    if (!sceneBuffer || sceneBuffer->getFBOHandle() == 0) return;
    if (!pingPong[0] || !pingPong[1] ||
        pingPong[0]->getFBOHandle() == 0 || pingPong[1]->getFBOHandle() == 0) return;
    if (!(pipeline->getCompositeFlags() & ECF_Scene)) return;   // resolve didn't run

    const SViewData viewData = resolveViewData(pipeline);
    const SVector2  res      = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    SPostProcessContext ctx;
    ctx.pipeline   = pipeline;
    ctx.registry   = &pipeline->getBufferRegistry();
    ctx.view       = &viewData;
    ctx.resolution = res;
    ctx.quad       = &quad;

    // Current image: starts as BUFFER_SCENE, then whichever buffer the last
    // effect wrote.
    SFrameBuffer* current = sceneBuffer;
    int           next    = 0;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    stack->forEachEnabled(getSlot(), [&](MPostProcessEffect& effect)
    {
        SFrameBuffer* target = pingPong[next];
        target->bindAsActive();
        glViewport(0, 0, w, h);

        ctx.sourceTexture = current->getColorTextureHandle();
        ctx.target        = target;

        if (effect.apply(ctx))
        {
            current = target;
            next    = 1 - next;
        }
    });

    // Copy the final image back so later stages read it from BUFFER_SCENE.
    // Colour only - BUFFER_SCENE's depth is the shared prepass depth.
    if (current != sceneBuffer)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, current->getFBOHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneBuffer->getFBOHandle());
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}
