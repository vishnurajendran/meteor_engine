//
// Created by Vishnu Rajendran on 2024-09-25.
//

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include "../lighting/lighting_system_manager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/gizmos/gizmos.h"
#include "core/graphics/core/material/material.h"
#include "core/graphics/core/render-pipeline/buffers/render/renderbuffer.h"
#include "core/graphics/core/render-pipeline/interfaces/render_pipeline_interface.h"
#include "core/graphics/core/render-pipeline/render_item.h"
#include "core/utils/logger.h"
#include "graphicsrenderer.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MGraphicsRenderer::init(IRenderPipeline* const pipeline)
{
    // Nothing to allocate — model uniform location is resolved per-draw
    // after each material binds its shader.
}

void MGraphicsRenderer::cleanup(IRenderPipeline* const pipeline)
{
    renderTarget = nullptr;
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void MGraphicsRenderer::preRender(IRenderPipeline* const pipeline)
{
    SRenderBuffer* buffer = pipeline->getRenderBuffer();
    if (!buffer)
    {
        MERROR("MGraphicsRenderer::preRender — render buffer is null");
        return;
    }

    renderTarget = buffer->getCoreRenderTarget();
    if (!renderTarget)
    {
        MERROR("MGraphicsRenderer::preRender — render target is null");
        return;
    }

    if (!renderTarget->setActive(true))
    {
        MERROR("MGraphicsRenderer::preRender — render target is not active");
        return;
    }

    const auto size = renderTarget->getSize();
    glViewport(0, 0, size.x, size.y);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MLightSystemManager::getInstance()->prepareLights();
}

void MGraphicsRenderer::render(IRenderPipeline* const pipeline)
{
    if (!renderTarget) return;

    // Re-activate the SFML render target to restore its internal FBO.
    // The depth stage may have bound and unbound a different FBO during render().
    renderTarget->setActive(true);

    // Upload the camera VP matrix to whichever shader the material activates.
    // We do this once per stage using the first active camera.
    MCameraEntity* camera = nullptr;
    const auto& cameras = MViewManagement::getCameras();
    for (auto* c : cameras)
    {
        if (c && c->getEnabled()) { camera = c; break; }
    }

    for (const SRenderItem& item : pipeline->getRenderItems())
    {
        if (item.vao == 0)  continue;
        if (!item.material) continue; // no material → skip in opaque pass

        // Bind shader + set all material properties (textures, colours, etc.).
        item.material->bindMaterial();

        // Set per-item uniforms on the now-active shader program.
        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
        if (prog != 0)
        {
            // Helper: try two uniform name variants (prefixed and unprefixed).
            auto loc = [prog](const char* a, const char* b) -> GLint {
                GLint l = glGetUniformLocation(prog, a);
                return l >= 0 ? l : glGetUniformLocation(prog, b);
            };

            // Model matrix — "model" or "u_model"
            GLint modelLoc = loc("model", "u_model");
            if (modelLoc >= 0)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(item.transform));

            // Camera matrices — "view"/"u_view", "projection"/"u_projection"
            if (camera)
            {
                const auto   sz  = renderTarget->getSize();
                const SVector2 res = { static_cast<float>(sz.x), static_cast<float>(sz.y) };

                const glm::mat4 viewMat = camera->getViewMatrix();
                const glm::mat4 projMat = camera->getProjectionMatrix(res);
                const glm::mat4 vpMat   = projMat * viewMat;

                GLint viewLoc = loc("view", "u_view");
                GLint projLoc = loc("projection", "u_projection");
                GLint vpLoc   = loc("viewProjection", "u_viewProjection");

                if (viewLoc >= 0)
                    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
                if (projLoc >= 0)
                    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projMat));
                if (vpLoc >= 0)
                    glUniformMatrix4fv(vpLoc,  1, GL_FALSE, glm::value_ptr(vpMat));
            }
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

    if (drawGizmos)
        MGizmos::requestGizmoDraws();

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        MERROR(SString("MGraphicsRenderer::render — GL error: ") + std::to_string(err));
}

void MGraphicsRenderer::postRender(IRenderPipeline* const pipeline)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindVertexArray(0);

    if (renderTarget)
        renderTarget->setActive(false);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MGraphicsRenderer::setModelUniform(const glm::mat4& model) const
{
    // Query the location from the currently bound program so this works
    // regardless of which shader the material activated.
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    if (prog == 0) return;

    GLint loc = glGetUniformLocation(prog, "u_model");
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));
}