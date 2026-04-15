//
// Created by ssj5v on 15-04-2026.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/graphics/core/render-pipeline/buffer_registry.h"
#include "core/graphics/core/render-pipeline/buffers/buffer_names.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/graphics/core/shader/shader.h"
#include "core/graphics/core/shader/shaderasset.h"
#include "core/utils/logger.h"
#include "depth_render_stage.h"

static constexpr const char* DEPTH_SHADER_PATH =
    "meteor_assets/engine_assets/shaders/depth.mesl";

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MDepthRenderStage::init(IRenderPipeline* const pipeline)
{
    // Load depth shader from asset — no GLSL in engine source.
    auto* shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>(DEPTH_SHADER_PATH);
    if (!shaderAsset)
    {
        MERROR("MDepthRenderStage::init — could not load depth shader: " + SString(DEPTH_SHADER_PATH));
        return;
    }
    depthShader = shaderAsset->getShader();

    // Register the depth buffer slot; FBO allocated when setRenderBuffer() fires.
    depthBuffer = pipeline->getBufferRegistry()
                           .createBuffer<SDepthBuffer>(MBufferNames::BUFFER_DEPTH);
    if (!depthBuffer)
        MERROR("MDepthRenderStage::init — failed to create depth buffer slot");
}

void MDepthRenderStage::cleanup(IRenderPipeline* const pipeline)
{
    pipeline->getBufferRegistry(); // registry destructor handles the buffer
    depthBuffer = nullptr;
    depthShader = nullptr;         // owned by asset manager
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void MDepthRenderStage::preRender(IRenderPipeline* const pipeline)
{
    // All GL state is set and restored inside render() — nothing leaks out.
}

void MDepthRenderStage::render(IRenderPipeline* const pipeline)
{
    if (!depthBuffer || depthBuffer->getFBOHandle() == 0 || !depthShader) return;

    // ---- Bind depth FBO and set depth-only state ---------------------------
    depthBuffer->bindAsActive();

    resolution = pipeline->getRenderResolution();
    glViewport(0, 0, static_cast<int>(resolution.x), static_cast<int>(resolution.y));

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glClear(GL_DEPTH_BUFFER_BIT);

    // ---- Upload camera matrices via the shader property system -------------
    depthShader->bind();

    MCameraEntity* camera = nullptr;
    for (auto* c : MViewManagement::getCameras())
    {
        if (c && c->getEnabled()) { camera = c; break; }
    }

    if (camera)
    {
        SShaderPropertyValue viewVal, projVal;
        viewVal.setMat4Val(camera->getViewMatrix());
        projVal.setMat4Val(camera->getProjectionMatrix(resolution));

        depthShader->setPropertyValue("view",       viewVal);
        depthShader->setPropertyValue("projection", projVal);
    }

    // ---- Draw each item — set model matrix, bind VAO, draw -----------------
    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0) continue;

        SShaderPropertyValue modelVal;
        modelVal.setMat4Val(item.transform);
        depthShader->setPropertyValue("model", modelVal);

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

    // ---- Restore state and unbind ------------------------------------------
    glBindVertexArray(0);
    glUseProgram(0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    depthBuffer->unbind();

    pipeline->addCompositeFlag(ECF_Depth);
}

void MDepthRenderStage::postRender(IRenderPipeline* const pipeline)
{
    // State fully restored in render(); nothing to do here.
}