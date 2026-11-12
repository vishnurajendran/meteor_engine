//
// Prepass stage - RS_Prepass (100), right after the clear stage.
//

#include <GL/glew.h>
#include <algorithm>

#include "prepass_stage.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/depth/depthbuffer.h"
#include "core/graphics/core/render-pipeline/buffers/texture/texture_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/view_data.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/graphics/core/shader/default_textures.h"
#include "core/utils/logger.h"

static constexpr const char* PREPASS_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/internal/prepass.mesl";

// Highest texture unit material textures may use in the prepass.
static constexpr int MAX_MATERIAL_TEX_UNIT = 15;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MPrepassStage::init(IRenderPipeline* const pipeline)
{
    const auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                                 ->getAsset<MShaderAsset>(PREPASS_SHADER_PATH);
    if (!shaderAsset)
        MERROR("MPrepassStage::init - could not load prepass shader: " + SString(PREPASS_SHADER_PATH));
    else
        prepassShader = shaderAsset->getShader();

    // GPU storage is allocated when the pipeline's render buffer is set (resizeAll).
    auto& registry = pipeline->getBufferRegistry();
    depthBuffer   = registry.createBuffer<SDepthBuffer>  (MBufferNames::BUFFER_DEPTH);
    normalBuffer  = registry.createBuffer<STextureBuffer>(MBufferNames::BUFFER_NORMAL);
    surfaceBuffer = registry.createBuffer<STextureBuffer>(MBufferNames::BUFFER_SURFACE);

    if (normalBuffer)  normalBuffer->setFormat(GL_RGBA16F);
    if (surfaceBuffer) surfaceBuffer->setFormat(GL_RGBA8);

    if (!depthBuffer || !normalBuffer || !surfaceBuffer)
        MERROR("MPrepassStage::init - failed to create prepass buffers");
}

void MPrepassStage::cleanup(IRenderPipeline* const pipeline)
{
    if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
    attachedDepth = attachedNormal = attachedSurface = 0;

    depthBuffer   = nullptr;   // owned by the buffer registry
    normalBuffer  = nullptr;
    surfaceBuffer = nullptr;
    prepassShader = nullptr;   // owned by the asset manager
}

bool MPrepassStage::syncFramebuffer()
{
    if (!depthBuffer || !normalBuffer || !surfaceBuffer) return false;

    const GLuint depthTex   = depthBuffer->getDepthTextureHandle();
    const GLuint normalTex  = normalBuffer->getTextureHandle();
    const GLuint surfaceTex = surfaceBuffer->getTextureHandle();
    if (depthTex == 0 || normalTex == 0 || surfaceTex == 0) return false;

    if (fbo == 0) glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (depthBuffer->getStorageId()   == attachedDepth &&
        normalBuffer->getStorageId()  == attachedNormal &&
        surfaceBuffer->getStorageId() == attachedSurface)
        return true;

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, depthTex,   0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalTex,  0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, surfaceTex, 0);

    // Shader output 0 -> BUFFER_NORMAL, output 1 -> BUFFER_SURFACE.
    const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    attachedDepth   = depthBuffer->getStorageId();
    attachedNormal  = normalBuffer->getStorageId();
    attachedSurface = surfaceBuffer->getStorageId();

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        MERROR("MPrepassStage - framebuffer incomplete (status 0x" + std::to_string(status) + ")");
        attachedDepth = attachedNormal = attachedSurface = 0;   // retry next frame
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void MPrepassStage::preRender(IRenderPipeline* const pipeline) {}

void MPrepassStage::render(IRenderPipeline* const pipeline)
{
    // Leaves fbo bound on success.
    if (!syncFramebuffer()) return;

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));

    // ---- Clear --------------------------------------------------------------
    // Always, even without a shader, so BUFFER_OPAQUE never depth-tests
    // against last frame's depth.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    const GLfloat zero[4]  = { 0.f, 0.f, 0.f, 0.f };   // no geometry: a = 0
    const GLfloat farDepth = 1.f;
    glClearBufferfv(GL_COLOR, 0, zero);
    glClearBufferfv(GL_COLOR, 1, zero);
    glClearBufferfv(GL_DEPTH, 0, &farDepth);

    if (prepassShader)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        prepassShader->bind();
        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

        const SViewData viewData = resolveViewData(pipeline);
        SShaderPropertyValue viewVal, projVal;
        viewVal.setMat4Val(viewData.view);
        projVal.setMat4Val(viewData.projection);
        prepassShader->setPropertyValue("view",       viewVal);
        prepassShader->setPropertyValue("projection", projVal);

        int highestUnitUsed = -1;
        // Opaque only - transparents must not hide what is behind them.
        for (const SRenderItem& item : pipeline->getOpaqueItems())
        {
            // Same filter as MOpaqueStage - anything drawn here occludes
            // opaque geometry, so it must also be drawn there.
            if (item.vao == 0 || !item.material) continue;
            if (!item.cameraVisible) continue;

            if (item.isDoubleSided()) glDisable(GL_CULL_FACE);
            else                      glEnable(GL_CULL_FACE);

            SShaderPropertyValue modelVal;
            modelVal.setMat4Val(item.transform);
            prepassShader->setPropertyValue("model", modelVal);

            const int nextUnit = bindMaterialInputs(item, static_cast<unsigned int>(prog));
            highestUnitUsed = std::max(highestUnitUsed, nextUnit - 1);

            glBindVertexArray(item.vao);
            if (item.ebo != 0)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, item.ebo);
                glDrawElements(GL_TRIANGLES, item.indexCount, GL_UNSIGNED_INT, nullptr);
            }
            else
            {
                glDrawArrays(GL_TRIANGLES, 0, item.vertexCount);
            }
        }

        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);

        for (int unit = 0; unit <= highestUnitUsed; ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    pipeline->addCompositeFlag(ECF_Prepass);
}

int MPrepassStage::bindMaterialInputs(const SRenderItem& item, unsigned int program)
{
    const GLuint prog = program;

    // Defaults for materials that don't define these properties.
    GLint l;
    l = glGetUniformLocation(prog, "_useNormalMap"); if (l >= 0) glUniform1i(l, 0);
    l = glGetUniformLocation(prog, "_metallic");     if (l >= 0) glUniform1f(l, 0.0f);
    l = glGetUniformLocation(prog, "_smoothness");   if (l >= 0) glUniform1f(l, 0.5f);
    // Cutout: the loop below fills these from the material (and binds
    // _albedo, since the prepass shader samples it); reset so a cutout
    // material's values never carry over to the next item.
    l = glGetUniformLocation(prog, "_useCutout");       if (l >= 0) glUniform1i(l, 0);
    l = glGetUniformLocation(prog, "_cutoutThreshold"); if (l >= 0) glUniform1f(l, 0.5f);
    l = glGetUniformLocation(prog, "_useAlbedoTex");    if (l >= 0) glUniform1i(l, 1);
    l = glGetUniformLocation(prog, "_color");           if (l >= 0) glUniform4f(l, 1.f, 1.f, 1.f, 1.f);
    l = glGetUniformLocation(prog, "_lit");
    if (l >= 0) glUniform1i(l, item.getShadingMode() == MMaterial::ShadingMode::Lit ? 1 : 0);

    int texUnit = 0;
    for (const auto& [key, val] : item.material->getProperties())
    {
        if (val.getType() == SShaderPropertyType::Texture)
        {
            // Only bind textures the prepass shader actually samples
            // (_normalMap, and _albedo for cutout); others are skipped.
            const GLint loc = glGetUniformLocation(prog, key.c_str());
            if (loc < 0) continue;
            if (texUnit > MAX_MATERIAL_TEX_UNIT) continue;

            // Empty or missing -> white, never a stale texture.
            bool bound = false;
            if (!val.getTexAssetReference().empty())
            {
                const auto texAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
                    ->getAssetFromReference<MTextureAsset>(val.getTexAssetReference());
                if (texAsset && texAsset->getTexture())
                {
                    texAsset->bindForRendering(texUnit);
                    bound = true;
                }
            }
            if (!bound) MDefaultTextures::bindWhite(texUnit);
            glUniform1i(loc, texUnit);
            ++texUnit;
        }
        else
        {
            // Unknown names (e.g. _color) are ignored by the shader.
            prepassShader->setPropertyValue(key, val);
        }
    }
    return texUnit;
}

void MPrepassStage::postRender(IRenderPipeline* const pipeline) {}
