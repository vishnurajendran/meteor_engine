//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "opaque_stage.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/utils/logger.h"

static constexpr const char* ALBEDO_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/albedo_pass.mesl";

void MOpaqueStage::init(IRenderPipeline* const pipeline)
{
    opaqueBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer)
        MERROR("MOpaqueStage::init failed to create opaque buffer");

    // Store the path only — the raw MShader* is looked up fresh each frame
    // so it survives asset manager refresh() without dangling.
    if (MAssetManager::getInstance()->getAsset<MShaderAsset>(ALBEDO_SHADER_PATH))
        albedoShaderPath = ALBEDO_SHADER_PATH;
    else
        MERROR("MOpaqueStage::init could not find albedo shader: " + SString(ALBEDO_SHADER_PATH));
}

void MOpaqueStage::cleanup(IRenderPipeline* const pipeline)
{
    opaqueBuffer     = nullptr;
    albedoShaderPath = "";
}

void MOpaqueStage::preRender(IRenderPipeline* const pipeline)
{
    MLightSystemManager::getInstance()->prepareLights();
}

void MOpaqueStage::render(IRenderPipeline* const pipeline)
{
    // Resolve albedo shader fresh each frame — survives asset manager refresh().
    auto* albedoAsset = MAssetManager::getInstance()
                            ->getAsset<MShaderAsset>(albedoShaderPath.c_str());
    MShader* albedoShader = albedoAsset ? albedoAsset->getShader() : nullptr;

    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0 || !albedoShader) return;

    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MCameraEntity* camera = nullptr;
    for (auto* c : MViewManagement::getCameras())
        if (c && c->getEnabled()) { camera = c; break; }

    const glm::mat4 viewMat = camera ? camera->getViewMatrix()          : glm::mat4(1.f);
    const glm::mat4 projMat = camera ? camera->getProjectionMatrix(res) : glm::mat4(1.f);

    // Helper: set model/view/projection on whatever program is currently bound.
    // Works for both the albedo shader and any custom material shader.
    auto setMVP = [&](const glm::mat4& model)
    {
        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
        if (prog == 0) return;

        GLint loc;
        loc = glGetUniformLocation(prog, "model");
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));
        loc = glGetUniformLocation(prog, "view");
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMat));
        loc = glGetUniformLocation(prog, "projection");
        if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projMat));
    };

    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0 || !item.material) continue;

        if (item.getShadingMode() == MMaterial::ShadingMode::Unlit)
        {
            // Unlit — the material's own shader runs in full.
            // bindMaterial() calls glUseProgram + uploads all properties.
            // BUFFER_LIGHTS is cleared to white so the composite stage
            // produces:  materialOutput × white = materialOutput (correct).
            item.material->bindMaterial();
        }
        else
        {
            // Lit (deferred) — albedo-only pass. The lighting stage renders
            // the same geometry again into BUFFER_LIGHTS, then composite
            // multiplies:  albedo × lighting = final shaded colour.
            albedoShader->bind();
            for (auto& [key, val] : item.material->getProperties())
                albedoShader->setPropertyValue(key, val);
        }

        setMVP(item.transform);

        // Re-assert depth state before every draw — bindMaterial() may change
        // glDepthMask or glDepthFunc if the material's shader has side effects,
        // causing subsequent items to render incorrectly (e.g. plane over cube).
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

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
    opaqueBuffer->unbind();

    pipeline->addCompositeFlag(ECF_Opaque);
}

void MOpaqueStage::postRender(IRenderPipeline* const pipeline) {}