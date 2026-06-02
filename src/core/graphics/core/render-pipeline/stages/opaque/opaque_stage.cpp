//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "opaque_stage.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/lighting/ambient/ambient_light_gpu_struct.h"
#include "core/engine/lighting/directional/directional_light_gpu_struct.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
#include "core/graphics/core/render-pipeline/stages/lighting/light_shader_constants.h"
#include "core/utils/logger.h"

void MOpaqueStage::init(IRenderPipeline* const pipeline)
{
    opaqueBuffer = pipeline->getBufferRegistry()
                            .createBuffer<SFrameBuffer>(MBufferNames::BUFFER_OPAQUE);
    if (!opaqueBuffer)
        MERROR("MOpaqueStage::init failed to create opaque buffer");
}

void MOpaqueStage::cleanup(IRenderPipeline* const pipeline)
{
    opaqueBuffer = nullptr;
    destroyOverrideUBOs();
}

void MOpaqueStage::preRender(IRenderPipeline* const pipeline)
{
    const auto& lo = pipeline->getLightOverride();
    if (lo.active)
        applyLightOverride(lo);
    else
        MLightSystemManager::getInstance()->prepareLights();
}

void MOpaqueStage::render(IRenderPipeline* const pipeline)
{
    if (!opaqueBuffer || opaqueBuffer->getFBOHandle() == 0) return;

    opaqueBuffer->bindAsActive();

    const SVector2 res = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(res.x), static_cast<int>(res.y));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    // No glClear -- MClearStage handles clearing before skybox and geometry.

    // Camera -- use pipeline override if set, otherwise MViewManagement.
    glm::mat4 viewMat(1.f), projMat(1.f);
    const auto& camOverride = pipeline->getCameraOverride();
    if (camOverride.active)
    {
        viewMat = camOverride.view;
        projMat = camOverride.proj;
    }
    else
    {
        const MCameraEntity* camera = MViewManagement::getFirstActiveCamera();
        viewMat = camera ? camera->getViewMatrix()          : glm::mat4(1.f);
        projMat = camera ? camera->getProjectionMatrix(res) : glm::mat4(1.f);
    }

    const bool lightOverrideActive = pipeline->getLightOverride().active;

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

        item.material->bindMaterial();
        setMVP(item.transform);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        // When light override is active, skip dynamic lights -- the override
        // provides ambient + directional only, no point/spot lights.
        if (!lightOverrideActive)
        {
            AABB queryBounds  = item.bounds;
            queryBounds.min  -= SVector3(5.0f);
            queryBounds.max  += SVector3(5.0f);
            MLightSystemManager::getInstance()->prepareDynamicLights(queryBounds);
        }

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

// -- Light override UBO helpers -----------------------------------------------

void MOpaqueStage::applyLightOverride(const IRenderPipeline::SLightOverride& lo)
{
    // -- Ambient UBO at LIGHT_INDEX_AMBIENT -----------------------------------
    SAmbientLightData ambientData;
    ambientData.color     = SVector3(lo.ambientColor.x, lo.ambientColor.y, lo.ambientColor.z);
    ambientData.intensity = lo.ambientIntensity;
    ambientData.enabled   = 1;

    if (!overrideAmbientUBO)
    {
        glGenBuffers(1, &overrideAmbientUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, overrideAmbientUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(ambientData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, overrideAmbientUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ambientData), &ambientData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_AMBIENT,
                      overrideAmbientUBO, 0, sizeof(ambientData));

    // -- Directional UBO at LIGHT_INDEX_DIRECTIONAL ---------------------------
    SDirectionalLightData dirData;
    dirData.lightDirection = SVector3(lo.directionalDirection.x,
                                      lo.directionalDirection.y,
                                      lo.directionalDirection.z);
    dirData.lightColor     = SVector3(lo.directionalColor.x,
                                      lo.directionalColor.y,
                                      lo.directionalColor.z);
    dirData.lightIntensity = lo.directionalIntensity;
    dirData.enabled        = 1;

    if (!overrideDirectionalUBO)
    {
        glGenBuffers(1, &overrideDirectionalUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, overrideDirectionalUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(dirData), nullptr, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    glBindBuffer(GL_UNIFORM_BUFFER, overrideDirectionalUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(dirData), &dirData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, LIGHT_INDEX_DIRECTIONAL,
                      overrideDirectionalUBO, 0, sizeof(dirData));
}

void MOpaqueStage::destroyOverrideUBOs()
{
    if (overrideAmbientUBO)     { glDeleteBuffers(1, &overrideAmbientUBO);     overrideAmbientUBO     = 0; }
    if (overrideDirectionalUBO) { glDeleteBuffers(1, &overrideDirectionalUBO); overrideDirectionalUBO = 0; }
}