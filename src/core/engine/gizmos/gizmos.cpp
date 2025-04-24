//
// Created by ssj5v on 22-04-2025.
//

#include "GL/glew.h"
#include "GL/gl.h"
#include "gizmos.h"
#include "core/engine/3d/shader/shaderasset.h"
#include "core/engine/assetmanagement/assetmanager/assetmanager.h"
#include "core/engine/camera/viewmanagement.h"
#include "core/engine/scene/scene.h"
#include "core/engine/scene/scenemanager.h"
#include "core/engine/texture/texture.h"
#include "core/graphics/core/graphicsrenderer.h"

unsigned int MGizmos::uiQuadVAO = 0;
unsigned int MGizmos::uiLineVAO = 0;
unsigned int MGizmos::uiLineVBO = 0;

MCameraEntity* MGizmos::getActiveCamera()
{
    auto& cameras = MViewManagement::getCameras();
    if (cameras.empty())
        return nullptr;

    for (auto camera : cameras)
    {
        if (camera != nullptr && camera->getEnabled())
            return camera;
    }

    return nullptr;
}

SVector2 MGizmos::getResolution() { return MGraphicsRenderer::getResolution(); }

void MGizmos::drawLine(const SVector3& start, const SVector3& end, const SColor& color, const float& thickness, bool ignoreZDepth=true)
{
    if (!uiLineVAO || !uiLineVBO)
        createLineVAO();

    auto shaderAsset = MAssetManager::getInstance()->getAsset<MShaderAsset>(
        "meteor_assets/engine_assets/shaders/line.mesl");

    if (shaderAsset == nullptr)
        return;

    auto lineShader =  shaderAsset->getShader();
    glm::vec3 lineVerts[2] = { start, end };

    glBindBuffer(GL_ARRAY_BUFFER, uiLineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVerts), lineVerts);

    lineShader->bind();

    auto camera = getActiveCamera();
    auto proj = camera->getProjectionMatrix(getResolution());
    auto view = camera->getViewMatrix();
    auto viewProj = proj * view;

    SShaderPropertyValue mvpProp;
    mvpProp.setMat4Val(viewProj);

    SShaderPropertyValue colorProp;
    colorProp.setVec4Val(SVector4(color.a, color.g, color.b, color.a));

    lineShader->setPropertyValue("uViewProj", mvpProp);
    lineShader->setPropertyValue("_color", colorProp);

    if (ignoreZDepth)
        glDisable(GL_DEPTH_TEST);

    glBindVertexArray(uiLineVAO);
    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}


void MGizmos::drawTextureRect(const SVector3& position, const SVector2& halfExtents, MTexture* texture)
{
    auto uiShader = MAssetManager::getInstance()->getAsset<MShaderAsset>("meteor_assets/engine_assets/shaders/gizmo.mesl");
    if (!texture || !uiShader)
    {
        MLOG("Shader or Texture NULL");
        return;
    };

    if (!uiQuadVAO)
        uiQuadVAO = createQuadVAO();

    // Set up orthographic projection for screen
    auto camera = getActiveCamera();
    SVector2 resolution = getResolution(); // screen resolution
    SMatrix4 proj = camera->getProjectionMatrix(resolution);
    SMatrix4 view = camera->getViewMatrix(); // no view transform

    // Model transform: translate to bottom-left and scale to size
    SVector3 bottomLeft = {
        position.x - halfExtents.x,
        position.y - halfExtents.y,
        position.z
    };

    SVector3 size = {
        halfExtents.x * 2.0f,
        halfExtents.y * 2.0f,
        1.0f
    };

    SMatrix4 model = glm::translate(SMatrix4(1.0f), bottomLeft) *
                      glm::scale(SMatrix4(1.0f), size);

    SMatrix4 mvp = proj * view * model;

    // Bind shader and set uniforms
    uiShader->getShader()->bind();

    SShaderPropertyValue uMVP;
    uMVP.setMat4Val(mvp);
    uiShader->getShader()->setPropertyValue("uMVP", uMVP);

    //bind the texture
    texture->bind(0,0);

    glDisable(GL_DEPTH_TEST);

    // Draw quad
    glBindVertexArray(uiQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void MGizmos::drawWireCube(SVector3 position, SVector3 halfExtents, SColor color, float thickness)
{
    // Compute corners of the cube (8 total)
    SVector3 min = position - halfExtents;
    SVector3 max = position + halfExtents;

    SVector3 corners[8] = {
        {min.x, min.y, min.z}, // 0
        {max.x, min.y, min.z}, // 1
        {max.x, max.y, min.z}, // 2
        {min.x, max.y, min.z}, // 3
        {min.x, min.y, max.z}, // 4
        {max.x, min.y, max.z}, // 5
        {max.x, max.y, max.z}, // 6
        {min.x, max.y, max.z}  // 7
    };

    // Bottom face
    drawLine(corners[0], corners[1], color, thickness, false);
    drawLine(corners[1], corners[2], color, thickness, false);
    drawLine(corners[2], corners[3], color, thickness, false);
    drawLine(corners[3], corners[0], color, thickness, false);

    // Top face
    drawLine(corners[4], corners[5], color, thickness, false);
    drawLine(corners[5], corners[6], color, thickness, false);
    drawLine(corners[6], corners[7], color, thickness, false);
    drawLine(corners[7], corners[4], color, thickness, false);

    // Vertical edges
    drawLine(corners[0], corners[4], color, thickness, false);
    drawLine(corners[1], corners[5], color, thickness, false);
    drawLine(corners[2], corners[6], color, thickness, false);
    drawLine(corners[3], corners[7], color, thickness, false);
}

void MGizmos::drawWireSphere(SVector3 position, float radius, SColor color, float thickness)
{
    const int segments = 32; // More segments = smoother sphere

    for (int i = 0; i < segments; ++i)
    {
        float theta1 = (float)i / segments * glm::two_pi<float>();
        float theta2 = (float)(i + 1) / segments * glm::two_pi<float>();

        SVector3 p1 = position + SVector3(radius * cos(theta1), radius * sin(theta1), 0);
        SVector3 p2 = position + SVector3(radius * cos(theta2), radius * sin(theta2), 0);

        drawLine(p1, p2, color, thickness,false);
    }

    for (int i = 0; i < segments; ++i)
    {
        float theta1 = (float)i / segments * glm::two_pi<float>();
        float theta2 = (float)(i + 1) / segments * glm::two_pi<float>();

        SVector3 p1 = position + SVector3(0, radius * cos(theta1), radius * sin(theta1));
        SVector3 p2 = position + SVector3(0, radius * cos(theta2), radius * sin(theta2));

        drawLine(p1, p2, color, thickness, false);
    }

    for (int i = 0; i < segments; ++i)
    {
        float theta1 = (float)i / segments * glm::two_pi<float>();
        float theta2 = (float)(i + 1) / segments * glm::two_pi<float>();

        SVector3 p1 = position + SVector3(radius * cos(theta1), 0, radius * sin(theta1));
        SVector3 p2 = position + SVector3(radius * cos(theta2), 0, radius * sin(theta2));

        drawLine(p1, p2, color, thickness, false);
    }
}


void MGizmos::drawRay(const SVector3& origin, const SVector3& direction, const float& length, const SColor& color, const float& thickness)
{
    glm::vec3 end = origin + direction * length;
    drawLine(origin, end, color, thickness); // shaft

    // Arrow tip (optional): draw 2 lines forming a small "V"
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0))) * (length * 0.1f);
    glm::vec3 up = glm::normalize(glm::cross(right, dir)) * (length * 0.1f);

    drawLine(end, end - dir * 0.2f + right, color, thickness);
    drawLine(end, end - dir * 0.2f - right, color, thickness);
    drawLine(end, end - dir * 0.2f + up, color, thickness);
    drawLine(end, end - dir * 0.2f - up, color, thickness);
}

void MGizmos::drawWireFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, SColor color, float thickness)
{
    // Inverse View-Projection to get clip-to-world
    glm::mat4 invVP = glm::inverse(projectionMatrix * viewMatrix);

    // Clip space corners (NDC): [-1, 1]
    glm::vec4 ndcCorners[8] = {
        {-1, -1, -1, 1}, // near bottom-left
        { 1, -1, -1, 1}, // near bottom-right
        { 1,  1, -1, 1}, // near top-right
        {-1,  1, -1, 1}, // near top-left
        {-1, -1,  1, 1}, // far bottom-left
        { 1, -1,  1, 1}, // far bottom-right
        { 1,  1,  1, 1}, // far top-right
        {-1,  1,  1, 1}  // far top-left
    };

    SVector3 worldCorners[8];

    // Transform to world space
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 world = invVP * ndcCorners[i];
        world /= world.w;
        worldCorners[i] = SVector3(world.x, world.y, world.z);
    }

    // Draw near face
    drawLine(worldCorners[0], worldCorners[1], color, thickness, false);
    drawLine(worldCorners[1], worldCorners[2], color, thickness, false);
    drawLine(worldCorners[2], worldCorners[3], color, thickness, false);
    drawLine(worldCorners[3], worldCorners[0], color, thickness, false);

    // Draw far face
    drawLine(worldCorners[4], worldCorners[5], color, thickness, false);
    drawLine(worldCorners[5], worldCorners[6], color, thickness, false);
    drawLine(worldCorners[6], worldCorners[7], color, thickness, false);
    drawLine(worldCorners[7], worldCorners[4], color, thickness, false);

    // Connect near and far corners
    for (int i = 0; i < 4; ++i)
        drawLine(worldCorners[i], worldCorners[i + 4], color, thickness, false);
}

unsigned int MGizmos::createQuadVAO()
{
    // Quad covering [0,1] in both x and y, with corresponding UVs
    float quadVertices[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,

                            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // inPos (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // inUV (location = 1)
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); // inPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glBindVertexArray(0);
}

void MGizmos::requestGizmoDraws()
{
    auto sceneManager = MSceneManager::getSceneManagerInstance();
    if (sceneManager == nullptr) return;

    auto scene = sceneManager->getActiveScene();
    if (scene == nullptr) return;

    for (auto root : scene->getRootEntities())
    {
       recursiveGizmoDraws(root);
    }
}

void MGizmos::recursiveGizmoDraws(MSpatialEntity* entity)
{
    if (entity == nullptr)
    {
        return;
    }

    entity->onDrawGizmo();

    for (auto child : entity->getChildren())
    {
        recursiveGizmoDraws(child);
    }
}
