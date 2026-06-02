//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "opaque_stage.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/frame/frame_buffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/render-pipeline/stages/lighting/lighting_system_manager.h"
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
}

void MOpaqueStage::preRender(IRenderPipeline* const pipeline)
{
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

    // Use the pipeline-level camera override if set, otherwise fall back to
    // MViewManagement.  The override is used by secondary pipelines
    // (thumbnails, etc.) that supply their own camera matrices.
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