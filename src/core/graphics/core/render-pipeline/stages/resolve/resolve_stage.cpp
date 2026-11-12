//
// Resolve stage - RS_Resolve (4100).
//

#include <GL/glew.h>

#include "resolve_stage.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/texture/texture_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"

static constexpr const char* RESOLVE_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/resolve.mesl";

void MResolveStage::init(IRenderPipeline* const pipeline)
{
    auto& registry = pipeline->getBufferRegistry();
    sceneBuffer = registry.createBuffer<SFrameBuffer>(MBufferNames::BUFFER_SCENE);
    if (sceneBuffer)
    {
        sceneBuffer->setColorFormat(0x881A); // GL_RGBA16F - linear HDR

        // Share the prepass depth so transparents depth-test against opaque
        // geometry. Pipelines without a prepass keep a private (empty) depth.
        if (auto* depth = registry.getBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH))
            sceneBuffer->shareDepthFrom(depth);
    }
    else
    {
        MERROR("MResolveStage::init - failed to create BUFFER_SCENE");
    }

    quad.create();
}

void MResolveStage::cleanup(IRenderPipeline* const pipeline)
{
    quad.destroy();
    sceneBuffer = nullptr;
}

void MResolveStage::preRender(IRenderPipeline* const pipeline) {}

void MResolveStage::render(IRenderPipeline* const pipeline)
{
    if (!sceneBuffer || sceneBuffer->getFBOHandle() == 0 || !quad.isValid()) return;

    auto& registry = pipeline->getBufferRegistry();
    auto* opaque   = registry.getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    auto* lights   = registry.getBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (!opaque || opaque->getFBOHandle() == 0) return;

    const uint32_t flags = pipeline->getCompositeFlags();
    if (!(flags & ECF_Opaque)) return;
    const bool useLights = (flags & ECF_Lights) && lights && lights->getFBOHandle() != 0;

    auto* emission = registry.getBuffer<STextureBuffer>(MBufferNames::BUFFER_EMISSION);
    const bool useEmission = emission && emission->getTextureHandle() != 0;

    auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                           ->getAsset<MShaderAsset>(RESOLVE_SHADER_PATH);
    MShader* shader = shaderAsset ? shaderAsset->getShader() : nullptr;
    if (!shader) return;

    const SVector2 res = pipeline->getRenderResolution();
    sceneBuffer->bindAsActive();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    // Overwrites every pixel - no clear needed. Depth is the prepass depth
    // and must stay intact for the transparent stage.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    shader->bind();
    SShaderPropertyValue identity;
    identity.setMat4Val(glm::mat4(1.f));
    shader->setPropertyValue("model",      identity);
    shader->setPropertyValue("view",       identity);
    shader->setPropertyValue("projection", identity);

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, opaque->getColorTextureHandle());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, useLights ? lights->getColorTextureHandle()
                                           : opaque->getColorTextureHandle());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, useEmission ? emission->getTextureHandle()
                                             : opaque->getColorTextureHandle());

    GLint l;
    l = glGetUniformLocation(prog, "_albedoTex"); if (l >= 0) glUniform1i(l, 0);
    l = glGetUniformLocation(prog, "_lightsTex"); if (l >= 0) glUniform1i(l, 1);
    l = glGetUniformLocation(prog, "_useLights"); if (l >= 0) glUniform1i(l, useLights ? 1 : 0);
    l = glGetUniformLocation(prog, "_emissionTex"); if (l >= 0) glUniform1i(l, 2);
    l = glGetUniformLocation(prog, "_useEmission"); if (l >= 0) glUniform1i(l, useEmission ? 1 : 0);

    quad.draw();

    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    sceneBuffer->unbind();
    pipeline->addCompositeFlag(ECF_Scene);
}

void MResolveStage::postRender(IRenderPipeline* const pipeline) {}
