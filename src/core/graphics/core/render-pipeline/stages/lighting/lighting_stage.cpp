#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "lighting_stage.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/buffers/shadow/shadow_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"

static constexpr const char* LIGHTING_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/lighting_pass.mesl";

// Texture unit layout (must match lighting_pass.mesl):
//   0  = directional shadow map
//   1..4  = spot shadow maps [0..3]
//   5..8  = point shadow cube maps [0..3]
static constexpr int TEX_UNIT_DIR_SHADOW   = 0;
static constexpr int TEX_UNIT_SPOT_BASE    = 1;   // 1..4
static constexpr int TEX_UNIT_POINT_BASE   = 5;   // 5..8

void MLightingStage::init(IRenderPipeline* const pipeline)
{
    lightsBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (lightsBuffer) lightsBuffer->setColorFormat(0x881A); // GL_RGBA16F

    auto* asset = MAssetManager::getInstance()->getAsset<MShaderAsset>(LIGHTING_SHADER_PATH);
    if (!asset)
        MERROR("MLightingStage::init — could not load: " + SString(LIGHTING_SHADER_PATH));
    else
        lightingShader = asset->getShader();
}

void MLightingStage::cleanup(IRenderPipeline* const pipeline)
{
    lightsBuffer   = nullptr;
    lightingShader = nullptr;
}

void MLightingStage::preRender(IRenderPipeline* const pipeline) {}

void MLightingStage::render(IRenderPipeline* const pipeline)
{
    // Flush any GL errors from previous stages so they don't corrupt our draws.
    while (glGetError() != GL_NO_ERROR) {}

    if (!lightsBuffer || lightsBuffer->getFBOHandle() == 0) return;

    // Always clear to white first — if the shader is missing or fails, the
    // composite stage multiplies albedo × white = albedo (unlit but visible).
    lightsBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    lightsBuffer->unbind();

    if (!lightingShader)
    {
        MWARN("MLightingStage::render — lightingShader is null, rendering without lighting");
        // ECF_Lights not set → composite falls back to opaque-only blit.
        return;
    }

    lightsBuffer->bindAsActive();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    MCameraEntity* camera = nullptr;
    for (auto* c : MViewManagement::getCameras())
        if (c && c->getEnabled()) { camera = c; break; }

    // ---- Fetch shadow buffer ------------------------------------------------
    auto* sb = pipeline->getBufferRegistry()
                         .getBuffer<SShadowBuffer>(MBufferNames::BUFFER_SHADOW);
    const bool hasShadow = sb && sb->getFBOHandle() != 0 &&
                           (pipeline->getCompositeFlags() & ECF_Shadow);

    // ---- Bind shader and set per-frame uniforms ----------------------------
    lightingShader->bind();

    if (camera)
    {
        SShaderPropertyValue viewVal, projVal;
        viewVal.setMat4Val(camera->getViewMatrix());
        projVal.setMat4Val(camera->getProjectionMatrix(res));
        lightingShader->setPropertyValue("view",       viewVal);
        lightingShader->setPropertyValue("projection", projVal);
    }

    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

    // ---- Directional shadow ------------------------------------------------
    if (hasShadow)
    {
        glActiveTexture(GL_TEXTURE0 + TEX_UNIT_DIR_SHADOW);
        glBindTexture(GL_TEXTURE_2D, sb->getDepthTextureHandle());
    }
    {
        GLint l;
        l = glGetUniformLocation(prog, "shadowMap");
        if (l >= 0) glUniform1i(l, TEX_UNIT_DIR_SHADOW);
        l = glGetUniformLocation(prog, "lightSpaceMatrix");
        if (l >= 0 && hasShadow)
            glUniformMatrix4fv(l, 1, GL_FALSE, glm::value_ptr(sb->lightSpaceMatrix));
        l = glGetUniformLocation(prog, "hasShadow");
        if (l >= 0) glUniform1i(l, hasShadow ? 1 : 0);

        l = glGetUniformLocation(prog, "hasDirShadow");
        if (l >= 0) glUniform1i(l,
            (hasShadow && MLightSystemManager::getInstance()->directionalShadowEnabled) ? 1 : 0);

        l = glGetUniformLocation(prog, "smoothShadows");
        if (l >= 0) glUniform1i(l, MLightSystemManager::getInstance()->smoothShadows ? 1 : 0);
    }

    // ---- Spot shadow maps — individual uniform names (no array indexing) ----
    static const char* spotMapNames[]    = {"spotShadowMap0","spotShadowMap1","spotShadowMap2","spotShadowMap3"};
    static const char* spotMatrixNames[] = {"spotLightSpaceMatrix0","spotLightSpaceMatrix1","spotLightSpaceMatrix2","spotLightSpaceMatrix3"};
    static const char* spotPosNames[]    = {"spotLightPos0","spotLightPos1","spotLightPos2","spotLightPos3"};
    static const char* spotFarNames2[]   = {"spotFarPlane0","spotFarPlane1","spotFarPlane2","spotFarPlane3"};

    if (hasShadow)
    {
        for (int i = 0; i < SShadowBuffer::MAX_SPOT_SHADOWS; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + TEX_UNIT_SPOT_BASE + i);
            glBindTexture(GL_TEXTURE_2D,
                          (i < sb->numSpotShadows) ? sb->spotDepthTextures[i] : 0);

            GLint l = glGetUniformLocation(prog, spotMapNames[i]);
            if (l >= 0) glUniform1i(l, TEX_UNIT_SPOT_BASE + i);

            l = glGetUniformLocation(prog, spotMatrixNames[i]);
            if (l >= 0 && i < sb->numSpotShadows)
                glUniformMatrix4fv(l, 1, GL_FALSE,
                                   glm::value_ptr(sb->spotLightSpaceMatrices[i]));

            // Linear depth uniforms — position and far plane for distance comparison.
            l = glGetUniformLocation(prog, spotPosNames[i]);
            if (l >= 0 && i < sb->numSpotShadows)
                glUniform3fv(l, 1, glm::value_ptr(sb->spotLightPositions[i]));

            l = glGetUniformLocation(prog, spotFarNames2[i]);
            if (l >= 0 && i < sb->numSpotShadows)
                glUniform1f(l, sb->spotFarPlanes[i]);
        }
    }

    // ---- Point shadow cube maps — individual uniform names -----------------
    static const char* pointMapNames[]    = {"pointShadowMap0","pointShadowMap1","pointShadowMap2","pointShadowMap3"};
    static const char* pointFarNames[]    = {"pointFarPlane0","pointFarPlane1","pointFarPlane2","pointFarPlane3"};

    if (hasShadow)
    {
        for (int i = 0; i < SShadowBuffer::MAX_POINT_SHADOWS; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + TEX_UNIT_POINT_BASE + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP,
                          (i < sb->numPointShadows) ? sb->pointCubemapTextures[i] : 0);

            GLint l = glGetUniformLocation(prog, pointMapNames[i]);
            if (l >= 0) glUniform1i(l, TEX_UNIT_POINT_BASE + i);

            l = glGetUniformLocation(prog, pointFarNames[i]);
            if (l >= 0 && i < sb->numPointShadows)
                glUniform1f(l, sb->pointFarPlanes[i]);
        }
    }

    // ---- Per-item draw -----------------------------------------------------
    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0) continue;

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        lightingShader->setPropertyValue("model", modelVal);

        // Expand bounds for flat geometry (planes) so nearby lights aren't missed.
        AABB queryBounds  = item.bounds;
        queryBounds.min  -= SVector3(5.0f);
        queryBounds.max  += SVector3(5.0f);
        MLightSystemManager::getInstance()->prepareDynamicLights(queryBounds);

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

    // Unbind all shadow textures.
    for (int i = 0; i < 9; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D,       0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    glActiveTexture(GL_TEXTURE0);

    lightsBuffer->unbind();
    pipeline->addCompositeFlag(ECF_Lights);
}

void MLightingStage::postRender(IRenderPipeline* const pipeline) {}