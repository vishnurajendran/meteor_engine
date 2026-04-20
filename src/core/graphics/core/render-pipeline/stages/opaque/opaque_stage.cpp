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

    auto* asset = MAssetManager::getInstance()->getAsset<MShaderAsset>(ALBEDO_SHADER_PATH);
    if (!asset)
        MERROR("MOpaqueStage::init could not load albedo shader: " + SString(ALBEDO_SHADER_PATH));
    else
        albedoShader = asset->getShader();
}

void MOpaqueStage::cleanup(IRenderPipeline* const pipeline)
{
    opaqueBuffer = nullptr;
    albedoShader = nullptr;
}

void MOpaqueStage::preRender(IRenderPipeline* const pipeline)
{
    MLightSystemManager::getInstance()->prepareLights();
}

void MOpaqueStage::render(IRenderPipeline* const pipeline)
{
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

    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0 || !item.material) continue;

        albedoShader->bind();

        for (auto& [key, val] : item.material->getProperties())
            albedoShader->setPropertyValue(key, val);

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        albedoShader->setPropertyValue("model", modelVal);

        if (camera)
        {
            SShaderPropertyValue viewVal, projVal;
            viewVal.setMat4Val(camera->getViewMatrix());
            projVal.setMat4Val(camera->getProjectionMatrix(res));
            albedoShader->setPropertyValue("view",       viewVal);
            albedoShader->setPropertyValue("projection", projVal);
        }

        // Expand bounds by a small margin so flat geometry (planes)
        // with a near-zero height AABB can still intersect light spheres.
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