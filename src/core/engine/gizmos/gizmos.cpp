//
// Created by ssj5v on 22-04-2025.
//

#include "GL/glew.h"
#include "gizmos.h"
#include <cmath>
#include "GL/gl.h"
#include "core/application/application.h"

#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/subsystem/subsystem_registry.h"
#include "core/engine/texture/texture.h"
#include "core/graphics/core/render-pipeline/render_pipeline.h"
#include "core/graphics/core/render-pipeline/render_pipeline_manager.h"
#include "core/graphics/core/shader/shaderasset.h"


// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

unsigned int   MGizmos::uiQuadVAO      = 0;
unsigned int   MGizmos::uiLineVAO      = 0;
unsigned int   MGizmos::uiLineVBO      = 0;
bool           MGizmos::gizmosEnabled  = false;
bool           MGizmos::batchingActive = false;
MApplication*  MGizmos::appInst        = nullptr;

std::vector<MGizmos::SGizmoBatchEntry> MGizmos::pendingBatches;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

MCameraEntity* MGizmos::getActiveCamera()
{
    return MViewManagement::getFirstActiveCamera();
}

SVector2 MGizmos::getResolution()
{
    auto* pipeline = MEngineSubsystemRegistry::getSubsystem<IRenderPipelineManagerSubsystem>();
    if (pipeline == nullptr) return SVector2(0);
    return pipeline->getRenderResolution();
}

void MGizmos::buildTangentFrame(const SVector3& n, SVector3& outTangent, SVector3& outBitangent)
{
    const SVector3 ref = (glm::abs(n.x) < 0.9f)
                         ? SVector3(1.0f, 0.0f, 0.0f)
                         : SVector3(0.0f, 1.0f, 0.0f);

    outTangent   = glm::normalize(glm::cross(n, ref));
    outBitangent = glm::normalize(glm::cross(n, outTangent));
}

// ---------------------------------------------------------------------------
// Batch management
// ---------------------------------------------------------------------------

MGizmos::SGizmoBatchEntry* MGizmos::findOrCreateBatch(const SColor& color, float thickness, bool ignoreZDepth)
{
    for (auto& entry : pendingBatches)
    {
        if (entry.ignoreZDepth == ignoreZDepth &&
            entry.thickness    == thickness    &&
            entry.color.r      == color.r      &&
            entry.color.g      == color.g      &&
            entry.color.b      == color.b      &&
            entry.color.a      == color.a)
        {
            return &entry;
        }
    }

    pendingBatches.push_back({ color, thickness, ignoreZDepth, {} });
    return &pendingBatches.back();
}

void MGizmos::flushBatches()
{
    if (pendingBatches.empty()) return;

    if (!uiLineVAO || !uiLineVBO)
        createLineVAO();

    auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/line.mesl");
    if (!shaderAsset) return;

    auto lineShader = shaderAsset->getShader();
    lineShader->bind();

    // Upload the view-projection matrix once — it is shared by every batch.
    auto camera  = getActiveCamera();
    auto viewProj = camera->getProjectionMatrix(getResolution()) * camera->getViewMatrix();

    SShaderPropertyValue mvpProp;
    mvpProp.setMat4Val(viewProj);
    lineShader->setPropertyValue("uViewProj", mvpProp);

    glBindVertexArray(uiLineVAO);

    // ---- Pass 1: depth-tested batches -------------------------------------
    // The depth test is already enabled by the gizmo stage — we do not touch
    // it here so we don't fight whatever state the stage has set.
    for (auto& entry : pendingBatches)
    {
        if (entry.ignoreZDepth || entry.vertices.empty()) continue;

        SShaderPropertyValue colorProp;
        colorProp.setVec4Val(SVector4(entry.color.r, entry.color.g, entry.color.b, entry.color.a));
        lineShader->setPropertyValue("_color", colorProp);

        // glBufferData orphans the old storage and allocates fresh storage for
        // the new size — the correct streaming pattern for variable-size uploads.
        glBindBuffer(GL_ARRAY_BUFFER, uiLineVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(sizeof(glm::vec3) * entry.vertices.size()),
                     entry.vertices.data(),
                     GL_DYNAMIC_DRAW);

        glLineWidth(entry.thickness);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(entry.vertices.size()));
    }

    // ---- Pass 2: depth-ignoring batches -----------------------------------
    // Draw these after opaque geometry so they always appear on top.
    bool hasDepthIgnored = false;
    for (const auto& entry : pendingBatches)
        if (entry.ignoreZDepth && !entry.vertices.empty()) { hasDepthIgnored = true; break; }

    if (hasDepthIgnored)
    {
        glDisable(GL_DEPTH_TEST);

        for (auto& entry : pendingBatches)
        {
            if (!entry.ignoreZDepth || entry.vertices.empty()) continue;

            SShaderPropertyValue colorProp;
            colorProp.setVec4Val(SVector4(entry.color.r, entry.color.g, entry.color.b, entry.color.a));
            lineShader->setPropertyValue("_color", colorProp);

            glBindBuffer(GL_ARRAY_BUFFER, uiLineVBO);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(sizeof(glm::vec3) * entry.vertices.size()),
                         entry.vertices.data(),
                         GL_DYNAMIC_DRAW);

            glLineWidth(entry.thickness);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(entry.vertices.size()));
        }

        glEnable(GL_DEPTH_TEST);
    }

    glBindVertexArray(0);

    // Release CPU memory. The std::vector<SVector3> inside each entry is freed
    // by the destructor — no manual vertex data lifetime management needed.
    pendingBatches.clear();
}

// ---------------------------------------------------------------------------
// drawLine — public entry point
// ---------------------------------------------------------------------------

void MGizmos::drawLine(const SVector3& start, const SVector3& end, const SColor& color,
                        const float& thickness, bool ignoreZDepth)
{
    if (batchingActive)
    {
        // Batching path — no GL calls here. findOrCreateBatch may push_back on
        // pendingBatches, invalidating any previously returned pointer, which is
        // why findOrCreateBatch is always called fresh rather than cached.
        auto* entry = findOrCreateBatch(color, thickness, ignoreZDepth);
        entry->vertices.push_back(start);
        entry->vertices.push_back(end);
        return;
    }

    // Immediate fallback — used when drawLine is called outside a gizmo traversal
    // (e.g. editor overlays, debug tools, drawRay called from non-entity code).
    drawLineImmediate(start, end, color, thickness, ignoreZDepth);
}

void MGizmos::drawLineImmediate(const SVector3& start, const SVector3& end,
                                  const SColor& color, const float& thickness, bool ignoreZDepth)
{
    if (!uiLineVAO || !uiLineVBO)
        createLineVAO();

    auto shaderAsset = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()
        ->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/line.mesl");
    if (!shaderAsset) return;

    auto lineShader = shaderAsset->getShader();
    glm::vec3 lineVerts[2] = { start, end };

    glBindBuffer(GL_ARRAY_BUFFER, uiLineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVerts), lineVerts);

    lineShader->bind();

    auto camera  = getActiveCamera();
    auto viewProj = camera->getProjectionMatrix(getResolution()) * camera->getViewMatrix();

    SShaderPropertyValue mvpProp;
    mvpProp.setMat4Val(viewProj);
    lineShader->setPropertyValue("uViewProj", mvpProp);

    SShaderPropertyValue colorProp;
    colorProp.setVec4Val(SVector4(color.r, color.g, color.b, color.a));
    lineShader->setPropertyValue("_color", colorProp);

    if (ignoreZDepth) glDisable(GL_DEPTH_TEST);

    glBindVertexArray(uiLineVAO);
    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);

    if (ignoreZDepth) glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// Wire shapes — all funnel through drawLine, so they batch automatically
// ---------------------------------------------------------------------------

void MGizmos::drawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture)
{
    auto uiShader = MEngineSubsystemRegistry::getSubsystem<IAssetManagerSubsystem>()->
    getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/internal/gizmo.mesl");
    if (!texture || !uiShader)
    {
        MLOG("Shader or Texture NULL");
        return;
    };

    if (!uiQuadVAO)
        uiQuadVAO = createQuadVAO();

    auto camera = getActiveCamera();

    SVector2 resolution = getResolution();
    SMatrix4 proj = camera->getProjectionMatrix(resolution);
    SMatrix4 view = camera->getViewMatrix();

    SMatrix4 model(1.0f);
    model = glm::translate(model, position);
    model *= SMatrix4(glm::mat3(glm::transpose(view)));
    model = glm::scale(model, SVector3(halfExtents.x * 2.0f, halfExtents.y * 2.0f, 1.0f));

    SMatrix4 mvp = proj * view * model;

    uiShader->getShader()->bind();

    SShaderPropertyValue uMVP;
    uMVP.setMat4Val(mvp);
    uiShader->getShader()->setPropertyValue("uMVP", uMVP);

    texture->bind(0, 0);

    glBindVertexArray(uiQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void MGizmos::drawWireCube(SVector3 position, SVector3 halfExtents, SColor color, float thickness, SQuaternion rotation)
{
    const SVector3 localCorners[8] = {
        {-halfExtents.x, -halfExtents.y, -halfExtents.z},
        { halfExtents.x, -halfExtents.y, -halfExtents.z},
        { halfExtents.x,  halfExtents.y, -halfExtents.z},
        {-halfExtents.x,  halfExtents.y, -halfExtents.z},
        {-halfExtents.x, -halfExtents.y,  halfExtents.z},
        { halfExtents.x, -halfExtents.y,  halfExtents.z},
        { halfExtents.x,  halfExtents.y,  halfExtents.z},
        {-halfExtents.x,  halfExtents.y,  halfExtents.z}
    };

    SVector3 corners[8];
    for (int i = 0; i < 8; ++i)
        corners[i] = position + rotation * localCorners[i];

    drawLine(corners[0], corners[1], color, thickness, false);
    drawLine(corners[1], corners[2], color, thickness, false);
    drawLine(corners[2], corners[3], color, thickness, false);
    drawLine(corners[3], corners[0], color, thickness, false);

    drawLine(corners[4], corners[5], color, thickness, false);
    drawLine(corners[5], corners[6], color, thickness, false);
    drawLine(corners[6], corners[7], color, thickness, false);
    drawLine(corners[7], corners[4], color, thickness, false);

    drawLine(corners[0], corners[4], color, thickness, false);
    drawLine(corners[1], corners[5], color, thickness, false);
    drawLine(corners[2], corners[6], color, thickness, false);
    drawLine(corners[3], corners[7], color, thickness, false);
}

void MGizmos::drawWireSphere(SVector3 position, float radius, SColor color, float thickness, glm::quat rotation)
{
    const int segments = 32;

    for (int i = 0; i < segments; ++i)
    {
        float theta1 = (float)i       / segments * glm::two_pi<float>();
        float theta2 = (float)(i + 1) / segments * glm::two_pi<float>();

        drawLine(position + rotation * SVector3(radius * cos(theta1), radius * sin(theta1), 0.0f),
                 position + rotation * SVector3(radius * cos(theta2), radius * sin(theta2), 0.0f),
                 color, thickness, false);

        drawLine(position + rotation * SVector3(0.0f, radius * cos(theta1), radius * sin(theta1)),
                 position + rotation * SVector3(0.0f, radius * cos(theta2), radius * sin(theta2)),
                 color, thickness, false);

        drawLine(position + rotation * SVector3(radius * cos(theta1), 0.0f, radius * sin(theta1)),
                 position + rotation * SVector3(radius * cos(theta2), 0.0f, radius * sin(theta2)),
                 color, thickness, false);
    }
}

void MGizmos::drawWireCircle(const SVector3& center, float radius, const SVector3& normal,
                              const SColor& color, float thickness, int segments)
{
    const SVector3 n = glm::normalize(normal);
    SVector3 tangent, bitangent;
    buildTangentFrame(n, tangent, bitangent);

    const float step = glm::two_pi<float>() / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
        const float a1 = i       * step;
        const float a2 = (i + 1) * step;
        drawLine(center + radius * (std::cos(a1) * tangent + std::sin(a1) * bitangent),
                 center + radius * (std::cos(a2) * tangent + std::sin(a2) * bitangent),
                 color, thickness, false);
    }
}

void MGizmos::drawWireCylinder(const SVector3& center, float halfHeight, float radius,
                                const SVector3& axisDir, const SColor& color, float thickness)
{
    const SVector3 axis         = glm::normalize(axisDir);
    const SVector3 topCenter    = center + axis * halfHeight;
    const SVector3 bottomCenter = center - axis * halfHeight;

    drawWireCircle(topCenter,    radius, axis, color, thickness);
    drawWireCircle(bottomCenter, radius, axis, color, thickness);

    SVector3 tangent, bitangent;
    buildTangentFrame(axis, tangent, bitangent);

    for (int i = 0; i < 4; ++i)
    {
        const float angle  = i * glm::half_pi<float>();
        const SVector3 off = radius * (std::cos(angle) * tangent + std::sin(angle) * bitangent);
        drawLine(topCenter + off, bottomCenter + off, color, thickness, false);
    }
}

void MGizmos::drawWireCapsule(const SVector3& center, float halfHeight, float radius,
                               const SVector3& axisDir, const SColor& color, float thickness)
{
    const SVector3 axis           = glm::normalize(axisDir);
    const SVector3 topJunction    = center + axis * halfHeight;
    const SVector3 bottomJunction = center - axis * halfHeight;

    SVector3 tangent, bitangent;
    buildTangentFrame(axis, tangent, bitangent);

    // Cylindrical section
    drawWireCircle(topJunction,    radius, axis, color, thickness);
    drawWireCircle(bottomJunction, radius, axis, color, thickness);

    for (int i = 0; i < 4; ++i)
    {
        const float angle  = i * glm::half_pi<float>();
        const SVector3 off = radius * (std::cos(angle) * tangent + std::sin(angle) * bitangent);
        drawLine(topJunction + off, bottomJunction + off, color, thickness, false);
    }

    // Hemisphere arcs — two perpendicular semicircles per cap
    const int arcSeg = 16;
    for (int i = 0; i < arcSeg; ++i)
    {
        const float a1 = (float)i       / arcSeg * glm::pi<float>();
        const float a2 = (float)(i + 1) / arcSeg * glm::pi<float>();

        // Top cap — tangent plane
        drawLine(topJunction + radius * (std::cos(a1) * tangent   + std::sin(a1) * axis),
                 topJunction + radius * (std::cos(a2) * tangent   + std::sin(a2) * axis),
                 color, thickness, false);
        // Top cap — bitangent plane
        drawLine(topJunction + radius * (std::cos(a1) * bitangent + std::sin(a1) * axis),
                 topJunction + radius * (std::cos(a2) * bitangent + std::sin(a2) * axis),
                 color, thickness, false);
        // Bottom cap — tangent plane (axis flipped)
        drawLine(bottomJunction + radius * (std::cos(a1) * tangent   - std::sin(a1) * axis),
                 bottomJunction + radius * (std::cos(a2) * tangent   - std::sin(a2) * axis),
                 color, thickness, false);
        // Bottom cap — bitangent plane (axis flipped)
        drawLine(bottomJunction + radius * (std::cos(a1) * bitangent - std::sin(a1) * axis),
                 bottomJunction + radius * (std::cos(a2) * bitangent - std::sin(a2) * axis),
                 color, thickness, false);
    }
}

void MGizmos::drawRay(const SVector3& origin, const SVector3& direction, const float& length,
                       const SColor& color, const float& thickness)
{
    glm::vec3 end  = origin + direction * length;
    glm::vec3 dir  = glm::normalize(direction);
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0))) * (length * 0.1f);
    glm::vec3 up    = glm::normalize(glm::cross(right, dir)) * (length * 0.1f);

    drawLine(origin, end, color, thickness);
    drawLine(end, end - dir * 0.2f + right, color, thickness);
    drawLine(end, end - dir * 0.2f - right, color, thickness);
    drawLine(end, end - dir * 0.2f + up,    color, thickness);
    drawLine(end, end - dir * 0.2f - up,    color, thickness);
}

void MGizmos::drawWireFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                               SColor color, float thickness)
{
    const glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);

    const glm::vec4 ndcCorners[8] = {
        {-1, -1, -1, 1}, { 1, -1, -1, 1}, { 1,  1, -1, 1}, {-1,  1, -1, 1},
        {-1, -1,  1, 1}, { 1, -1,  1, 1}, { 1,  1,  1, 1}, {-1,  1,  1, 1}
    };

    SVector3 c[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 w = invVP * ndcCorners[i];
        w /= w.w;
        c[i] = SVector3(w.x, w.y, w.z);
    }

    drawLine(c[0], c[1], color, thickness, false); drawLine(c[1], c[2], color, thickness, false);
    drawLine(c[2], c[3], color, thickness, false); drawLine(c[3], c[0], color, thickness, false);
    drawLine(c[4], c[5], color, thickness, false); drawLine(c[5], c[6], color, thickness, false);
    drawLine(c[6], c[7], color, thickness, false); drawLine(c[7], c[4], color, thickness, false);

    for (int i = 0; i < 4; ++i)
        drawLine(c[i], c[i + 4], color, thickness, false);
}

// ---------------------------------------------------------------------------
// VAO / VBO helpers
// ---------------------------------------------------------------------------

unsigned int MGizmos::createQuadVAO()
{
    float quadVertices[] = {
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void MGizmos::createLineVAO()
{
    glGenVertexArrays(1, &uiLineVAO);
    glGenBuffers(1, &uiLineVBO);

    glBindVertexArray(uiLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiLineVBO);

    // Allocate 2 vertices of initial storage so glBufferSubData in the
    // immediate fallback path has valid storage to write into.
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// Gizmo dispatch
// ---------------------------------------------------------------------------

void MGizmos::enableGizmos(bool enable)
{
    gizmosEnabled = enable;
}

void MGizmos::requestGizmoDraws()
{
    if (!gizmosEnabled) return;

#if METEOR_EDITOR
    auto sceneManager = MSceneManager::getSceneManagerInstance();
    if (!sceneManager) return;

    auto scene = sceneManager->getActiveScene();
    if (!scene) return;

    // Open the batch — all drawLine calls during the traversal accumulate
    // CPU-side rather than submitting GL calls.
    pendingBatches.clear();
    batchingActive = true;

    for (auto root : scene->getRootEntities())
        recursiveGizmoDraws(root);

    // Close the batch and submit everything to the GPU in one pass.
    batchingActive = false;
    flushBatches();
#endif
}

void MGizmos::recursiveGizmoDraws(MSpatialEntity* entity)
{
    if (!gizmosEnabled) return;

#if METEOR_EDITOR
    if (!entity) return;

    if (entity->getEnabled() && entity->isEnabledInHierarchy())
    {
        entity->onDrawGizmo(getResolution());
        for (auto child : entity->getChildren())
            recursiveGizmoDraws(child);
    }

#endif
}