#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include "lighting_stage.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/texture/texture.h"
#include "core/engine/texture/textureasset.h"
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
    "meteor_assets/engine_assets/shaders/internal/lighting_pass.mesl";

static constexpr int TEX_UNIT_DIR_SHADOW   = 0;
static constexpr int TEX_UNIT_SPOT_BASE    = 1;
static constexpr int TEX_UNIT_POINT_BASE   = 5;

// Material textures start here to avoid collisions with shadow map
// units (0-8). Most GPUs expose at least 16 fragment texture units.
static constexpr int TEX_UNIT_MAT_BASE     = 10;

void MLightingStage::init(IRenderPipeline* const pipeline)
{
    lightsBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SFrameBuffer>(MBufferNames::BUFFER_LIGHTS);
    if (lightsBuffer) lightsBuffer->setColorFormat(0x881A); // GL_RGBA16F

    auto handle = MAssetManager::getInstance()->getAsset<MShaderAsset>(LIGHTING_SHADER_PATH);
    if (handle)
        lightingShaderPath = LIGHTING_SHADER_PATH;
    else
        MERROR("MLightingStage::init -- could not find: " + SString(LIGHTING_SHADER_PATH));
}

void MLightingStage::cleanup(IRenderPipeline* const pipeline)
{
    lightsBuffer      = nullptr;
    lightingShaderPath = "";
}

void MLightingStage::preRender(IRenderPipeline* const pipeline) {}

void MLightingStage::render(IRenderPipeline* const pipeline)
{
    while (glGetError() != GL_NO_ERROR) {}

    if (!lightsBuffer || lightsBuffer->getFBOHandle() == 0) return;

    lightsBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    const int w = static_cast<int>(res.x);
    const int h = static_cast<int>(res.y);

    glViewport(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto* opaqueBuffer = pipeline->getBufferRegistry()
                                  .getBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (opaqueBuffer && opaqueBuffer->getFBOHandle() != 0)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, opaqueBuffer->getFBOHandle());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightsBuffer->getFBOHandle());
        glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    lightsBuffer->unbind();

    auto lightingAsset = MAssetManager::getInstance()
                              ->getAsset<MShaderAsset>(lightingShaderPath.c_str());
    MShader* lightingShader = lightingAsset ? lightingAsset->getShader() : nullptr;

    if (!lightingShader)
    {
        MWARN("MLightingStage::render -- lightingShader is null, rendering without lighting");
        return;
    }

    lightsBuffer->bindAsActive();
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);

    MCameraEntity* camera = nullptr;
    for (auto* c : MViewManagement::getCameras())
        if (c && c->getEnabled()) { camera = c; break; }

    auto* sb = pipeline->getBufferRegistry()
                         .getBuffer<SShadowBuffer>(MBufferNames::BUFFER_SHADOW);
    const bool hasShadow = sb && sb->getFBOHandle() != 0 &&
                           (pipeline->getCompositeFlags() & ECF_Shadow);

    // Per-light shadow settings via manager accessors (queries the entity).
    auto* mgr = MLightSystemManager::getInstance();
    const bool dirShadowEnabled = mgr->isDirectionalShadowEnabled();
    const bool dirSmoothShadow  = mgr->isDirectionalSmoothShadow();

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
        if (l >= 0) glUniform1i(l, (hasShadow && dirShadowEnabled) ? 1 : 0);

        l = glGetUniformLocation(prog, "smoothShadows");
        if (l >= 0) glUniform1i(l, dirSmoothShadow ? 1 : 0);
    }

    // ---- Spot shadow maps --------------------------------------------------
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

            l = glGetUniformLocation(prog, spotPosNames[i]);
            if (l >= 0 && i < sb->numSpotShadows)
                glUniform3fv(l, 1, glm::value_ptr(sb->spotLightPositions[i]));

            l = glGetUniformLocation(prog, spotFarNames2[i]);
            if (l >= 0 && i < sb->numSpotShadows)
                glUniform1f(l, sb->spotFarPlanes[i]);
        }
    }

    // ---- Point shadow cube maps --------------------------------------------
    static const char* pointMapNames[] = {"pointShadowMap0","pointShadowMap1","pointShadowMap2","pointShadowMap3"};
    static const char* pointFarNames[] = {"pointFarPlane0","pointFarPlane1","pointFarPlane2","pointFarPlane3"};

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

    // ---- Per-item draw -- lit items only ------------------------------------
    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0) continue;
        if (item.getShadingMode() == MMaterial::ShadingMode::Unlit) continue;

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        lightingShader->setPropertyValue("model", modelVal);

        // -- Reset material-specific uniforms to safe defaults ---------------
        // Items whose material lacks these properties (e.g. a different
        // shader) must not inherit values from the previous draw call.
        {
            GLint l;
            l = glGetUniformLocation(prog, "_useNormalMap");
            if (l >= 0) glUniform1i(l, 0);
            l = glGetUniformLocation(prog, "_metallic");
            if (l >= 0) glUniform1f(l, 0.0f);
            l = glGetUniformLocation(prog, "_smoothness");
            if (l >= 0) glUniform1f(l, 0.5f);
        }

        // -- Forward material properties to the lighting shader --------------
        // Non-texture properties go through setPropertyValue (sets the GL
        // uniform via glGetUniformLocation -- returns -1 and silently skips
        // for properties that the lighting shader does not declare).
        //
        // Texture properties are bound manually to units >= TEX_UNIT_MAT_BASE
        // so they do not overwrite shadow map units 0-8.  setPropertyValue
        // for textures would bind to unit 0 (its hardcoded default), which
        // would clobber the directional shadow map.
        int matTexUnit = TEX_UNIT_MAT_BASE;

        if (item.material)
        {
            for (const auto& [key, val] : item.material->getProperties())
            {
                if (val.getType() == SShaderPropertyType::Texture)
                {
                    if (val.getTexAssetReference().empty()) continue;

                    const auto texAsset = MAssetManager::getInstance()
                        ->getAsset<MTextureAsset>(val.getTexAssetReference());
                    if (!texAsset || !texAsset->getTexture()) continue;

                    glActiveTexture(GL_TEXTURE0 + matTexUnit);
                    glBindTexture(GL_TEXTURE_2D,
                                  texAsset->getTexture()->getTextureID());

                    GLint loc = glGetUniformLocation(prog, key.c_str());
                    if (loc >= 0)
                        glUniform1i(loc, matTexUnit);

                    matTexUnit++;
                }
                else
                {
                    lightingShader->setPropertyValue(key, val);
                }
            }
        }

        AABB queryBounds  = item.bounds;
        queryBounds.min  -= SVector3(5.0f);
        queryBounds.max  += SVector3(5.0f);
        mgr->prepareDynamicLights(queryBounds);

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

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Unbind shadow map units (0-8) and material texture units (10+).
    for (int i = 0; i < 16; ++i)
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