#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "composite_stage.h"

#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/utils/logger.h"

static constexpr const char* FINAL_COMPOSITE_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/final_composite.mesl";

static const float k_quadVerts[] = {
    -1.f,-1.f,0.f, 0.f,0.f,  1.f,-1.f,0.f, 1.f,0.f,
     1.f, 1.f,0.f, 1.f,1.f, -1.f, 1.f,0.f, 0.f,1.f,
};
static const unsigned int k_quadIdx[] = { 0,1,2, 0,2,3 };

void MCompositeStage::init(IRenderPipeline* const pipeline)
{
    initFullscreenQuad();
    auto* asset = MAssetManager::getInstance()
                               ->getAsset<MShaderAsset>(FINAL_COMPOSITE_SHADER_PATH);
    if (!asset)
        MERROR("MCompositeStage::init — could not load: " + SString(FINAL_COMPOSITE_SHADER_PATH));
    else
        finalCompositeShader = asset->getShader();
}

void MCompositeStage::cleanup(IRenderPipeline* const pipeline)
{
    destroyFullscreenQuad();
    finalCompositeShader = nullptr;
}

void MCompositeStage::preRender (IRenderPipeline* const pipeline) {}
void MCompositeStage::postRender(IRenderPipeline* const pipeline) {}

void MCompositeStage::render(IRenderPipeline* const pipeline)
{
    while (glGetError() != GL_NO_ERROR) {}

    const uint32_t flags = pipeline->getCompositeFlags();
    if (flags == ECF_None) return;

    SRenderBuffer* renderBuffer = pipeline->getRenderBuffer();
    if (!renderBuffer) return;

    // Bind SFML target — all output goes here directly.
    renderBuffer->bindAsActive();
    GLint sfmlFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &sfmlFBO);

    const SVector2 res = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    const bool hasOpaque = flags & ECF_Opaque;
    const bool hasLights = flags & ECF_Lights;

    if (hasOpaque && hasLights)
        compositeOpaqueWithLights(pipeline, sfmlFBO, w, h);
    else if (hasOpaque)
        compositeOpaqueOnly(pipeline, sfmlFBO, w, h);

    if (flags & ECF_Depth)
        compositeDepth(pipeline, sfmlFBO, w, h);

    // Gizmos drawn last, directly into the SFML FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
    glViewport(0, 0, w, h);
    MGizmos::requestGizmoDraws();
}

void MCompositeStage::compositeOpaqueWithLights(IRenderPipeline* const pipeline,
                                                int sfmlFBO, int w, int h) const
{
    if (!finalCompositeShader || quadVAO == 0) return;

    auto* opaque = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    auto* lights = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (!opaque || opaque->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    finalCompositeShader->bind();

    const glm::mat4 identity(1.0f);
    SShaderPropertyValue mv;
    mv.setMat4Val(identity);
    finalCompositeShader->setPropertyValue("model",      mv);
    finalCompositeShader->setPropertyValue("view",       mv);
    finalCompositeShader->setPropertyValue("projection", mv);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, opaque->getColorTextureHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lights && lights->getFBOHandle() != 0
                                  ? lights->getColorTextureHandle()
                                  : opaque->getColorTextureHandle());

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    GLint albLoc = glGetUniformLocation(prog, "_albedoTex");
    GLint litLoc = glGetUniformLocation(prog, "_lightsTex");
    if (albLoc >= 0) glUniform1i(albLoc, 0);
    if (litLoc >= 0) glUniform1i(litLoc, 1);

    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}

void MCompositeStage::compositeOpaqueOnly(IRenderPipeline* const pipeline,
                                          int sfmlFBO, int w, int h) const
{
    auto* opaque = pipeline->getBufferRegistry()
                            .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaque || opaque->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, opaque->getFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);
    while (glGetError() != GL_NO_ERROR) {}
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
}

void MCompositeStage::compositeDepth(IRenderPipeline* const pipeline,
                                     int sfmlFBO, int w, int h) const
{
    auto* buf = pipeline->getBufferRegistry()
                         .getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    if (!buf || buf->getFBOHandle() == 0) return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, buf->getFBOHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sfmlFBO);
    while (glGetError() != GL_NO_ERROR) {}
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, sfmlFBO);
}

void MCompositeStage::initFullscreenQuad()
{
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(k_quadVerts), k_quadVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(k_quadIdx), k_quadIdx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glBindVertexArray(0);
}

void MCompositeStage::destroyFullscreenQuad()
{
    if (quadEBO) { glDeleteBuffers(1, &quadEBO); quadEBO = 0; }
    if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
}